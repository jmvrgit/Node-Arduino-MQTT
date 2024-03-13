#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>               // This library is already built in to the Arduino IDE
#include <LiquidCrystal_I2C.h>  //This library you can add via Include Library > Manage Library >
#include <SPI.h>                // MicroSD
#include <SD.h>
#include "RTClib.h"
// PCF
#include "Arduino.h"
#include "PCF8574.h"
PCF8574 pcf8574(0x20, 2, 0);
LiquidCrystal_I2C lcd(0x27, 16, 2);
// PZEM
#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>
#define USE_SOFTWARE_SERIAL true
#define PZEM_RX_PIN 5
#define PZEM_TX_PIN 4
#define NUM_PZEMS 3
#define MQTT_DELAY 3000
PZEM004Tv30 pzems[NUM_PZEMS];
SoftwareSerial pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
// SoftwareSerial GSMSerial(1, 16);  //UNUSED RX to TX 16 (D0)
// //https://www.theengineeringprojects.com/2018/10/introduction-to-nodemcu-v3.html

File myFile;
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

// RTC
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

// Update these with values suitable for your network.
const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";

// GSM
String contactNumber = "";

// power data global variables
String nodeName = "";
double voltage;
double ampere1;
double ampere2;
double ampere3;
double ampere1sort;
double ampere2sort;
double ampere3sort;
double phaseAngle1;
double phaseAngle2;
double phaseAngle3;
double power1;
double power2;
double power3;
double energy1;
double energy2;
double energy3;
bool R1 = false;
bool R2 = false;
bool R3 = false;
String status = "normal";
String controlsubs = "";
String prevStatus = "normal";

void sendMessage(String message) {
  Serial.println("AT+CMGF=1");
  delay(500);
  Serial.println("AT+CMGS=\"" + contactNumber + "\"");
  delay(500);
  Serial.print(message);
  delay(500);
  Serial.write(26);
  delay(500);
}

