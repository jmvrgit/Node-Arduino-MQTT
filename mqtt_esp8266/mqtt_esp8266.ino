#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Update these with values suitable for your network.

const char* ssid = "Reyes_WIFI_4G";
const char* password = "jonmarco11";
const char* mqtt_server = "192.168.254.108";

int relay1Pin = D3;
int relay2Pin = D4;
int relay3Pin = D5;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

// power data global variables
String nodeName = "Node00000";
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
bool R1 = true;
bool R2 = true;
bool R3 = true;
String status = "normal";
String controlsubs = "/relaycontrols/" + nodeName;
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
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
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  if (doc["nodeName"] == nodeName) {
    R1 = doc["R1"];
    if(R1){
      digitalWrite(relay1Pin, HIGH);
      Serial.println("relay 1 on");        
    } else {
      digitalWrite(relay1Pin, LOW);
      Serial.println("relay 1 off");  
    }
    // Serial.print("relay1 set to: ");
    // Serial.println(R1);

    R2 = doc["R2"];
    if(R2){
      digitalWrite(relay1Pin, HIGH);
      Serial.println("relay 2 on");        
    } else {
      digitalWrite(relay1Pin, LOW);
      Serial.println("relay 2 off");  
    }
    // Serial.print("relay2 set to: ");
    // Serial.println(R2);

    R3 = doc["R3"];
    if(R3){
      digitalWrite(relay1Pin, HIGH);
      Serial.println("relay 3 on");        
    } else {
      digitalWrite(relay1Pin, LOW);
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

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  StaticJsonDocument<256> doc;
}

float randomFloat(float min, float max) {
  float random_value = (float)random() / RAND_MAX * (max - min) + min;
  return round(random_value * 100.0) / 100.0;
}

void loadValues(){
  voltage = randomFloat(220.0, 240.0);
  ampere1 = randomFloat(0.0, 10.0);
  ampere2 = randomFloat(0.0, 10.0);
  ampere3 = randomFloat(0.0, 10.0);
  phaseAngle1 = randomFloat(0.0, 1.0);
  phaseAngle2 = randomFloat(0.0, 1.0);
  phaseAngle3 = randomFloat(0.0, 1.0);
  power1 = randomFloat(0.0, 1000.0);
  power2 = randomFloat(0.0, 1000.0);
  power3 = randomFloat(0.0, 1000.0);
}

String prepareJSONpayload(float voltage, float ampere1, float ampere2, float ampere3, float phaseAngle1, float phaseAngle2, float phaseAngle3, float power1, float power2, float power3, bool relay1, bool relay2, bool relay3, String status) {
    StaticJsonDocument<384> doc; //https://arduinojson.org/v6/assistant/
    doc["nodeName"] = nodeName;
    doc["voltage"] = round(voltage * 100.0) / 100.0;
    doc["ampere1"] = round(ampere1 * 100.0) / 100.0;
    doc["ampere2"] = round(ampere2 * 100.0) / 100.0;
    doc["ampere3"] = round(ampere3 * 100.0) / 100.0;
    doc["phaseAngle1"] = round(phaseAngle1 * 100.0) / 100.0;
    doc["phaseAngle2"] = round(phaseAngle2 * 100.0) / 100.0;
    doc["phaseAngle3"] = round(phaseAngle3 * 100.0) / 100.0;
    doc["power1"] = round(power1 * 100.0) / 100.0;
    doc["power2"] = round(power2 * 100.0) / 100.0;
    doc["power3"] = round(power3 * 100.0) / 100.0;
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
    Serial.print("Publish message: ");
    Serial.println(output);
    client.publish("powerdata", output.c_str());
  }
}
