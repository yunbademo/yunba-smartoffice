/* serial data format:
    header:
      flag: 1 byte, 0xaa, filter out debug message
      type: 1 bytes, 1: upstream, 2: downstream
      body length: 2 bytes, network byte order
    body:
      json data from yunba service
*/
#include <ArduinoJson.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define MSG_TYPE_UP 0x01
#define MSG_TYPE_DOWN 0x02

#define HEADER_LEN 4
#define BUF_LEN 256
#define FLAG_CHAR 0xaa

#define COM_NUM 4
#define SEG_NUM 4

#define COM_PIN A0
#define SEG_PIN A4

#define PIN_ON_OFF 9
#define PIN_MODE 10
#define PIN_FAN 11
#define PIN_ADD_TEMP 12
#define PIN_DEC_TEMP 13

#define MIN_ANALOG_V 32
#define MAX_ANALOG_V 992


const char *g_devid = "air-condition_0";

uint8_t g_header[HEADER_LEN];
uint8_t g_buf[BUF_LEN];
int g_step = 1; // 1: recv header, 2 recv body
uint16_t g_body_len = 0;
uint16_t g_recv_len = 0;

char g_need_report = 0;
unsigned long g_check_ms = 0;

uint8_t g_com[COM_NUM];
uint8_t g_seg[SEG_NUM];
uint8_t g_com_cmp[COM_NUM];

uint8_t g_data_last[COM_NUM][SEG_NUM];
uint8_t g_data[COM_NUM][SEG_NUM];

int g_on_off; /* 1: on, 2: off */
int g_mode; /* 1: cool, 2: hot */
int g_fan; /* 1: hi, 2: mid, 3: low, 4: auto */
int g_set_temp;
int g_room_temp;

void recv_header() {
  while (Serial.available() >= HEADER_LEN) {
    Serial.readBytes(g_header, 1);
//    //Serial.println((uint8_t)g_header[0], HEX);
    if (g_header[0] == FLAG_CHAR) {
      break;
    }
  }

  if (g_header[0] != FLAG_CHAR) { // no enough data
    return;
  }

  Serial.readBytes(g_header + 1, HEADER_LEN - 1);

  if (g_header[1] != MSG_TYPE_DOWN) { // not a downstream message
    return;
  }

  /* le */
  ((uint8_t *)&g_body_len)[0] = g_header[3];
  ((uint8_t *)&g_body_len)[1] = g_header[2];

  //Serial.println("len:");
  //Serial.println(g_body_len);

  if (g_body_len > (BUF_LEN - HEADER_LEN - 1)) {
    //Serial.println("too long msg");
    return;
  }

  g_recv_len = 0;
  g_step = 2;
}

void recv_body() {
  uint16_t len = 0;

  len = Serial.readBytes(g_buf + g_recv_len, g_body_len - g_recv_len);
  g_recv_len += len;

  if (g_recv_len != g_body_len) {
    return;
  }

  // now got a completed msg
  g_buf[g_body_len] = 0;
  //Serial.println("got a msg:");
  //Serial.println((char *)g_buf);

  handle_msg();

  memset(g_header, 0, HEADER_LEN);
  g_step = 1;
}

void handle_msg() {
  StaticJsonBuffer<256> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject((char *)g_buf);
  if (!root.success()) {
    //Serial.println("parseObject");
    return;
  }

  if (strcmp(root["devid"], g_devid) != 0) {
    //Serial.println("not my devid");
    return;
  }

  if (strcmp(root["cmd"], "ac_set") == 0) {
  } else if (strcmp(root["cmd"], "ac_get") == 0) {
    g_need_report = 1;
  }
}

void send_msg() {
  g_buf[0] = FLAG_CHAR;
  g_buf[1] = MSG_TYPE_UP; // 1: upstream
  g_buf[2] = ((uint8_t *)&g_body_len)[1];
  g_buf[3] = ((uint8_t *)&g_body_len)[0];

  Serial.write(g_buf, g_body_len + HEADER_LEN);
}

void report_status() {
  StaticJsonBuffer<256> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["devid"] = g_devid;

  JsonArray& st = root.createNestedArray("status");
  
  g_body_len = root.printTo((char *)g_buf + HEADER_LEN, BUF_LEN - HEADER_LEN);

  send_msg();
}

void handle_input() {
  if (g_step == 1) {
    recv_header();
  } else if (g_step == 2) {
    recv_body();
  }
}

inline int trans(int i) {
  if (i < MIN_ANALOG_V) {
    return 0;
  } else if (i > MAX_ANALOG_V) {
    return 1;
  } else {
    return 2;
  }
}


int make_number(int seg_index) {
  int i = 0;
  int j = 0;
  int k = 0;

  /* 十位: CGB */
  i = g_data[0][seg_index];
  i += g_data[1][seg_index] * 2;
  i += g_data[2][seg_index] * 4;

  switch (i) {
    case 0x05:
      j = 1;
      break;
    case 0x06:
      j = 2;
      break;
    case 0x07:
      j = 3;
      break;
    default:
      return -1;
  }

  /* 个位: ABCDEFG */
  i = g_data[3][seg_index + 1];
  i += g_data[2][seg_index + 2] * 2;
  i += g_data[0][seg_index + 2] * 4;
  i += g_data[0][seg_index + 1] * 8;
  i += g_data[1][seg_index + 1] * 16;
  i += g_data[2][seg_index + 1] * 32;
  i += g_data[1][seg_index + 2] * 64;

  switch (i) {
    case 0x3f:
      k = 0;
      break;
    case 0x06:
      k = 1;
      break;
    case 0x5b:
      k = 2;
      break;
    case 0x4f:
      k = 3;
      break;
    case 0x66:
      k = 4;
      break;
    case 0x6d:
      k = 5;
      break;
    case 0x7d:
      k = 6;
      break;
    case 0x07:
      k = 7;
      break;
    case 0x7f:
      k = 8;
      break;
    case 0x6f:
      k = 9;
      break;
    default:
      return -1;
  }

  return j * 10 + k;
}