String getDate(DateTime now) {
  char dateNow[20]; // Ensure this is large enough to hold the resulting string
  sprintf(dateNow, "%04d/%02d/%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  return String(dateNow);
}

bool loadConfig(const char* filename, const char*& ssid, const char*& password, const char*& mqtt_server, String& contactNumber, String& nodeName) {
  File configFile = SD.open(filename);
  while (!configFile) {
    // Serial.println("Error: config.conf missing");
  }

  StaticJsonDocument<384> doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    // Serial.println("Error: Failed to read config.conf");
    return false;
  }

  ssid = strdup(doc["ssid"].as<String>().c_str());
  password = strdup(doc["password"].as<String>().c_str());
  mqtt_server = strdup(doc["mqtt_server"].as<String>().c_str());
  contactNumber = doc["contactNumber"].as<String>();
  nodeName = doc["nodeName"].as<String>();
  return true;
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  // Serial.println();
  // Serial.print("Connecting to ");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  // Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.print(".");
  }

  randomSeed(micros());

  // Serial.println("");
  // Serial.println("WiFi connected");
  // Serial.println("IP address: ");
  // Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP Address:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(250);
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Serial.print("Message arrived [");
  // Serial.print(topic);
  // Serial.print("] ");
  // for (int i = 0; i < length; i++) {
  //   Serial.print((char)payload[i]);
  // }
  // Serial.println();

  // Parse the incoming JSON message
  StaticJsonDocument<96> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    // Serial.print(F("deserializeJson() failed: "));
    // Serial.println(error.c_str());
    return;
  }

  if (doc["node"] == nodeName) {
    R1 = doc["r1"];
    if (R1) {
      pcf8574.digitalWrite(P0, LOW);
    } else {
      pcf8574.digitalWrite(P0, HIGH);
    }
    // Serial.print("relay1 set to: ");
    // Serial.println(R1);

    R2 = doc["r2"];
    if (R2) {
      pcf8574.digitalWrite(P1, LOW);
    } else {
      pcf8574.digitalWrite(P1, HIGH);
    }
    // Serial.print("relay2 set to: ");
    // Serial.println(R2);

    R3 = doc["r3"];
    if (R3) {
      pcf8574.digitalWrite(P2, LOW);
    } else {
      pcf8574.digitalWrite(P2, HIGH);
    }
    // Serial.print("relay3 set to: ");
    // Serial.println(R3);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(mqtt_server);
    if (client.connect(clientId.c_str())) {
      // Serial.println("connected");
      lcd.setCursor(0, 1);
      lcd.print("MQTT Connected");
      // Once connected, publish an announcement...
      // client.publish("powerdata", "hello world");
      // ... and resubscribe
      client.subscribe(controlsubs.c_str());
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(status);
      lcd.setCursor(0, 1);
      lcd.print(nodeName);
    } else {
      // Serial.print("failed, rc=");
      // Serial.print(client.state());
      // Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void initializeSD() {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing SD card...");

  if (!SD.begin(15)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("initialization failed!");
    return;
  }
  lcd.setCursor(0, 1);
  lcd.println("SD is OK");
  delay(3000);
}

void setup() {
#ifndef ESP8266
  while (!Serial)
    ;  // wait for serial port to connect. Needed for native USB -- RTC
#endif

  pinMode(BUILTIN_LED, OUTPUT);  // Initialize the BUILTIN_LED pin as an output
  // Serial.begin(115200);
  Serial.begin(9600);  // Set GSM Baud at 9600
    // LCD
  Wire.begin(2, 0);
  lcd.init();       // initializing the LCD
  lcd.backlight();  // Enable or Turn On the backlight

  // RTC
  if (!rtc.begin()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERROR:");
    lcd.setCursor(0, 1);
    lcd.print("RTC NOT FOUND");
    // Serial.flush();
    while (1) delay(10);
  }

  if (!rtc.lostPower()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERROR:");
    lcd.setCursor(0, 1);
    lcd.print("RTC ERROR 001");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2023, 04, 23, 0, 49, 0));
  }

  // SD Card
  initializeSD();
  myFile = SD.open("log.txt", FILE_WRITE);
  if (myFile) {
    // Serial.print("Writing to log.txt...");
    DateTime now = rtc.now();
    String datetime = getDate(now);
    String message = nodeName + " BOOTUP INITIALIZED" + " at " + datetime;
    myFile.println(message);
    myFile.close();
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERROR:");
    lcd.setCursor(0, 1);
    lcd.print("LOGFILE FAIL");
    delay(10000);
    // if the file didn't open, print an error:
  }
  //config

  if (loadConfig("config.conf", ssid, password, mqtt_server, contactNumber, nodeName)) {
    // Serial.println("Configuration loaded from config.conf:");
    // Serial.print("SSID: ");
    // Serial.println(ssid);
    // Serial.print("Password: ");
    // Serial.println(password);
    // Serial.print("MQTT Server: ");
    // Serial.println(mqtt_server);
    // Serial.print("Contact Number: ");
    // Serial.println(contactNumber);
    // Serial.print("Node Name: ");
    // Serial.println(nodeName);

    controlsubs = "/relaycontrols/" + nodeName;
  } else {
    // Serial.println("Error: Failed to load configuration");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERROR:");
    lcd.setCursor(0, 1);
    lcd.print("LOAD CONFIG FAIL");
    delay(10000);
  }

  // Wi-Fi
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  StaticJsonDocument<256> doc;

  // For each PZEM, initialize it
  for (int i = 0; i < NUM_PZEMS; i++) {
    pzems[i] = PZEM004Tv30(pzemSWSerial, 0x11 + i);
  }

  // SET Relays
  pcf8574.pinMode(P0, OUTPUT);
  pcf8574.pinMode(P1, OUTPUT);
  pcf8574.pinMode(P2, OUTPUT);
  pcf8574.begin();
  //Send SMS
  DateTime now = rtc.now();
  String datetime = getDate(now);
  String message = nodeName + " BOOTUP NOTIFICATION" + " at " + datetime;
  // Serial.println("GSM MESSAGE: " + message);
  sendMessage(message);
}

void loadValues() {
  voltage = pzems[0].voltage();
  ampere1 = pzems[0].current();
  ampere2 = pzems[1].current();
  ampere3 = pzems[2].current();
  phaseAngle1 = pzems[0].pf();
  phaseAngle2 = pzems[1].pf();
  phaseAngle3 = pzems[2].pf();
  power1 = pzems[0].power();
  power2 = pzems[1].power();
  power3 = pzems[2].power();
  energy1 = pzems[0].energy();
  energy2 = pzems[1].energy();
  energy3 = pzems[2].energy();
}

