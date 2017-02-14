#include <Ethernet.h>
#include <ArduinoJson.h>
#include <MQTTClient.h>

#define BUFSIZE 128
#define JSON_BUFSIZE 128

//#define PIN_NET_STATUS 8
#define PIN_PLUG_CONTROL 9

const char *g_appkey = "563c4afef085fc471efdf803";
const char *g_topic = "yunba_smart_plug";
const char *g_devid = "plc_0";

bool g_net_status = false;

char *g_addr = "182.92.106.18";
uint16_t g_port = 1883;

unsigned long g_last_check_ms = 0;
char *g_client_id = "0000002823-000000057760";
char *g_username = "2886481985262254976";
char *g_password = "13ddd9f3df8f1";
bool g_need_report = true;

unsigned long g_connected_ms = 0;

EthernetClient *g_net_client;
MQTTClient *g_mqtt_client;

uint8_t g_plug_status = 0;

void (*reset)(void) = 0;

uint8_t g_retry_cnt = 0;

void retry_or_reset(uint8_t try_cnt) {
  if (g_retry_cnt >= try_cnt) {
    reset();
  } else {
    ++g_retry_cnt;
  }
}

void check_connect() {
  if (millis() - g_last_check_ms > 10000) {
    bool st = g_mqtt_client->connected();
    Serial.print("cs:");
    Serial.println(st);

    if (st != g_net_status) {
      g_net_status = st;
    }

    if (!st) {
      reset();
//      digitalWrite(PIN_NET_STATUS, LOW);
      delete g_mqtt_client;
      delete g_net_client;

      init_ethernet();
      init_yunba();
      connect_yunba();
    }
    g_last_check_ms = millis();
  }
}

void set_plug_status(uint8_t status) {
  if (status != 0)
    status = 1;

  if (g_plug_status == status)
    return;

  g_plug_status = status;
  if (status == 0) {
    Serial.println(0);
    digitalWrite(PIN_PLUG_CONTROL, LOW);
  } else {
    Serial.println(1);
    digitalWrite(PIN_PLUG_CONTROL, HIGH);
  }
  g_need_report = true;
}

void report_status() {
  uint8_t buf[BUFSIZE];

  snprintf((char *)buf, BUFSIZE, "{\"status\":%d,\"devid\":\"%s\"}", g_plug_status, g_devid);
  Serial.println((char *)buf);
  g_mqtt_client->publish(g_topic, (char *)buf);
}

void messageReceived(String topic, String payload, char *bytes, unsigned int length) {
  #if 1
  StaticJsonBuffer<JSON_BUFSIZE> jsonBuffer;

  bytes[length] = 0;
  Serial.println(bytes);

  if (millis() - g_connected_ms < 5000) { // discard wrong offline message...
    Serial.println("ds");
    return;
  }

  JsonObject& root = jsonBuffer.parseObject(bytes);
  if (!root.success()) {
    Serial.println("js");
    return;
  }

  if (strcmp(root["devid"], g_devid) != 0) {
    Serial.println("dv");
    return;
  }

  if (strcmp(root["cmd"], "plug_set") == 0) {
    uint8_t st = root["status"];
    set_plug_status(st);
  } else if (strcmp(root["cmd"], "plug_get") == 0) {
    report_status();
  }
  #endif
}

void extMessageReceived(EXTED_CMD cmd, int status, String payload, unsigned int length) {
  Serial.println("em");
  Serial.println(cmd);
}

void init_ethernet() {
  uint8_t mac[] = {0xb0, 0x5a, 0xda, 0x3a, 0x2e, 0x9f};

  Serial.println("ie.."); // init ethernet
  g_retry_cnt = 0;
  while (!Ethernet.begin(mac)) {
    Serial.println("..");
    delay(100);
    retry_or_reset(0);
  }

  Serial.print("i:");
  Serial.println(Ethernet.localIP());
#if 0
  Serial.print("s:");
  Serial.println(Ethernet.subnetMask());
  Serial.print("g:");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("d:");
  Serial.println(Ethernet.dnsServerIP());
#endif
}

void init_yunba() {
  g_net_client = new EthernetClient();
  g_mqtt_client = new MQTTClient();
  g_mqtt_client->begin(g_addr, g_port, *g_net_client);
}

void connect_yunba() {
  Serial.println("cn.."); // connecting

  g_retry_cnt = 0;
  while (!g_mqtt_client->connect(g_client_id, g_username, g_password)) {
    Serial.println("..");
    delay(100);
    retry_or_reset(5);
  }

  g_last_check_ms = millis();
  Serial.println("co"); // connect ok

//  g_mqtt_client->subscribe(g_topic);
  g_mqtt_client->publish(",yali", g_devid); // set alias

  g_connected_ms = millis();
//  digitalWrite(PIN_NET_STATUS, HIGH);
}

void setup() {

  Serial.begin(57600);
  Serial.println("st.."); // setup

//  pinMode(PIN_NET_STATUS, OUTPUT);
//  digitalWrite(PIN_NET_STATUS, LOW);

  pinMode(PIN_PLUG_CONTROL, OUTPUT);
  digitalWrite(PIN_PLUG_CONTROL, LOW);

  init_ethernet();

  init_yunba();
  connect_yunba();

  Serial.println("so"); // init ok
}

void loop() {
  g_mqtt_client->loop();

  check_connect();

  if (g_need_report) {
    report_status();
    g_need_report = false;
  }

//  Ethernet.maintain();

//  delay(100);
}

