#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>  // This library is already built in to the Arduino IDE
#include <LiquidCrystal_I2C.h> //This library you can add via Include Library > Manage Library > 
#include <SPI.h> // MicroSD
#include <SD.h>
#include "RTClib.h"
// PCF
#include "Arduino.h"
#include "PCF8574.h"
PCF8574 pcf8574(0x20,2,0);
LiquidCrystal_I2C lcd(0x27, 16, 2);
// PZEM
#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>
#define USE_SOFTWARE_SERIAL true 
#define PZEM_RX_PIN 5
#define PZEM_TX_PIN 4
#define NUM_PZEMS 3
PZEM004Tv30 pzems[NUM_PZEMS];
SoftwareSerial pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
SoftwareSerial GSMSerial(1, 16); //UNUSED RX to TX 16 (D0)
// //https://www.theengineeringprojects.com/2018/10/introduction-to-nodemcu-v3.html
// const int relay1Pin = 16; //D0
// const int relay2Pin = 5; //D1
// const int relay3Pin = 4; //D2

File myFile;
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

// RTC
RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Update these with values suitable for your network.
const char* ssid = "Reyes_WIFI_4G";
const char* password = "jonmarco11";
const char* mqtt_server = "192.168.254.108";

// GSM
String contactNumber = "+639565309575";

// power data global variables
String nodeName = "Node00001";
double voltage;
double ampere1;
double ampere2;
double ampere3;
double phaseAngle1;
double phaseAngle2;
double phaseAngle3;
double power1;
double power2;
double power3;
bool R1 = false;
bool R2 = false;
bool R3 = false;
String status = "normal";
String controlsubs = "/relaycontrols/" + nodeName;

String prevStatus = "normal";

void sendMessage(String message){
  // GSMSerial.println("AT+CMGF=1");
  // delay(500);
  // GSMSerial.println("AT+CMGS=\"" + contactNumber + "\"");
  // delay(500);
  // GSMSerial.print(message);
  // delay(500);
  // GSMSerial.write(26);
  // delay(500);
}
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connecting to");
  lcd.setCursor(0,1);
  lcd.print(ssid);
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
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
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(WiFi.localIP());

}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Parse the incoming JSON message
  StaticJsonDocument<96> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  if (doc["node"] == nodeName) {
    R1 = doc["r1"];
    if(R1){
      pcf8574.digitalWrite(P0, LOW);
    } else {
      pcf8574.digitalWrite(P0, HIGH);
    }
    Serial.print("relay1 set to: ");
    Serial.println(R1);

    R2 = doc["r2"];
    if(R2){
      pcf8574.digitalWrite(P1, LOW);      
    } else {
      pcf8574.digitalWrite(P1, HIGH); 
    }
    Serial.print("relay2 set to: ");
    Serial.println(R2);

    R3 = doc["r3"];
    if(R3){
      pcf8574.digitalWrite(P2, LOW);     
    } else {
      pcf8574.digitalWrite(P2, HIGH); 
    }
    Serial.print("relay3 set to: ");
    Serial.println(R3);
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
      lcd.setCursor(0,1);
      lcd.print("MQTT Connected");
      // Once connected, publish an announcement...
      // client.publish("powerdata", "hello world");
      // ... and resubscribe
      client.subscribe(controlsubs.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void initializeSD(){

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Initializing SD card...");

  if (!SD.begin(15)) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("initialization failed!");
    return;
  }
  lcd.setCursor(0,1);
  lcd.println("SD is OK");
  delay(3000);
}

void setup() {
  #ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB -- RTC
  #endif

  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  // GSMSerial.begin(9600); // Set GSM Baud at 9600
    // LCD
  Wire.begin(2,0);
  lcd.init();   // initializing the LCD
  lcd.backlight(); // Enable or Turn On the backlight 

  // RTC
  if (! rtc.begin()) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("ERROR:");
    lcd.setCursor(0,1);
    lcd.print("RTC NOT FOUND");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.isrunning()) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("ERROR:");
    lcd.setCursor(0,1);
    lcd.print("RTC ERROR 001");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2023, 04, 23, 0, 49, 0));
  }
  // SD Card
  initializeSD();
  myFile = SD.open("log.txt", FILE_WRITE);
  if (myFile) {
    Serial.print("Writing to log.txt...");
    DateTime now = rtc.now();
    String datetime = String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + " ";
    String message = nodeName + " BOOTUP INITIALIZED" + " at " + datetime;
    myFile.println(message);
    myFile.close();
  } else {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("ERROR:");
    lcd.setCursor(0,1);
    lcd.print("LOGFILE FAIL");
    delay(10000);
    // if the file didn't open, print an error:
  }

  // Wi-Fi  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  StaticJsonDocument<256> doc;
  // pinMode(relay1Pin, LOW);
  // pinMode(relay2Pin, LOW);
  // pinMode(relay3Pin, LOW);

  // For each PZEM, initialize it
    for(int i = 0; i < NUM_PZEMS; i++)
    {
       pzems[i] = PZEM004Tv30(pzemSWSerial, 0x11 + i);
    }

  // SET Relays
  pcf8574.pinMode(P0, OUTPUT);
  pcf8574.pinMode(P1, OUTPUT);
  pcf8574.pinMode(P2, OUTPUT);
  pcf8574.begin();
  //Send SMS
    DateTime now = rtc.now();
    String datetime = String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + " ";
    String message = nodeName + "BOOTUP NOTIFICATION" + " at " + datetime;
    Serial.println("GSM MESSAGE: " + message);
    sendMessage(message);
}