String prepareJSONpayload(float voltage, float ampere1, float ampere2, float ampere3, float phaseAngle1, float phaseAngle2, float phaseAngle3, float power1, float power2, float power3, float energy1, float energy2, float energy3, bool relay1, bool relay2, bool relay3, String status) {
  StaticJsonDocument<384> doc;  //https://arduinojson.org/v6/assistant/
  doc["node"] = nodeName;
  doc["v"] = round(voltage * 100.0) / 100.0;
  doc["a1"] = round(ampere1 * 100.0) / 100.0;
  doc["a2"] = round(ampere2 * 100.0) / 100.0;
  doc["a3"] = round(ampere3 * 100.0) / 100.0;
  doc["pf1"] = round(phaseAngle1 * 100.0) / 100.0;
  doc["pf2"] = round(phaseAngle2 * 100.0) / 100.0;
  doc["pf3"] = round(phaseAngle3 * 100.0) / 100.0;
  doc["w1"] = round(power1 * 100.0) / 100.0;
  doc["w2"] = round(power2 * 100.0) / 100.0;
  doc["w3"] = round(power3 * 100.0) / 100.0;
  doc["e1"] = round(energy1 * 100.0) / 100.0;
  doc["e2"] = round(energy2 * 100.0) / 100.0;
  doc["e3"] = round(energy3 * 100.0) / 100.0;
  doc["r1"] = relay1;
  doc["r2"] = relay2;
  doc["r3"] = relay3;
  // Serial.println(voltage);

  if (R1 && !isnan(voltage)) {
    ampere1sort = ampere1;
  }
  if (R2 && !isnan(voltage)) {
    ampere2sort = ampere2;
  }
  if (R3 && !isnan(voltage)) {
    ampere3sort = ampere3;
  }
  // add if R1, r2 r3 is on read amp1
  float amperes[] = { ampere1sort, ampere2sort, ampere3sort };
  int order[] = { 0, 1, 2 };
  for (int i = 0; i < 3; ++i) {
    for (int j = i + 1; j < 3; ++j) {
      if (amperes[i] > amperes[j]) {
        std::swap(amperes[i], amperes[j]);
        std::swap(order[i], order[j]);
      }
    }
  }
  // for (int i = 0; i < 3; ++i) {
  //   Serial.print(order[i]);
  //   Serial.print(" - ");
  //   Serial.println(amperes[i]);
  // }

  if (isnan(voltage)) {
    R1 = false;
    R2 = false;
    R3 = false;
    pcf8574.digitalWrite(P0, HIGH);
    pcf8574.digitalWrite(P1, HIGH);
    pcf8574.digitalWrite(P2, HIGH);
    status = "blackout";
    doc["status"] = "blackout";
  } else if (voltage < 200) {  // Brownout defined when voltage is less than 200V
    R1 = false;
    R2 = false;
    R3 = false;
    pcf8574.digitalWrite(P0, HIGH);
    pcf8574.digitalWrite(P1, HIGH);
    pcf8574.digitalWrite(P2, HIGH);
    status = "brownout";
    doc["status"] = "brownout";
  } else {
    status = "normal";
    doc["status"] = "normal";
  }
  if (status != prevStatus) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(status);
    lcd.setCursor(0, 1);
    lcd.print(nodeName);
    DateTime now = rtc.now();
    String datetime = getDate(now);
    String message = "";
    if (status == "normal") {
      message = nodeName + " status changed to " + status + " at " + datetime + ". Slow Restoration is in progress.";
      sendMessage(message);
      // Serial.println("GSM MESSAGE: " + message);

      for (int i = 0; i < 3; i++) {
        if (order[i] == 0) {
          R1 = true;
          pcf8574.digitalWrite(P0, LOW);
        } else if (order[i] == 1) {
          R2 = true;
          pcf8574.digitalWrite(P1, LOW);
        } else if (order[i] == 2) {
          R3 = true;
          pcf8574.digitalWrite(P2, LOW);
        }
        delay(1500);
      }

    } else {
      message = nodeName + " status changed to " + status + " at " + datetime + ".";
      sendMessage(message);
      // Serial.println("GSM MESSAGE: " + message);
    }

    prevStatus = status;
  }
  String output;
  serializeJson(doc, output);
  return output;
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  unsigned long now = millis();
  if (now - lastMsg > MQTT_DELAY) {
    lastMsg = now;
    loadValues();
    String output = prepareJSONpayload(voltage, ampere1, ampere2, ampere3, phaseAngle1, phaseAngle2, phaseAngle3, power1, power2, power3, energy1, energy2, energy3, R1, R2, R3, status);
    myFile = SD.open("log.txt", FILE_WRITE);
    if (myFile) {
      DateTime now = rtc.now();
      String datetime = getDate(now);
      myFile.print(datetime);
      myFile.println(output);
      // Serial.println(output);
      myFile.close();
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ERROR:");
      lcd.setCursor(0, 1);
      lcd.print("LOG WRITE FAIL");
      // if the file didn't open, print an error:
    }
    String fullTopic = "powerdata/" + nodeName;  // Concatenate the base topic with the node name
    client.publish(fullTopic.c_str(), output.c_str());
    client.publish("powerdata", output.c_str());
  }
}