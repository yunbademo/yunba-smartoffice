/*
 *  因为一个 Arduino mini 模拟引脚不够用，此代码用于辅助 sketch_air-condition 读取温控器的 LCD 信息，
 *  用 7 位数字通知主片，高 2 位表示风速 FAN，低 5 位表示设置的温度。
 */
#define COM_NUM 4
#define SEG_NUM 4

#define COM_PIN A0
#define SEG_PIN A4

#define MIN_ANALOG_V 96
#define MAX_ANALOG_V 928

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

uint8_t g_com[COM_NUM];
uint8_t g_seg[SEG_NUM];
uint8_t g_com_cmp[COM_NUM];

uint8_t g_data_last[COM_NUM][SEG_NUM];
uint8_t g_data[COM_NUM][SEG_NUM];

uint8_t g_fan; /* 1: hi, 2: mid, 3: low, 4: auto */
uint8_t g_set_temp;

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

  /* 十位: CGB */
  i = g_data[3][seg_index];
  i += g_data[0][seg_index] * 2;
  i += g_data[1][seg_index] * 4;
  i += g_data[2][seg_index] * 8;

  switch (i) {
    case 0x0f:
      j = 0;
      break;
    case 0x00:
      j = 1;
      break;
    case 0x07:
      j = 2;
      break;
    case 0x03:
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

void setup() {
  Serial.begin(115200);
  Serial.println("setup...");

  sbi(ADCSRA,ADPS2);
  cbi(ADCSRA,ADPS1);
  cbi(ADCSRA,ADPS0);

  DDRD |= B11111100;
  DDRB |= B00000001;
}

void handle_status() {
  int i = 0;
  int j = 0;
  int v = 0;
  int f = 1;
  int t = 100;
  uint8_t data = 0;
  uint8_t u = 0;

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

#if 1
  for (i = 0; i < COM_NUM; i++) {
    for (j = 0; j < SEG_NUM; j++) {
      Serial.print(g_data[i][j], DEC);
    }
    Serial.println();
  }
  Serial.println();
#endif

  /* g_fan */
  if (g_data[1][0]) {
    g_fan = 1;
  } else if (g_data[2][0]) {
    g_fan = 2;
  } else if (g_data[3][0]) {
    g_fan = 3;
  } else {
    g_fan = 4;
  }
  g_fan -= 1;

  /* g_set_temp */
  i = make_number(1);
  if (i != -1) {
    g_set_temp = i;
  }
  g_set_temp -= 5;

  data = (((g_fan << 5) & B01100000) | (g_set_temp & B00011111));

  Serial.println("slave data:");
  Serial.println(data);

  u = ((data << 2) & B11111100) | (PORTD & B00000011);
  PORTD = u;

  u = ((data >> 6) & B00000001) | (PORTB & B11111110);
  PORTB = u;
}

void loop() {
  handle_status();
  delay(10);
}

