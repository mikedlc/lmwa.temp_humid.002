/***************************************************************************************************************************/
/*
    A base program template that has the following features:
      1.3" OLED display
      Wifi
      OTA Updating
      MQTT
      Uptime Counter

    Auther:     Various
    Updated:    28th Aug 2024
*/
/***************************************************************************************************************************/

//Device Information
const char* ProgramID = "LMWA.env.01";
const char* SensorType = "Environment";
const char* mqtt_topic = "temperature/01";
const char* mqtt_unit = "F";
const char* mqtt_server_init = "192.168.12.165";
const char* mqtt_user = "mqttuser";
const char* mqtt_password = "Lafayette123!";

//OTA Stuff
#include <ArduinoOTA.h>

//Wifi Stuff
//#include <WiFi.h> // Uncomment for ESP32
#include <ESP8266WiFi.h> // Uncomment for D1 Mini ESP8266
#include <ESP8266mDNS.h> // Uncomment for D1 mini ES8266
#include <WiFiUdp.h> // Uncomment for D1 Mini ESP8266
const char *ssid =	"LMWA-PumpHouse";		// cannot be longer than 32 characters!
const char *password =	"ds42396xcr5";		//
//const char *ssid =	"WiFiFoFum";		// cannot be longer than 32 characters!
//const char *password =	"6316EarlyGlow";		//
WiFiClient wifi_client;
String wifistatustoprint;
void printWifiStatus();

//For 1.3in displays
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Timing
unsigned long currentMillis = 0;
int uptimeSeconds = 0;
int uptimeDays;
int uptimeHours;
int secsRemaining;
int uptimeMinutes;
char uptimeTotal[30];

//MQTT Stuff
#include <PubSubClient.h>
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void sendMQTT(double mqtt_payload);
const char* mqtt_server = mqtt_server_init;  //Your network's MQTT server (usually same IP address as Home Assistant server)
PubSubClient pubsub_client(wifi_client);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


// ********* Put your program's custom stuff below here ********** //

//Set up DHT22 Temp Sensor
#include "DHT.h"
#define DHTPIN 0
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);
int counter = 1;
char farenheitout[32];
String farenheittoprint;
char humidityout[32];
String humiditytoprint;

// ********* Put your program's custom stuff above here ********** //







void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("\n\nBooting");
  Serial.println(__FILE__);
  
  //1.3" OLED Setup
  delay(250); // wait for the OLED to power up
  display.begin(i2c_Address, true); // Address 0x3C default
  display.display(); //Turn on
  delay(2000);

  // Clear the buffer & start drawing
  display.clearDisplay(); // Clear display
  display.setTextColor(SH110X_WHITE);
  display.drawPixel(64, 64, SH110X_WHITE); // draw a single pixel
  display.display();   // Show the display buffer on the hardware.
  delay(2000); // Wait a couple
  display.clearDisplay(); // Clear display

  //Init DHT22
  dht.begin();


  //Wifi Setup Stuff
  WiFi.mode(WIFI_STA);
  if (WiFi.status() != WL_CONNECTED) {
    
    //Write wifi connection to display
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println("Booting Program ID:");
    display.println(ProgramID);
    display.println("Sensor Type:");
    display.println(SensorType);
    display.println("Connecting To WiFi:");
    display.println(ssid);
    display.println("\nWait for it......");
    display.display();

    //write wifi connection to serial
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, password);
    WiFi.setHostname(ProgramID);

    //delay 8 seconds for effect
    delay(8000);

    if (WiFi.waitForConnectResult() != WL_CONNECTED){
      Serial.println("WIFI NOT CONNECTED");
      return;
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Booting Program ID:");
    display.println(ProgramID);
    display.println("Sensor Type:");
    display.println(SensorType);
    display.println("Connected To WiFi:");
    display.println(ssid);
    display.println(WiFi.localIP());
    display.display();
    delay(5000);
    Serial.println("\n\nWiFi Connected! ");
  //  printWifiStatus();

  }

  //OTA Setup Stuff
  if(1){
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    display.clearDisplay();
    Serial.println("Start OTA");
    display.setCursor(0, 0);
    display.println("Starting OTA!");
    display.display();
    });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd OTA - Rebooting!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("OTA Done!"); display.println("Rebooting!");
    display.display();
    ESP.restart();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Progress: " + (progress / (total / 100)));
    display.display();
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

    //Start OTA
  ArduinoOTA.begin();
  Serial.println("OTA Listenerer Started");

  }//End OTA Code Wrapper

  //MQTT Setup
  pubsub_client.setServer(mqtt_server, 1883);
  pubsub_client.setCallback(callback);


  //Report done booting
  Serial.println("Ready");
  Serial.print("Hostname: ");
  Serial.println(WiFi.getHostname());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  delay(5000);

}

