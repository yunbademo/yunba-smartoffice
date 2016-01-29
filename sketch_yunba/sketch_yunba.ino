#include <UIPEthernet.h>
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
char client_id[24];
char username[24];
char password[16];

EthernetClient net;
MQTTClient client;

bool get_ip_port(const char *url, char *addr, uint16_t *port) {
  char *p = strstr(url, "tcp://");
  if (p) {
    p += 6;
    char *q = strstr(p, ":");
    if (q) {
      int len = strlen(p) - strlen(q);
      if (len > 0) {
        memcpy(addr, p, len);
        //sprintf(addr, "%.*s", len, p);
        *port = atoi(q + 1);
        Serial.print("i:");
        Serial.println(addr);
        Serial.print("p:");
        Serial.println(*port);
        return true;
      }
    }
  }
  return false;
}

void simple_send_recv(uint8_t *buf, uint16_t *len, const char *host, uint16_t port) {
  EthernetClient net_client;

  while (0 == net_client.connect(host, port)) {
    Serial.println("c");
    delay(1000);
  }
  delay(100);

  net_client.write(buf, *len);
  net_client.flush();

  while (!net_client.available()) {
    Serial.println("a");
    delay(1000);
  }

  *len = net_client.read(buf, BUFSIZE - 1);
  buf[*len] = 0;

  net_client.stop();
}

void simple_send_recv2(uint8_t *buf, uint16_t *len, IPAddress ip, uint16_t port) {
  EthernetClient net_client;

  while (0 == net_client.connect(ip, port)) {
    Serial.println("c");
    delay(1000);
  }
  delay(100);

  net_client.write(buf, *len);
  net_client.flush();

  while (!net_client.available()) {
    Serial.println("a");
    delay(1000);
  }

  *len = net_client.read(buf, BUFSIZE - 1);
  buf[*len] = 0;

  net_client.stop();
}

bool get_host_v2(const char *appkey, char *url) {
  uint8_t buf[BUFSIZE];

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
  Serial.println((char *)buf + 3);

//  simple_send_recv(buf, &len, "tick-t.yunba.io", 9977);

  IPAddress ip(101, 200, 229, 48);
  simple_send_recv2(buf, &len, ip, 9977);

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
  Serial.println("e1");
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
  Serial.println((char *)buf + 3);

//  simple_send_recv(buf, &len, "reg-t.yunba.io", 9944);

  IPAddress ip(182, 92, 105, 230);
  simple_send_recv2(buf, &len, ip, 9944);

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

  Serial.println("e2");
  return false;
}

void set_alias(const char *alias) {
  client.publish(",yali", alias);
}

void connect() {
//  Serial.print("\nconnecting...");
  while (!client.connect(client_id, username, password)) {
//    Serial.print(".");
    delay(1000);
  }

//  Serial.println("\nconnected!");
//  client.subscribe(g_topic);
  set_alias(g_devid);
}

void check_connect() {
  if (millis() - g_last_check_ms > 2000) {
    boolean st = client.connected();
//    Serial.println(st);
    if (st != g_net_status) {
      Serial.print("cst:");
      g_net_status = st;
      Serial.println(g_net_status);
    }

    if (!st) {
      connect();
    }
    g_last_check_ms = millis();
  }
}

void plug_set(uint8_t status) {
  if (status == 0) {
    Serial.println(0);
    digitalWrite(PIN_CONTROL, LOW);
  } else if (status == 1) {
    Serial.println(1);
    digitalWrite(PIN_CONTROL, HIGH);
  }
}

void messageReceived(String topic, String payload, char *bytes, unsigned int length) {
  StaticJsonBuffer<JSON_BUFSIZE> jsonBuffer;

  Serial.println(payload);

  JsonObject& root = jsonBuffer.parseObject(payload);
  if (!root.success()) {
    Serial.println("bad json");
    return;
  }

  if (strcmp(root["devid"], g_devid) != 0) {
    Serial.println("bad devid");
    return;
  }

  if (strcmp(root["cmd"], "plug_set") == 0) {
    uint8_t st = root["status"];
    plug_set(st);
  }
}

void extMessageReceived(EXTED_CMD cmd, int status, String payload, unsigned int length) {
  Serial.println("em");
}

void init_ethernet() {
  IPAddress ip(192,168,2,183);
  
  Ethernet.begin(mac, ip);
//  Ethernet.begin(mac);

  Serial.print("i:"); 
  Serial.println(Ethernet.localIP());
  Serial.print("s:"); 
  Serial.println(Ethernet.subnetMask());
  Serial.print("g:"); 
  Serial.println(Ethernet.gatewayIP());
  Serial.print("d:"); 
  Serial.println(Ethernet.dnsServerIP());

}

void setup() {

  Serial.begin(57600);
  Serial.println("init..");

  pinMode(PIN_CONTROL, OUTPUT);
  digitalWrite(PIN_CONTROL, HIGH);
  init_ethernet();

  //TODO: if we can't get reg info and tick info
  get_host_v2(g_appkey, url);
  get_ip_port(url, broker_addr, &port);

  setup_with_appkey_and_devid(g_appkey, g_devid);

  client.begin(broker_addr, port, net);
  connect();
  Serial.println("init..ok");
}

void loop() {
  client.loop();

  check_connect();

#if 0
  if (millis() - g_last_check_ms > 20000) {
    g_last_check_ms = millis();
      client.publish(g_topic, "test");
  }
#endif
  delay(100);
}


