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

// Update these with values suitable for your network.

const char* ssid = "MikroTik";
const char* password = "Instagram";
const char* mqtt_server = "192.168.88.221";
const char* outTopic = "humidifier/state";
String state;
String target_state;
bool do_change_state = false;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
long lastPublish = 0;
char msg[50];
// int value = 0;
int d1;
int d2;
int d3;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

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

void callback(char* topic, byte* payload, unsigned int length) {
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
      if (res == "off" || res == "0" || res == "1" || res == "2" || res == "3") {
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
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(outTopic, "hello world");
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

void read_state() {
  d1 = digitalRead(D5);
  d2 = digitalRead(D6);
  d3 = digitalRead(D7);

  
  if (d1 == 1 && d2 == 1 && d3 == 1) {
    state = "off";
  }
  else if(d1 == 0 && d2 == 1 && d3 == 0) {
    state = "1";
  }
  else if(d1 == 1 && d2 == 0 && d3 == 1) {
    state = "2";
  }
  else if(d1 == 0 && d2 == 1 && d3 == 1) {
    state = "3";
  }
  else if(d1 == 1 && d2 == 1 && d3 == 0) {
    state = "empty";
  }
  else {
    state = "unknown";
  }
  Serial.println(state);
}

void change_state() {
  digitalWrite(D0, HIGH);
  delay(50);
  digitalWrite(D0, LOW);
  // digitalWrite(BUILTIN_LED, LOW);
  // delay(50);
  // digitalWrite(BUILTIN_LED, HIGH);
}

void setup() {
  pinMode(D0, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(D5, INPUT);
  pinMode(D6, INPUT);
  pinMode(D7, INPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  read_state();
  char copy[50];
  state.toCharArray(copy, 50);
  client.publish(outTopic, copy);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000 && do_change_state) {
    lastMsg = now;

    change_state();

    // snprintf (msg, 50, "hello world #%ld", value);
    Serial.println("State changed ");
    
  }

  read_state();

  if (now - lastPublish > 10000) {
    lastPublish = now;
    char copy[50];
    state.toCharArray(copy, 50);
    client.publish(outTopic, copy);
  }

  
  if (state == target_state) {
    do_change_state = false;
  }
  

  Serial.println();
  delay(1000);
}