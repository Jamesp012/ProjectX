#include <DebouncedLDR.h>
#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>


// PINS
// 1. DHT =  2
// 2. DS18B20 =  3
// 3. PHOTOSENSITIVE RESISTOR = A0
// 4. WATER PUMP = 22
// 5. FLOAT SWITCH (MAIN) = 51
// 6. FLOAT SWITHC (RESERVE) = 52
// 7. HEATING ELEMENT = 4
// 8. FAN 1 = 5
// 9. FAN 2 = 6
// 10. LED 1 = 7
// 11. LED 2 = 8

#define DHTTYPE DHT22

// Define pin for the photosensitive resistor
constexpr static int LDR_PIN = A0;

// Define pin for water pump
constexpr static int PUMP_PIN = 22;

// Define pin for DHT22 sensor
constexpr static int DHT_PIN = 2;

// Define pin for DS18B20 sensor
constexpr static int ONE_WIRE_BUS = 3;
// Lower resolution
#define TEMPERATURE_PRECISION 9 

// Initialize the photosensitive resistor
DebouncedLDR ldr(1023, 9, 100);

// Initialize DHT sensor
DHT dht(DHT_PIN, DHTTYPE);

// Initialize DS18B20 sensor
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Variable to store the number of DS18B20 devices found
int numberOfDevices;

// Variable to store the address of DS18B20 sensor
DeviceAddress tempDeviceAddress;

// Structure to hold sensor data
struct sensorData {
  float DHT22TempC;
  float DHT22TempF;
  float DHT22Humid;
  float DS18B20TempC;
  float LDR;
  bool FloatR;
  bool FloatM;
};

struct actuatorData{
  int Heating_Element;
  bool LED_Light;
  bool Ultrasonic_Atomizer;
  bool Water_Pump;
  int Cpu_Fan1;
  int Cpu_Fan2;
};



void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // Set pin modes
  pinMode(LDR_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(POWER_PIN, INPUT);

  // Begin DHT sensor
  dht.begin();

  // Begin DS18B20 sensor
  sensors.begin();

  // Get the number of DS18B20 devices connected
  numberOfDevices = sensors.getDeviceCount();

  // Set resolution for DS18B20 devices
  for (int i = 0; i < numberOfDevices; i++) {
    if (sensors.getAddress(tempDeviceAddress, i)) {
      sensors.setResolution(tempDeviceAddress, 9);
    }
  }
}

void loop() {

  // Create an instance of sensorData struct to store sensor readings
  sensorData sensor;
  actuatorData actuator;

  // Read data from sensors and update the struct
  // DHT
  int DHTReading;

  if(sensor.DHT22TempC < 25){
    int DHTReading = 1;
  }else if(sensor.DHT22TempC <= 25 && sensor.DHT22TempC >= 35){
    int DHTReading = 2;
  }else if(sensor.DHT22TempC > 35){
    int DHTReading = 3; 
  }

  // DHT
  readDHT22(sensor);

  // DS18B20
  readDS18B20(sensor);

  //PHOTOSENSITIVE RESISTOR / LDR
  readPhotoResistor(sensor);

  // Print sensor readings
  Serial.println("DHT22 Humidity: " + String(sensor.DHT22Humid));
  Serial.println("DHT22 Temperature (Celsius): " + String(sensor.DHT22TempC));
  Serial.println("DS18B20 Temperature (Celsius): " + String(sensor.DS18B20TempC)); // Represent te Temperature of heating element
  Serial.println("LDR Reading: " + String(sensor.LDR));
  Serial.println("Float Switch (reserved): " + String(sensor.FloatR));
  Serial.println("Float Switch (main): " + String(sensor.FloatM));


  // Delay for stability
  delay(1000);
}

// Global DHT Logic
  int tempControl;
// Function to read data from DHT22 sensor and update sensorData struct
void readDHT22(sensorData &sensor) {
  sensor.DHT22Humid = dht.readHumidity();
  sensor.DHT22TempC = dht.readTemperature();
  sensor.DHT22TempF = dht.readTemperature(true);

  if(sensor.DHT22TempC < 25 && sensor.DS18B20TempC < 60){
    int tempControl = 1;
  }else if(sensor.DHT22TempC <= 25 && sensor.DHT22TempC >= 35){
    int tempControl = 2;
  }else if(sensor.DHT22TempC > 35){
    int tempControl = 3; 
  }
}

// Function to control the FAN, ULTRASONIC ATOMIZER, and HEATING ELEMENT
void controlCpuFan(actuatorData &actuator) {
  switch(tempControl) {
    case 1:
      // HEATING
      actuator.Heating_Element = 1;
      actuator.Cpu_Fan1 = 255;
      actuator.Cpu_Fan2 = 255;
      delay(100);
      actuator.Cpu_Fan1 = 150;
      actuator.Cpu_Fan2 = 150;
      actuator.Ultrasonic_Atomizer = 0;
      break;
    case 2:
      // STABLIZING
      actuator.Heating_Element = 0;
      actuator.Cpu_Fan1 = 200; // AnalogWrite value for medium fan speed
      actuator.Cpu_Fan2 = 200;
      actuator.Ultrasonic_Atomizer = 0;
      break;
    case 3:
      // COOLING DOWN
      actuator.Heating_Element = 0;
      actuator.Cpu_Fan1 = 255; // AnalogWrite value for maximum fan speed
      actuator.Cpu_Fan2 = 255;
      actuator.Ultrasonic_Atomizer = 1;
      break;
    default:
      // Handle invalid DHTReading
      break;
  }
}

// Function to read data from DS18B20 sensor and update sensorData struct
void readDS18B20(sensorData &sensor) {
  for (int i = 0; i  < numberOfDevices; i++) {
    if (sensors.getAddress(tempDeviceAddress, i)) {
      sensor.DS18B20TempC = sensors.getTempC(tempDeviceAddress);
    }
  }
}

// Function to read data from photosensitive resistor and update sensorData struct
void readPhotoResistor(sensorData &sensor) {
  sensor.LDR = ldr.update(analogRead(LDR_PIN));
  // need ng timer
  // may timer na di ko lang madiskartehan
}

// Control the water pump based on the desired state
void controlWaterPumpM(actuatorData &data, bool state) {
  digitalWrite(data.Water_Pump, state ? HIGH : LOW);
}

// Control the Led Light 
void controlLedLight(actuatorData &data, bool state) { 
  digitalWrite(data.LED_Light, state ? HIGH : LOW);
}

// Control the Heating Element 
void controlHeatingElement(actuatorData &data, bool state) {
  digitalWrite(data.LED_Light, state ? HIGH : LOW);
}

// Control the Ultrasonic Atomizer
void controlUltrasonicAtomizer(actuatorData &data, bool state) {
  digitalWrite(data.Ultrasonic_Atomizer, state ? HIGH : LOW);
}


