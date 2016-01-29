#include <Ethernet.h>
#include <ArduinoJson.h>
#include <MQTTClient.h>

const char *g_appkey = "56a0a88c4407a3cd028ac2fe";
const char *g_topic = "office";
const char *g_devid = "plug_plc";

#define BUFSIZE 192
#define JSON_BUFSIZE 96
#define PIN_CONTROL 4

uint8_t mac[] = {0xb0, 0x5a, 0xda, 0x3a, 0x2e, 0x7e};

boolean g_net_status = false;
char url[24];

char broker_addr[24];
uint16_t port;

unsigned int g_last_check_ms = 0;
char client_id[32];
char username[24];
char password[16];

EthernetClient net;
MQTTClient *mqtt_client;

uint8_t g_status = 0;

bool get_ip_port(const char *url, char *addr, uint16_t *port) {
  char *p = strstr(url, "tcp://");
  if (p) {
    p += 6;
    char *q = strchr(p, ':');
    if (q) {
      int len = strlen(p) - strlen(q);
      if (len > 0) {
        memcpy(addr, p, len);
        *port = atoi(q + 1);
#if 0
        Serial.print("i:");
        Serial.println(addr);
        Serial.print("p:");
        Serial.println(*port);
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

bool get_host_v2(const char *appkey, char *url) {
  uint8_t buf[BUFSIZE] = {0};

  String data("{\"a\":\"");
  data += String(appkey);
  data += String("\",\"n\":\"1\",\"v\":\"v1.0\",\"o\":\"1\"}");
  uint16_t len = data.length();

  buf[0] = 1;
  buf[1] = (uint8_t)((len >> 8) & 0xff);
  buf[2] = (uint8_t)(len & 0xff);

  memcpy(buf + 3, data.c_str(), len);
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
        strcpy(url, root["c"]);
        return true;
      }
    }
  }
  Serial.println("eh");
  return false;
}

bool setup_with_appkey_and_devid(const char *appkey, const char *devid) {
  uint8_t buf[BUFSIZE];

  if (appkey == NULL) return false;

  String data("{\"a\": \"");
  data += String(appkey);

  if (devid == NULL) {
    data += String("\", \"p\":4}");
  } else {
    data += String("\", \"p\":4, \"d\": \"");
    data += String(devid);
    data += String("\"}");
  }
  uint16_t len = data.length();

  buf[0] = 1;
  buf[1] = (uint8_t)((len >> 8) & 0xff);
  buf[2] = (uint8_t)(len & 0xff);

  memcpy(buf + 3, data.c_str(), len);
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
        strcpy(username, root["u"]);
        strcpy(password, root["p"]);
        strcpy(client_id, root["c"]);
        return true;
      }
    }
  }

  Serial.println("es");
  return false;
}

void set_alias(const char *alias) {
  mqtt_client->publish(",yali", alias);
}

void mqtt_connect() {
  Serial.println("cn.."); // connecting
  while (!mqtt_client->connect(client_id, username, password)) {
    Serial.println("..");
    delay(1000);
  }

  Serial.println("co"); // connect ok

//  mqtt_client->subscribe(g_topic);
  set_alias(g_devid);
}

void check_connect() {
  if (millis() - g_last_check_ms > 2000) {
    boolean st = mqtt_client->connected();
//    Serial.println(st);
    if (st != g_net_status) {
      Serial.print("cs:");
      g_net_status = st;
      Serial.println(g_net_status);
    }

    if (!st) {
      mqtt_client->disconnect();
      delete(mqtt_client);
      init_network();
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
  report_status();
}

void report_status() {
  String data("{\"status\":");
  data += String(g_status);
  data += String(",\"devid\":\"");
  data += String(g_devid);
  data += String("\"}");

  Serial.println(data);
  mqtt_client->publish(g_topic, data.c_str());
}

void messageReceived(String topic, String payload, char *bytes, unsigned int length) {
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
}

void extMessageReceived(EXTED_CMD cmd, int status, String payload, unsigned int length) {
  Serial.println("em");
}

void init_ethernet() {
//  IPAddress ip(192,168,2,183);
//  Ethernet.begin(mac, ip);

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

void init_network() {
  init_ethernet();

  //TODO: if we can't get reg info and tick info
  get_host_v2(g_appkey, url);
  get_ip_port(url, broker_addr, &port);

  setup_with_appkey_and_devid(g_appkey, g_devid);

  mqtt_client->begin(broker_addr, port, net);
  mqtt_connect();
}

void setup() {

  Serial.begin(57600);
  Serial.println("in.."); // init

  pinMode(PIN_CONTROL, OUTPUT);
  digitalWrite(PIN_CONTROL, LOW);

  mqtt_client = new MQTTClient();
  init_network();

  Serial.println("io"); // init ok
}

void loop() {
  mqtt_client->loop();

  check_connect();

  Ethernet.maintain();

  delay(100);
}

