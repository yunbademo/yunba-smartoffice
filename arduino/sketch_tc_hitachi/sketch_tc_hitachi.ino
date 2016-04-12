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

#define COM_NUM 3
#define SEG_NUM 5

#define COM_PIN A0
#define SEG_PIN A3

#define PIN_ON_OFF 9
#define PIN_MODE 10
#define PIN_FAN 11
#define PIN_INC 12
#define PIN_DEC 13

#define MIN_ANALOG_V 256
#define MAX_ANALOG_V 938

#define MAX_STABLE_CNT 32 /* 去除波动，MAX_STABLE_CNT 次数据不变后才上报状态 */

#define SIM_BTN_DELAY 32

const char *g_devid = "temp_ctrl_1";

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
uint8_t g_data_stable[COM_NUM][SEG_NUM];

uint8_t g_on_off; /* 1: on, 2: off */
uint8_t g_mode; /* 1: cool(制冷), 2: hot(制热), 3: dry(抽湿), 4: blast(送风) */
uint8_t g_fan; /* 1: hi(高风), 2: mid(中风), 3: low(低风), 4: auto(自动) */
int8_t g_set_temp;
int8_t g_room_temp;

uint8_t g_stable_cnt = 0; /* 稳定次数 */
uint8_t g_stable_cnt_slave = 0;

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

  Serial.println((const char *)root["cmd"]);

  if (strcmp(root["cmd"], "on_off") == 0) {
    digitalWrite(PIN_ON_OFF, LOW);
    delay(SIM_BTN_DELAY);
    digitalWrite(PIN_ON_OFF, HIGH);
  } else if (strcmp(root["cmd"], "mode") == 0) {
    digitalWrite(PIN_MODE, LOW);
    delay(SIM_BTN_DELAY);
    digitalWrite(PIN_MODE, HIGH);
  } else if (strcmp(root["cmd"], "fan") == 0) {
    digitalWrite(PIN_FAN, LOW);
    delay(SIM_BTN_DELAY);
    digitalWrite(PIN_FAN, HIGH);
  } else if (strcmp(root["cmd"], "inc") == 0) {
    digitalWrite(PIN_INC, LOW);
    delay(SIM_BTN_DELAY);
    digitalWrite(PIN_INC, HIGH);
  } else if (strcmp(root["cmd"], "dec") == 0) {
    digitalWrite(PIN_DEC, LOW);
    delay(SIM_BTN_DELAY);
    digitalWrite(PIN_DEC, HIGH);
  } else if (strcmp(root["cmd"], "get") == 0) {
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

  st.add(g_on_off);
  st.add(g_mode);
  st.add(g_fan);
  st.add(g_set_temp);
  st.add(g_room_temp);

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
  uint8_t i = 0;
  uint8_t j = 0;
  uint8_t k = 0;

  /* 十位: BCD */
  i = g_data[0][seg_index + 2];
  i += g_data[1][seg_index + 2] * 2;
  i += g_data[2][seg_index + 2] * 4;

  switch (i) {
    case 0x03:
      j = 1;
      break;
    case 0x05:
      j = 2;
      break;
    case 0x07:
      j = 3;
      break;
    default:
      //j = 0;
      //break;
      return -1;
  }

  /* 个位: BCDFGE */
  i = g_data[0][seg_index];
  i += g_data[1][seg_index] * 2;
  i += g_data[2][seg_index] * 4;
  i += g_data[0][seg_index + 1] * 8;
  i += g_data[1][seg_index + 1] * 16;
  i += g_data[2][seg_index + 1] * 32;

  switch (i) {
    case 0x2f:
      k = 0;
      break;
    case 0x03:
      k = 1;
      break;
    case 0x35:
      k = 2;
      break;
    case 0x17:
      k = 3;
      break;
    case 0x1b:
      k = 4;
      break;
    case 0x1e:
      k = 5;
      break;
    case 0x3e:
      k = 6;
      break;
    case 0x0b:
      k = 7;
      break;
    case 0x3f:
      k = 8;
      break;
    case 0x1f:
      k = 9;
      break;
    default:
      return -1;
  }

  return j * 10 + k;
}

void handle_status() {
  int i = 0;
  int j = 0;
  int v = 0;
  int f = 1;
  int t = 64;
  uint8_t z = 0; /* 过滤掉全 0 的无效数据 */

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
        z |= g_data[i][j];
      }
    }
  }
  if (!z) {
    return;
  }

  if (memcmp(g_data, g_data_last, COM_NUM * SEG_NUM) == 0) {
    if (g_stable_cnt >= MAX_STABLE_CNT) {
      return; /* reported */
    }
    g_stable_cnt++;
    if (g_stable_cnt < MAX_STABLE_CNT) {
      return;
    }
    if (memcmp(g_data_stable, g_data, COM_NUM * SEG_NUM) == 0) {
      g_stable_cnt = 0;
      return;
    } else {
      memcpy(g_data_stable, g_data, COM_NUM * SEG_NUM);
    }
  } else {
    memcpy(g_data_last, g_data, COM_NUM * SEG_NUM);
    g_stable_cnt = 0;
    return;
  }

#if 1
  for (i = 0; i < COM_NUM; i++) {
    for (j = 0; j < SEG_NUM; j++) {
      Serial.print(g_data[i][j], DEC);
    }
    Serial.println();
  }
#endif

  /* g_mode */
  if (g_data[0][4]) {
    g_mode = 1;
  } else if (g_data[1][4]) {
    g_mode = 2;
  } else if (g_data[2][4]) {
    g_mode = 3;
  } else {
    g_mode = 4;
  }

  /* g_fan */
  if (g_data[0][3]) {
    g_fan = 2;
  } else if (g_data[1][3]) {
    g_fan = 3;
  } else {
    g_fan = 1;
  }

  /* g_set_temp */
  g_set_temp = make_number(0);
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

  pinMode(PIN_ON_OFF, OUTPUT);
  digitalWrite(PIN_ON_OFF, HIGH);
  pinMode(PIN_MODE, OUTPUT);
  digitalWrite(PIN_MODE, HIGH);
  pinMode(PIN_FAN, OUTPUT);
  digitalWrite(PIN_FAN, HIGH);
  pinMode(PIN_INC, OUTPUT);
  digitalWrite(PIN_INC, HIGH);
  pinMode(PIN_DEC, OUTPUT);
  digitalWrite(PIN_DEC, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
  handle_input();

  handle_status();

  if (g_need_report) {
    print_status();
//    report_status();
    g_need_report = 0;
  }

//  delay(100);
}
