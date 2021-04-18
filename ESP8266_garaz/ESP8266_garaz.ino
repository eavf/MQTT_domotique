/*
 Basic ESP8266 MQTT example
 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "test" every two seconds
  - subscribes to the topic "test", printing out any messages
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
#include <DallasTemperature.h>
#include "arduino_secrets.h"

// Update these with values suitable for your network.
const char* ssid = SSID_a;
const char* password = PASS;
const char* mqtt_server = "10.255.1.201";

//GPIO kde je teplotný senzor DS18B20 pripojený je
const int oneWireBus = 4;
short int Do_monitor = 1;         //0 - does not monitor state / 1 - monitor state
const short int Gar_no_cid = 1;   //Nomber of hall sensors on garage door

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

//Hall senzor je na A0 alebo D7
int analogPin = A0;
int HallPin = 13;
int G_open = 1;     //Terba zadefinovať PIN, ktorý bude spínať gar. spínač

//Knižnice pre WiFi a MQTT
WiFiClient espClient;
PubSubClient client(espClient);

//Premenné pre riadenie
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
long lastReconnectAttempt = 0;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  delay(250);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.printf("Connection status: %d\n", WiFi.status());
    WiFi.printDiag(Serial);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  int state, command;

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    state = i;
  }
  Serial.println();
  Serial.print("Stav monitorovania v callback je :");
  Serial.println(Do_monitor);

  //Open garage door
  if ((char)payload[state] == '1') {
    Serial.print("State is : ");
    Serial.print((char)payload[state]);
    Serial.println();
    //First test where garage doors are positionned

    //Open garage door if closed
    //Instead light up built-in LED
    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on - note that LOW is the voltage level but actually the LED is on;
  } else {
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
    Serial.print("State is : ");
    Serial.print((char)payload[state]);
    Serial.println();
  }
  
  if ((char)payload[state] == '2') {
    Do_monitor = 1;         //0 - does not monitor state / 1 - monitor state
  } 
  if ((char)payload[state] == '3') {
    Do_monitor = 0;         //0 - does not monitor state / 1 - monitor state
  }
}


boolean monitoring(int k) {
  int i;
  for (i = 0; i < Gar_no_cid; i++) {     //Gar_no_cid = počet hall čidiel na garažovej bráne
    int digitalVal = digitalRead(HallPin);
    snprintf (msg, MSG_BUFFER_SIZE, "#%ld, g_brana, čidlo : %ld, hodnota: %ld", k, i, digitalVal);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("test", msg);
  }
}

bool matchTemplate ( const char secretWord[], const char guessTemplate[] ) {
  //  will return true if secretWord is part of the guessTemplate

  for ( int i = 0; i < MSG_BUFFER_SIZE; i++ ) {
    if ( secretWord[i] == guessTemplate[i] )
      return true;
    else
      return false;
    }
  }



boolean reconnect() {
  //Non-blocking verzia
  // Loop until we're reconnected
  String clientId = "ESP8266C-Garage";
  // Attempt to connect
  Serial.print("Attempting MQTT connection...");
  if (client.connect(clientId.c_str())) {
    Serial.println("connected");
    // Once connected, publish an announcement...
    client.publish("test", "Pripojil som sa...");
    // ... and resubscribe
    client.subscribe("test");
  }
  return client.connected();
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(HallPin, INPUT);          // Citanie hall senzora

  Serial.println("WiFi connecting");
  Serial.begin(115200);

  setup_wifi();

  //Setup pre MQTT clienta
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Start the DS18B20 sensor
  sensors.begin();
  lastReconnectAttempt = 0;
}

void loop()
//Non-blocking verzia
{
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected
    // Udržovať MQTT v činnosti...
    client.loop();

    //Pravidelné publikovanie informácii o garáži
    unsigned long now = millis();
    if (now - lastMsg > 10000) {
      lastMsg = now;
      ++value;

      //Digital Hall senzor
      //int digitalVal = digitalRead(HallPin);
      //Serial.println(digitalVal);

      //Teplota
      /*
      sensors.requestTemperatures();
      float temperatureC = sensors.getTempCByIndex(0);
      */

      //Teraz zostaviť MQQT spravu

      //Teplota
      /*
      snprintf (msg, MSG_BUFFER_SIZE, "#%ld,g_teplota,#%ld", value, temperatureC);
      Serial.print("Publish message: ");
      Serial.println(msg);
      client.publish("test", msg);
      */
      //Brána
      //Najprv, ak chceme vykonávať monitoring... tak
      if (Do_monitor == 1) monitoring(value);
    }
  }
}
