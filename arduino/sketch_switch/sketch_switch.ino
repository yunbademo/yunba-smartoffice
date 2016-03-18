/* serial data format:
    header:
      flag: 1 byte, 0xaa, filter out debug message
      type: 1 bytes, 1: upstream, 2: downstream
      body length: 2 bytes, network byte order
    body:
      json data from yunba service
*/
#include <ArduinoJson.h>
#include <PWM.h>

#define MSG_TYPE_UP 0x01
#define MSG_TYPE_DOWN 0x02

#define HEADER_LEN 4
#define BUF_LEN 256
#define FLAG_CHAR 0xaa

#define FIRST_PIN_CTRL 3
#define FIRST_PIN_LED 6
#define FIRST_PIN_PWM 9
#define FIRST_PIN_TOUCH A1

/* 检测到 MAX_LOW_CNT 次小于 MAX_LOW_ANNALOG 的电压后, 认为是面板发出了关的信号 */
#define MAX_LOW_CNT 16
#define MAX_LOW_ANNALOG 192

#define CHILD_NUM 1


class Child;

const char *g_devid = "switch_0";

uint8_t g_header[HEADER_LEN];
uint8_t g_buf[BUF_LEN];
int g_step = 1; // 1: recv header, 2 recv body
uint16_t g_body_len = 0;
uint16_t g_recv_len = 0;

Child *g_child[CHILD_NUM];

char g_need_report = 1;
unsigned long g_check_ms = 0;


class Child {
private:
  int pin_ctrl; /* 控制继电器 */
  int pin_led; /* 控制开关的指示LED */
  int pin_pwm; /* 输出PWM给触摸面板 */
  int pin_touch; /* 接受来自触摸面板的开关信号  */

  int ctrl_value; /* 当前继电器状态 */

  int touch_value; /* 当前触摸面板认为的开关状态 */
  int low_cnt; /* 触摸面板开关信号为0的计数, 当超过阈值, 认为是关信号(因为面板输出不稳定的脉冲) */

public:
  Child(int pin_ctrl, int pin_led, int pin_pwm, int pin_touch) {
    this->pin_ctrl = pin_ctrl;
    this->pin_led = pin_led;
    this->pin_pwm = pin_pwm;
    this->pin_touch = pin_touch;

    pinMode(this->pin_ctrl, OUTPUT);
    digitalWrite(this->pin_ctrl, LOW);

    pinMode(this->pin_led, INPUT);

    SetPinFrequencySafe(this->pin_pwm, 50);
    pwmWrite(this->pin_pwm, 127);

    pinMode(this->pin_touch, INPUT);

    this->ctrl_value = 0;
    this->touch_value = 0;
    this->low_cnt = 0;
  }

  void loop() {
    int value = check_touch();
    if (this->touch_value != value) {
      Serial.println("touch signal!");
      SetCtrl(!ctrl_value);
      this->touch_value = value;
      g_need_report = 1;
    }
  }

  int check_touch() {
    int i = 0;
    i = analogRead(this->pin_touch);
  
    if (i < MAX_LOW_ANNALOG) {
      ++this->low_cnt;
      if (this->low_cnt >= MAX_LOW_CNT) {
        this->low_cnt = 0;
        return 0;
      }
    } else {
      this->low_cnt = 0;
      return 1;
    }
    return this->touch_value; 
  }

  int GetCtrl() {
    return this->ctrl_value;
  }

  void SetCtrl(int value) {
    Serial.println("set ctrl:");
    Serial.println(value);
    if (value) {
      value = 1;
    }
    if (value == this->ctrl_value) {
      return;
    }
    this->ctrl_value = value;
    if (value) {
      digitalWrite(this->pin_ctrl, HIGH);
      pinMode(this->pin_led, INPUT);
      digitalWrite(this->pin_led, LOW);
    } else {
      digitalWrite(this->pin_ctrl, LOW);
      pinMode(this->pin_led, INPUT);
    }
  }
};


void recv_header() {
  while (Serial.available() >= HEADER_LEN) {
    Serial.readBytes(g_header, 1);
//    Serial.println((uint8_t)g_header[0], HEX);
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

  Serial.println("len:");
  Serial.println(g_body_len);

  if (g_body_len > (BUF_LEN - HEADER_LEN - 1)) {
    Serial.println("too long msg");
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
  Serial.println("got a msg:");
  Serial.println((char *)g_buf);

  handle_msg();

  memset(g_header, 0, HEADER_LEN);
  g_step = 1;
}

void handle_msg() {
  StaticJsonBuffer<128> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject((char *)g_buf);
  if (!root.success()) {
    Serial.println("parseObject");
    return;
  }

  if (strcmp(root["devid"], g_devid) != 0) {
    Serial.println("not my devid");
    return;
  }

  if (strcmp(root["cmd"], "switch_set") == 0) {
    int j = min(CHILD_NUM, root["status"].size());
    for (int i = 0; i < j; i++) {
      uint8_t value = root["status"][i];
      if (value) {
        value = 1;
      }
      if (value != g_child[i]->GetCtrl()) {
        g_child[i]->SetCtrl(value);
        g_need_report = 1;
      }
    }
  } else if (strcmp(root["cmd"], "switch_get") == 0) {
    g_need_report = 1;
  }
}

void send_msg() {
  g_buf[0] = FLAG_CHAR;
  g_buf[1] = MSG_TYPE_UP; // 1: upstream
  g_buf[2] = ((uint8_t *)&g_body_len)[1];
  g_buf[3] = ((uint8_t *)&g_body_len)[0];

  Serial.write(g_buf, g_body_len + HEADER_LEN);
  ///  Serial.flush();
}

void report_status() {
  StaticJsonBuffer<128> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["devid"] = g_devid;

  JsonArray& st = root.createNestedArray("status");
  for (int i = 0; i < CHILD_NUM; i++) {
    st.add(g_child[i]->GetCtrl());
  }
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

void init_child() {
  InitTimersSafe();

  for (int i = 0; i < CHILD_NUM; i++) {
    g_child[i] = new Child(FIRST_PIN_CTRL + i, FIRST_PIN_LED + i, FIRST_PIN_PWM + i, FIRST_PIN_TOUCH + i);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //Serial.setTimeout(100);
  Serial.println("setup...");

  init_child();
}

void loop() {
  // put your main code here, to run repeatedly:
  handle_input();

  if (g_need_report) {
    report_status();
    g_need_report = 0;
  }

  for (int i = 0; i < CHILD_NUM; i++) {
    g_child[i]->loop();
  }

  delay(1);
}