void loadValues(){
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
}

String prepareJSONpayload(float voltage, float ampere1, float ampere2, float ampere3, float phaseAngle1, float phaseAngle2, float phaseAngle3, float power1, float power2, float power3, bool relay1, bool relay2, bool relay3, String status) {
    StaticJsonDocument<384> doc; //https://arduinojson.org/v6/assistant/
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
    doc["r1"] = relay1;
    doc["r2"] = relay2;
    doc["r3"] = relay3;
    // Serial.println(voltage);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("STATUS: ");
    if(isnan(voltage)){
      R1 = false;
      R2 = false;
      R3 = false;
      pcf8574.digitalWrite(P0, HIGH);
      pcf8574.digitalWrite(P1, HIGH);
      pcf8574.digitalWrite(P2, HIGH);
      status = "blackout";
      lcd.print("Blackout");
      doc["status"] = "blackout";
    } else if (voltage < 200){ // Brownout defined when voltage is less than 200V
      R1 = false;
      R2 = false;
      R3 = false;
      pcf8574.digitalWrite(P0, HIGH);
      pcf8574.digitalWrite(P1, HIGH);
      pcf8574.digitalWrite(P2, HIGH);
      status = "brownout";
      lcd.print("Brownout");
      doc["status"] = "brownout";
    } else {
      status = "normal";
      lcd.print("Normal");
      doc["status"] = "normal";
    }

    if(status != prevStatus){
      if (status == "normal"){
        DateTime now = rtc.now();
        String datetime = String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + " ";
        String message = nodeName + " status changed to " + status + " at " + datetime + ". Performing Slow Restore, please wait 10 seconds. Once restored, refresh the page to update.";
        Serial.println("GSM MESSAGE: " + message);
        sendMessage(message);
        for (int i = 0; i < 3; i++) {
          if (i == 0) {
            R1 = true;
            pcf8574.digitalWrite(P0, LOW);
          }
          else if (i == 1) {
            R2 = true;
            pcf8574.digitalWrite(P1, LOW);
          }
          else if (i == 2) {
            R3 = true;
            pcf8574.digitalWrite(P2, LOW);
          }
          delay(3000);
        }
      } else {

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
  if (now - lastMsg > 2500) {
    lastMsg = now;
    loadValues();
    String output = prepareJSONpayload(voltage, ampere1, ampere2, ampere3, phaseAngle1, phaseAngle2, phaseAngle3, power1, power2, power3, R1, R2, R3, status);
    myFile = SD.open("log.txt", FILE_WRITE);
    if (myFile) {
      DateTime now = rtc.now();
      String datetime = String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + " ";
      myFile.print(datetime);
      myFile.println(output);
      Serial.println(output);
      myFile.close();
    } else {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("ERROR:");
      lcd.setCursor(0,1);
      lcd.print("LOG WRITE FAIL");
      // if the file didn't open, print an error:
  }
    client.publish("powerdata", output.c_str());
  }
}