void loop() {

  ArduinoOTA.handle(); // Start listening for OTA Updates

  //Calculate Uptime
  currentMillis = millis();
  uptimeSeconds=currentMillis/1000;
  uptimeHours= uptimeSeconds/3600;
  uptimeDays=uptimeHours/24;
  secsRemaining=uptimeSeconds%3600;
  uptimeMinutes=secsRemaining/60;
  uptimeSeconds=secsRemaining%60;
  sprintf(uptimeTotal,"Uptime %02dD:%02d:%02d:%02d",uptimeDays,uptimeHours,uptimeMinutes,uptimeSeconds);


  // *************** Put your program below here *********************//

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.print("ProgramID:"); display.println(ProgramID);
    display.println("Connected to WiFi:");
    display.println(ssid);
    display.print("Hostname: "); display.println(WiFi.getHostname());
    display.print("IP: "); display.println(WiFi.localIP());
    display.println("DHT Sensor Error!");
    display.display();

    //delay 2 seconds for effect
    delay(2000);
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));

  //Convert floats to strings
  dtostrf(f, 8, 2, farenheitout);
  dtostrf(h, 8, 2, humidityout);
  farenheittoprint = "Temperature:" + String(farenheitout) + "F";
  humiditytoprint = "Humidity:   " + String(humidityout) + "%";


  // *************** Put your program above here *********************//


  //buffer & print next display payload
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Sensor: "); display.println(SensorType);
  display.print("Prog.ID: "); display.println(ProgramID);
  display.println(farenheittoprint);
  display.println(humiditytoprint);
  display.print("Hostname: "); display.println(WiFi.getHostname());
  display.print("IP: "); display.println(WiFi.localIP());
  display.print(uptimeTotal);
  display.display();

  sendMQTT(f); //Update MQTT

  //wait 5 before next cycle
  delay(1000);
}



void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.print("Hostname: ");
  Serial.println(WiFi.getHostname());

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  Serial.println("");
}

//MQTT Callback
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

}

//connect MQTT if not
void reconnect() {
  // Loop until we're reconnected
  while (!pubsub_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random pubsub_client ID
    String clientId = "PUMPSENSOR-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (pubsub_client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(pubsub_client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void sendMQTT(double mqtt_payload) {

  if (!pubsub_client.connected()) {
    reconnect();
  }

  unsigned long now = millis();
  if (now - lastMsg > 300000) {//send every 5 minutes
    lastMsg = now;
    ++value;

    Serial.println("\nSending alert via MQTT...");
    Serial.print("Topic: "); Serial.print(mqtt_topic); Serial.print(" Payload: "); Serial.print(mqtt_payload); Serial.print(" Unit: "); Serial.println(mqtt_unit);

    //msg variable contains JSON string to send to MQTT server
    //snprintf (msg, MSG_BUFFER_SIZE, "\{\"amps\": %4.1f, \"humidity\": %4.1f\}", temperature, humidity);
    snprintf (msg, MSG_BUFFER_SIZE, "{\"%s\": %4.2f}", mqtt_unit, mqtt_payload);

    Serial.print("Publishing message: "); Serial.println(msg);
    pubsub_client.publish(mqtt_topic, msg);
  }

}