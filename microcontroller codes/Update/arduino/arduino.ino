#include <DebouncedLDR.h>
#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>


constexpr static int LDR_PIN = A0; // Define pin for the photosensitive resistor
constexpr static int PumpPin = 22; // Define pin for water pump
constexpr static int DHT_PIN = 2; // Define pin for DHT22 sensor
constexpr static int ONE_WIRE_BUS = 3;  // Define pin for DS18B20 sensor
constexpr static int FloatMain = 51; // Define pin for Float switch main
constexpr static int FloatReserve = 52; // Define pin for Float switch reserved
constexpr static int HeatingElement = 4; // Define pin for Heating Element
constexpr static int Fan = 5; // Define pin for Fan1
constexpr static int Led = 6; // Define pin for LED light


// Initialize the photosensitive resistor
DebouncedLDR ldr(1023, 9, 100);

// Initialize DHT sensor
DHT dht(DHT_PIN, DHT22);

// Initialize DS18B20 sensor
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Structure to hold sensor data
struct SensorData {
  float DHT22TempC;
  float DHT22Humid;
  float DS18B20TempC;
  float LDR;
  bool FloatR;
  bool FloatM;
};

// Structure to hold actuator data
struct ActuatorData {
  int Heating_Element;
  bool LED_Light;
  bool Ultrasonic_Atomizer;
  bool Water_Pump;
  int Cpu_Fan;
};

//Globals
int DHTReading;

// Function prototypes
void readSensors(SensorData &sensor);
void controlActuators(ActuatorData &actuator, const SensorData &sensor, int DHTReading);

void setup() {
  Serial.begin(9600);

  // Actuators
  pinMode(PumpPin, OUTPUT);
  pinMode(FloatMain, OUTPUT);
  pinMode(FloatReserve, OUTPUT);
  pinMode(HeatingElement, OUTPUT);
  pinMode(Fan, OUTPUT);
  pinMode(Led, OUTPUT);
  
  // Sensors
  pinMode(LDR_PIN, INPUT);
  dht.begin();
  sensors.begin();
}

void loop() {
  SensorData sensor;
  ActuatorData actuator;

  readSensors(sensor);
  controlActuators(actuator, sensor, DHTReading);

  // Optional: Print sensor readings for debugging
  // Serial.println("DHT22 Humidity: " + String(sensor.DHT22Humid));
  // Serial.println("DHT22 Temperature (Celsius): " + String(sensor.DHT22TempC));
  // Serial.println("DS18B20 Temperature (Celsius): " + String(sensor.DS18B20TempC));
  // Serial.println("LDR Reading: " + String(sensor.LDR));
  // Serial.println("Float Switch (reserved): " + String(sensor.FloatR));
  // Serial.println("Float Switch (main): " + String(sensor.FloatM));

  Serial.println("StageOneDone");
  delay(1000);
}

void readSensors(SensorData &sensor) {

  // DHT 22 data request
  sensor.DHT22Humid = dht.readHumidity();
  sensor.DHT22TempC = dht.readTemperature();
  
  // DS18B20 data request
  sensors.requestTemperatures();
  sensor.DS18B20TempC = sensors.getTempCByIndex(0);

  // LDR data request
  sensor.LDR = ldr.update(analogRead(LDR_PIN));

  // Float Switch Main data request
  sensor.FloatM = digitalRead(FloatMain);

  // Float Switch Reserved data request;
  sensor.FloatR = digitalRead(FloatReserve);

}
// Time Global
int time = getTime();

