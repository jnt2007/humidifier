/*
 Basic ESP8266 MQTT example
 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off
 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

// Update these with values suitable for your network.

const char* ssid = "ssid";
const char* password = "password";
const char* mqtt_server = "192.168.88.221";
const char* mqtt_user = "user";
const char* mqtt_password = "pass";
const char* outTopic = "humidifier/state";
String state;
String new_state;
String target_state;
String free_heap;
bool do_change_state = false;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
long lastPublish = 0;
char msg[50];
int d1;
int d2;
int d3;

void setup_wifi() {

  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.disconnect();
  WiFi.hostname("humidifier");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_ota() {
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String res = "";
  for (int i = 0; i < length; i++) {
    res += String((char)payload[i]);
  }
  Serial.print(res);
  Serial.println();


  if (state != "empty") {
    // Switch state if updated
    if (res != state) {
      if (res == "0" || res == "1" || res == "2" || res == "3") {
        do_change_state = true;
        target_state = res;
      } else {
        do_change_state = false;
        Serial.print("Unknown state received: ");
        Serial.println(res);
      }

    } else {
      do_change_state = false;
    }

  } else {
    do_change_state = false;
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe("humidifier/command");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void report_state() {
  char copy[50];
  state.toCharArray(copy, 50);
  client.publish(outTopic, copy);
}

void read_state() {
  d1 = digitalRead(D5);
  d2 = digitalRead(D6);
  d3 = digitalRead(D7);

  if (d1 == 1 && d2 == 1 && d3 == 1) {
    new_state = "0";
  }
  else if(d1 == 0 && d2 == 1 && d3 == 0) {
    new_state = "1";
  }
  else if(d1 == 1 && d2 == 0 && d3 == 1) {
    new_state = "2";
  }
  else if(d1 == 0 && d2 == 1 && d3 == 1) {
    new_state = "3";
  }
  else if(d1 == 1 && d2 == 1 && d3 == 0) {
    new_state = "empty";
  }
  else {
    new_state = "unknown";
  }
  if (new_state != state) {
    state = new_state;
    report_state();
  }
  // Serial.println(state);
}

void change_state() {
  pinMode(D0, INPUT);
  delay(50);
  pinMode(D0, OUTPUT);
  // digitalWrite(D0, HIGH);
  // delay(50);
  // digitalWrite(D0, LOW);
  // digitalWrite(BUILTIN_LED, LOW);
  // delay(50);
  // digitalWrite(BUILTIN_LED, HIGH);
}

void setup() {
  delay(3000);
  // pinMode(D0, OUTPUT);
  // pinMode(BUILTIN_LED, OUTPUT);
  pinMode(D5, INPUT);
  pinMode(D6, INPUT);
  pinMode(D7, INPUT);
  Serial.begin(115200);
  setup_wifi();
  setup_ota();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  read_state();
  char copy[50];
  state.toCharArray(copy, 50);
  client.publish(outTopic, copy);
}

void loop() {
  ArduinoOTA.handle();

  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000 && do_change_state) {
    lastMsg = now;

    change_state();
    delay(50);
    read_state();

    Serial.print("State changed ");
    Serial.println(state);

    char copy[50];
    state.toCharArray(copy, 50);
    client.publish(outTopic, copy);
  }

  read_state();

  if (now - lastPublish > 30000) {
    lastPublish = now;
    report_state();
  }
  
  if (state == target_state) {
    do_change_state = false;
  }
}
