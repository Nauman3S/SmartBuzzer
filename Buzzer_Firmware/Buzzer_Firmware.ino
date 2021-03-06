/*
 Buzzer Firmware 

 It connects to an MQTT server then:
  - publishes "number_macAddress" to the topic "SmartBuzzer/b" every two seconds
  - subscribes to the topic "SmartBuzzer/config", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "SmartBuzzer/conf" is an 1, switch ON the ESP Led,
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
#include "SoftwareStack.h"

// Update these with values suitable for your network.

const char* ssid = "WiFi Name";
const char* password = "WiFi Password";
const char* mqtt_server = "broker.hivemq.com";
String buttonID="b1";

SoftwareStack ss;
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
String macAddress="";
int pushButton=D6;
int buzzerPin=D7;
int buzzerPlay=0;
int btnCount=0;
void setup_wifi() {

  delay(10);
  pinMode(pushButton,INPUT);
  pinMode(buzzerPin,OUTPUT);
  digitalWrite(buzzerPin,LOW);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  macAddress=String(WiFi.macAddress());

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
  String pl="";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    pl=pl+String(payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
   if(String(topic)==String("SmartBuzzer/config")){
    if(String(pl)==String("reset")){
      btnCount=0;
    }
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "SBuzzer-"+macAddress;
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("SmartBuzzer/config");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}
String val="";
String topicName="SmartBuzzer/device/"+buttonID;

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if(digitalRead(pushButton)==0){
    btnCount++;
    val=String(btnCount);
    client.publish(topicName.c_str(), val.c_str());    
    digitalWrite(buzzerPin, 1);
    buzzerPlay=1;
  }
  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    if(buzzerPlay==1){
      buzzerPlay=2;
    }
    else if(buzzerPlay==2){
      buzzerPlay=0;
      digitalWrite(buzzerPin, 0);//stop the buzzer
    }
    
    lastMsg = now;
    ++value;
    // val=String(value)+String("_")+macAddress;
    // //snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    // Serial.print("Publish message: ");
    // Serial.println(val);
    // client.publish(topicName.c_str(), val.c_str());
  }
}
