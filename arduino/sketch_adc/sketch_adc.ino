/* 因为一个 Arduino mini 模拟引脚不够用，此代码用于辅助 sketch_air-condition 读取温控器的 LCD 信息 */
#define IN_PIN (A0)
#define OUT_PIN (2)

/* 模拟输入只有最低和最高有效，其他无效，所以他有 0(低), 1(高), 2(无效) 3个状态，可以用 7 个数字引脚输出给主 Arduino mini。(3 ^ 4 = 81, 2 ^ 7 = 128) */
#define IN_NUM (4)
#define OUT_NUM (7)

int g_data = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("setup...");

  for (int i = 0; i < IN_NUM; i++) {
    pinMode(IN_PIN + i, INPUT);
  }

  for (int i = 0; i < OUT_NUM; i++) {
    pinMode(OUT_PIN + i, OUTPUT);
    digitalWrite(OUT_PIN + i, LOW);
  }
}

inline void read_input() {
  int v = 0;
  int f = 1;

  g_data = 0;

  for (int i = 0; i < IN_NUM; i++) {
    v = analogRead(IN_PIN + i);
    if (v < 32) {
      v = 0;
    } else if (v > 992) {
      v = 1;
    } else {
      v = 2;
    }

    g_data += (v * f);
    f *= 3;
  }
}

inline void write_output() {
  int r = 0;
  for (int i = 0; i < OUT_NUM; i++) {
    r = g_data % 2;
    if (r == 0) {
      digitalWrite(OUT_PIN + i, LOW);
    } else {
      digitalWrite(OUT_PIN + i, HIGH);
    }
    g_data = g_data / 2;
  }
}

void loop() {
  read_input();
  Serial.println("data is:");
  Serial.println(g_data);
  write_output();
}