#if 0
void handle_status_2() {
   int v = 0;
  uint8_t data = 0;

  data = ((PIND >> 2) & B00111111);
  data |= ((PINB << 6) & B01000000);
    /* 转换 */
  for (i = 0; i < SEG_NUM_S; i++) {
    v = data % 3;
    g_seg[SEG_INDEX_S + i] = v;
    data /= 3;
  }
  /* g_fan */
  if (g_data[1][1]) {
    g_fan = 1;
    g_on_off = 1;
  } else if (g_data[2][1]) {
    g_fan = 2;
    g_on_off = 1;
  } else if (g_data[3][1]) {
    g_fan = 3;
    g_on_off = 1;
  } else if (g_data[3][2]) {
    g_fan = 4;
    g_on_off = 1;
  } else {
    g_on_off = 2;
  }

  /* g_set_temp */
  g_set_temp = make_number(2);
}
#endif

void handle_status_2() {
  uint8_t data = 0;
  uint8_t data_cmp = 0;
  int t = 100;

  while (t--) {
    data = ((PIND >> 2) & B00111111);
    data |= ((PINB << 6) & B01000000);

    delay(2);

    data_cmp = ((PIND >> 2) & B00111111);
    data_cmp |= ((PINB << 6) & B01000000);

    if (data == data_cmp) {
      break;
    }
  }

  /* g_fan */
  g_fan = ((data >> 5) & B00000011) + 1;

  /* g_set_temp */
  g_set_temp = (data & B00011111) + 5;
}

void handle_status() {
  int i = 0;
  int j = 0;
  int v = 0;
  int f = 1;
  int t = 100;

  while (t--) {
    /* 读 COM */
    for (i = 0; i < COM_NUM; i++) {
      g_com_cmp[i] = trans(analogRead(COM_PIN + i));
    }
    /* 读 SEG */
    for (i = 0; i < SEG_NUM; i++) {
      g_seg[i] = trans(analogRead(SEG_PIN + i));
    }

    /* 再读 COM */
    for (i = 0; i < COM_NUM; i++) {
      g_com[i] = trans(analogRead(COM_PIN + i));
    }

    /* 两次读的 COM 没变，才认为这次读的有效 */
    if (memcmp(g_com, g_com_cmp, COM_NUM) == 0) {
      //Serial.println("valid reading");
      break;
    }
  }
  
  for (i = 0; i < COM_NUM; i++) {
    if (g_com[i] == 2) {
      continue;
    }
    for (j = 0; j < SEG_NUM; j++) {
      if (g_seg[j] == 2) {
        g_data[i][j] = 0;
      } else {
        g_data[i][j] = abs(g_com[i] - g_seg[j]);
      }
    }
  }

  if (memcmp(g_data, g_data_last, COM_NUM * SEG_NUM) == 0) {
    return;
  }

  memcpy(g_data_last, g_data, COM_NUM * SEG_NUM);

  /* g_mode */
  if (g_data[1][0]) {
    g_mode = 1;
    g_on_off = 1;
  } else if (g_data[2][0]) {
    g_mode = 2;
    g_on_off = 1;
  } else {
    g_on_off = 2;
  }

  /* g_room_temp */
  i = make_number(1);
  if (i != -1) {
    g_room_temp = i;
  }
 
  g_need_report = 1;
}

void print_status() {
  Serial.println("status:");
  Serial.print(' ');
  Serial.print(g_on_off, DEC);
  Serial.print(' ');
  Serial.print(g_mode, DEC);
  Serial.print(' ');
  Serial.print(g_fan, DEC);
  Serial.print(' ');
  Serial.print(g_set_temp, DEC);
  Serial.print(' ');
  Serial.println(g_room_temp);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("setup...");

  sbi(ADCSRA,ADPS2);
  cbi(ADCSRA,ADPS1);
  cbi(ADCSRA,ADPS0);

  for (int i = 0; i < COM_NUM; i++) {
    pinMode(COM_PIN + i, INPUT);
  }
  for (int i = 0; i < SEG_NUM; i++) {
    pinMode(SEG_PIN + i, INPUT);
  }

  DDRD &= B00000011;
  DDRB &= B11111110;

  pinMode(PIN_ON_OFF, OUTPUT);
  digitalWrite(PIN_ON_OFF, LOW);
  pinMode(PIN_MODE, OUTPUT);
  digitalWrite(PIN_MODE, LOW);
  pinMode(PIN_FAN, OUTPUT);
  digitalWrite(PIN_FAN, LOW);
  pinMode(PIN_ADD_TEMP, OUTPUT);
  digitalWrite(PIN_ADD_TEMP, LOW);
  pinMode(PIN_DEC_TEMP, OUTPUT);
  digitalWrite(PIN_DEC_TEMP, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
//  handle_input();

  handle_status();
//  handle_status_2();

  if (g_need_report) {
    print_status();
//    report_status();
    g_need_report = 0;
  }

  delay(200);
}
