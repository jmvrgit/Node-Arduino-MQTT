#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>  // This library is already built in to the Arduino IDE
#include <LiquidCrystal_I2C.h> //This library you can add via Include Library > Manage Library > 
#include <SPI.h> // MicroSD
#include <SD.h>
#include "RTClib.h"
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

  if (doc["nodeName"] == nodeName) {
    R1 = doc["R1"];
    if(R1){
      // pinMode(relay1Pin, HIGH);
      Serial.println("relay 1 on");
    } else {
      // pinMode(relay1Pin, LOW);
      Serial.println("relay 1 off");
    }
    // Serial.print("relay1 set to: ");
    // Serial.println(R1);

    R2 = doc["R2"];
    if(R2){
      // pinMode(relay2Pin, HIGH);
      Serial.println("relay 2 on");        
    } else {
      // pinMode(relay2Pin, LOW);
      Serial.println("relay 2 off");  
    }
    // Serial.print("relay2 set to: ");
    // Serial.println(R2);

    R3 = doc["R3"];
    if(R3){
      // pinMode(relay3Pin, HIGH);
      Serial.println("relay 3 on");        
    } else {
      // pinMode(relay3Pin, LOW);
      Serial.println("relay 3 off");  
    }
    // Serial.print("relay3 set to: ");
    // Serial.println(R3);
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

    // LCD
  Wire.begin(2,0);
  lcd.init();   // initializing the LCD
  lcd.backlight(); // Enable or Turn On the backlight 

  // RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
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
    myFile.println("DATE -- BOOT UP INITIALIZED -- ");
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
}

// float randomFloat(float min, float max) {
//   float random_value = (float)random() / RAND_MAX * (max - min) + min;
//   return round(random_value * 100.0) / 100.0;
// }

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
    doc["V"] = round(voltage * 100.0) / 100.0;
    doc["A1"] = round(ampere1 * 100.0) / 100.0;
    doc["A2"] = round(ampere2 * 100.0) / 100.0;
    doc["A3"] = round(ampere3 * 100.0) / 100.0;
    doc["PF1"] = round(phaseAngle1 * 100.0) / 100.0;
    doc["PF2"] = round(phaseAngle2 * 100.0) / 100.0;
    doc["PF3"] = round(phaseAngle3 * 100.0) / 100.0;
    doc["W1"] = round(power1 * 100.0) / 100.0;
    doc["W2"] = round(power2 * 100.0) / 100.0;
    doc["W3"] = round(power3 * 100.0) / 100.0;
    doc["R1"] = relay1;
    doc["R2"] = relay2;
    doc["R3"] = relay3;
    doc["status"] = status;
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
  if (now - lastMsg > 1000) {
    lastMsg = now;
    loadValues();
    String output = prepareJSONpayload(voltage, ampere1, ampere2, ampere3, phaseAngle1, phaseAngle2, phaseAngle3, power1, power2, power3, R1, R2, R3, status);
    // Serial.print("Publish message: ");
    myFile.println(output);
    myFile = SD.open("log.txt", FILE_WRITE);
    if (myFile) {
      DateTime now = rtc.now();
      myFile.print(now.year(), DEC);
      myFile.print('/');
      myFile.print(now.month(), DEC);
      myFile.print('/');
      myFile.print(now.day(), DEC);
      myFile.print(' ');
      myFile.print(now.hour(), DEC);
      myFile.print(':');
      myFile.print(now.minute(), DEC);
      myFile.print(':');
      myFile.print(now.second(), DEC);
      // Serial.println(output);
      myFile.println(output);
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