void controlActuators(ActuatorData &actuator, const SensorData &sensor, int DHTReading) {
  if(sensor.DHT22TempC < 25 && sensor.DS18B20TempC < 60){
    // HEATING
    actuator.Heating_Element = true;
    actuator.Cpu_Fan = 255;
    delay(100);
    actuator.Cpu_Fan = 150;
    actuator.Ultrasonic_Atomizer = false;
    DHTReading = 1;
  }else if(sensor.DHT22TempC <= 25 && sensor.DHT22TempC >= 35){
    // STABLIZING
    actuator.Heating_Element = false;
    actuator.Cpu_Fan = 200; // AnalogWrite value for medium fan speed
    actuator.Ultrasonic_Atomizer = false;
    DHTReading = 2;
  }else if(sensor.DHT22TempC > 35){
    // COOLING DOWN
    actuator.Heating_Element = false;
    actuator.Cpu_Fan = 255; // AnalogWrite value for maximum fan speed
    actuator.Ultrasonic_Atomizer = true;
    DHTReading = 3;
  }

  
  // Float Switch Main Logic
  if(sensor.FloatM == true && sensor.FloatR == false){
    digitalWrite(PumpPin, HIGH);
    actuator.Water_Pump = true;
  }else{
    digitalWrite(PumpPin, LOW);
    actuator.Water_Pump = false;
  }

  // Float Switch Reserve Logic
  if(sensor.FloatR == true){
    // Serial.println("Water is Low please Refill the tank.");
  }else{
    // Serial.println("Water is enough");
  }


  // Change the LDR comparing value accordingly
  if(sensor.LDR > 400 && time >= 7 && time <= 17 ){
    actuator.LED_Light = true;
  }else{
    digitalWrite(Led, LOW);
    actuator.LED_Light = false;
  }


}

int getTime(){
  if(Serial.available()){

    // Read the incoming JSON string
    String jsonTime = Serial.readStringUntil('/n');

    // Deserialize JSON string to a JSON object
    StaticJsonDocument<100> doc; 
    DeserializationError error = deserializeJson(doc, jsonTime);

    if(!error){
      //Parse the JSON object and extract time
      time = doc["time"];
    }
  }
  return time;
}



void sendData(){
  SensorData sensor;
  ActuatorData actuator;
  
  // Create a JSON object
  StaticJsonDocument<300> jsonData;

  //Populate JSON object with sensor data
  jsonData["DHT22TempC"] = sensor.DHT22TempC;
  jsonData["DHT22Humid"] = sensor.DHT22Humid;
  jsonData["DS18B20TempC"] = sensor.DS18B20TempC;

  //DHT22 remarks
  if(sensor.DHT22TempC > 0){
    jsonData["DHT22State"] = "Working";
  }else{
    jsonData["DHT22State"] = "Not working";
  }

  //DS18B20 remarks 
  if(sensor.DS18B20TempC > 0){
    jsonData["DS18B20State"] = "Working";
  }else{
    jsonData["DS18B20State"] = "Not working";
  }

  //LDR remarks
  if(sensor.LDR > 0){
    jsonData["LDRState"] = "Working";
  }else{
    jsonData["LDRState"] = "Not working";
  }

  // Float Switch RESERVED remarks
  if(sensor.FloatR == true){
    jsonData["FloatRState"] = "OFF";
  }else{
    jsonData["FloatRState"] = "ON";
  }

  // Float Switch MAIN remarks
  if(sensor.FloatM == true){
    jsonData["FloatMState"] = "OFF";
  }else{
    jsonData["FloatMState"] = "ON";
  }

  // Heating Element remarks
  if(sensor.DS18B20TempC > 0 && DHTReading == 1){
    jsonData["HeatingElementState"] = "Heating";
  }else if(sensor.DS18B20TempC > 0 && DHTReading == 2){
    jsonData["HeatingElementState"] = "Stabilizing";
  }else if(sensor.DS18B20TempC > 0 && DHTReading == 3){
    jsonData["HeatingElementState"] = "Cooling";
  }

  // LED_Light remarks
  if(actuator.LED_Light == true){
    jsonData["LEDLightState"] = "ON";
  }else{
    jsonData["LEDLightState"] = "OFF";
  }

  // Ultasonic Atomizer remarks
  if(actuator.Ultrasonic_Atomizer == true){
    jsonData["UltrasonicAtomizerState"] = "ON";
  }else{
    jsonData["UltrasonicAtomizerState"] = "OFF";
  }

  // Water_Pump
  if(actuator.Water_Pump == true){
    jsonData["WaterPumpState"] = "ON";
  }else{
    jsonData["WaterPumpState"] = "OFF";
  }

  // CPU Fan1
  if(actuator.Cpu_Fan > 0 ){
    jsonData["FanState"] = "Working";
  }else{
    jsonData["FanState"] = "Not Working";
  }

  // Serialize JSON to a string
  String jsonString;
  serializeJson(jsonData, jsonString);

  // Print the serialized JSON string
  Serial.println(jsonString);
  Serial.println();
  delay(1000);
}