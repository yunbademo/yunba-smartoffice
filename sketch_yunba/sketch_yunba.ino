#include <Ethernet.h>
#include <ArduinoJson.h>
#include <MQTTClient.h>

#define BUFSIZE 128
#define JSON_BUFSIZE 128
#define PIN_CONTROL 4

const char *g_appkey = "56a0a88c4407a3cd028ac2fe";
const char *g_topic = "office";
const char *g_devid = "plug_plc";

bool g_net_status = false;
char g_url[32];
char g_addr[32];
uint16_t g_port;

unsigned long g_last_check_ms = 0;
char g_client_id[32];
char g_username[24];
char g_password[16];
bool g_need_report = true;

EthernetClient *g_net_client;
MQTTClient *g_mqtt_client;

uint8_t g_status = 0;

bool get_ip_port() {
  char *p = strstr(g_url, "tcp://");
  if (p) {
    p += 6;
    char *q = strchr(p, ':');
    if (q) {
      int len = strlen(p) - strlen(q);
      if (len > 0) {
        memcpy(g_addr, p, len);
        g_port = (uint16_t)atoi(q + 1);
#if 0
        Serial.print("i:");
        Serial.println(g_addr);
        Serial.print("p:");
        Serial.println(g_port);
#endif
        return true;
      }
    }
  }
  return false;
}

void simple_send_recv(uint8_t *buf, uint16_t *len, const char *host, uint16_t port) {
  EthernetClient net_client;

  while (0 == net_client.connect(host, port)) {
    Serial.println("cs"); // connect server
    delay(1000);
  }
  delay(100);

  Serial.println("wd"); // write data
  Serial.println((char *)buf + 3);
  net_client.write(buf, *len);
  net_client.flush();

  Serial.println("ca"); // check available
  while (!net_client.available()) {
    Serial.println(".."); // wait data
    delay(1000);
  }

  Serial.println("rd"); // read data
  *len = net_client.read(buf, BUFSIZE - 1);
  buf[*len] = 0;

  net_client.stop();
}

bool get_host_v2() {
  uint8_t buf[BUFSIZE];
  uint16_t len;

  len = snprintf((char *)buf + 3, BUFSIZE - 3, "{\"a\":\"%s\",\"n\":\"1\",\"v\":\"v1.0\",\"o\":\"1\"}", g_appkey);

  buf[0] = 1;
  buf[1] = (uint8_t)((len >> 8) & 0xff);
  buf[2] = (uint8_t)(len & 0xff);

  len += 3;

  buf[len] = 0;

  simple_send_recv(buf, &len, "tick-t.yunba.io", 9977);

  if (len > 0) {
    len = (uint16_t)(((uint8_t)buf[1] << 8) | (uint8_t)buf[2]);
    char *p = (char *)buf + 3;
    if (len == strlen(p)) {
      Serial.println(p);
      StaticJsonBuffer<JSON_BUFSIZE> jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(p);
      if (root.success()) {
        strcpy(g_url, root["c"]);
        return true;
      }
    }
  }
  Serial.println("eh");
  return false;
}

bool setup_with_appkey_and_devid() {
  uint8_t buf[BUFSIZE];
  uint16_t len;

  len = snprintf((char *)buf + 3, BUFSIZE - 3, "{\"a\":\"%s\",\"p\":4,\"d\":\"%s\"}", g_appkey, g_devid);

  buf[0] = 1;
  buf[1] = (uint8_t)((len >> 8) & 0xff);
  buf[2] = (uint8_t)(len & 0xff);

  len += 3;

  buf[len] = 0;

  simple_send_recv(buf, &len, "reg-t.yunba.io", 9944);

  if (len > 0) {
    len = (uint16_t)(((uint8_t)buf[1] << 8) | (uint8_t)buf[2]);
    char *p = (char *)buf + 3;
    if (len == strlen(p)) {
      Serial.println(p);
      StaticJsonBuffer<JSON_BUFSIZE> jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(p);
      if (root.success()) {
        strcpy(g_username, root["u"]);
        strcpy(g_password, root["p"]);
        strcpy(g_client_id, root["c"]);
        return true;
      }
    }
  }

  Serial.println("es");
  return false;
}

void mqtt_init() {

}

void check_connect() {
  if (millis() - g_last_check_ms > 2000) {
    bool st = g_mqtt_client->connected();
    Serial.print("cs:");
    Serial.println(st);

    if (st != g_net_status) {
      g_net_status = st;
    }

    if (!st) {
      delete g_mqtt_client;
      delete g_net_client;

      init_ethernet();
      init_yunba();
      connect_yunba();
    }
    g_last_check_ms = millis();
  }
}

void set_status(uint8_t status) {
  if (status != 0)
    status = 1;

  if (g_status == status)
    return;

  g_status = status;
  if (status == 0) {
    Serial.println(0);
    digitalWrite(PIN_CONTROL, LOW);
  } else {
    Serial.println(1);
    digitalWrite(PIN_CONTROL, HIGH);
  }
  g_need_report = true;
}

void report_status() {
  uint8_t buf[BUFSIZE];

  snprintf((char *)buf, BUFSIZE, "{\"status\":%d,\"devid\":\"%s\"}", g_status, g_devid);
  Serial.println((char *)buf);
  g_mqtt_client->publish(g_topic, (char *)buf);
}

void messageReceived(String topic, String payload, char *bytes, unsigned int length) {
  #if 1
  StaticJsonBuffer<JSON_BUFSIZE> jsonBuffer;

  bytes[length] = 0;
  Serial.println(bytes);

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
      set_status(st);
  } else if (strcmp(root["cmd"], "plug_get") == 0) {
    report_status();
  }
  #endif
}

void extMessageReceived(EXTED_CMD cmd, int status, String payload, unsigned int length) {
  Serial.println("em");
}

void init_ethernet() {
  uint8_t mac[] = {0xb0, 0x5a, 0xda, 0x3a, 0x2e, 0x7e};

  Serial.println("ie.."); // init ethernet
  while (!Ethernet.begin(mac)) {
    Serial.println("..");
    delay(1000);
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
  get_host_v2();
  get_ip_port();
  setup_with_appkey_and_devid();

  g_net_client = new EthernetClient();
  g_mqtt_client = new MQTTClient();
  g_mqtt_client->begin(g_addr, g_port, *g_net_client);
}

void connect_yunba() {
  Serial.println("cn.."); // connecting

  while (!g_mqtt_client->connect(g_client_id, g_username, g_password)) {
    Serial.println("..");
    delay(1000);
  }

  g_last_check_ms = millis();
  Serial.println("co"); // connect ok

//  g_mqtt_client->subscribe(g_topic);
  g_mqtt_client->publish(",yali", g_devid); // set alias
}

void setup() {

  Serial.begin(57600);
  Serial.println("st.."); // setup

  pinMode(PIN_CONTROL, OUTPUT);
  digitalWrite(PIN_CONTROL, LOW);

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

  delay(100);
}

