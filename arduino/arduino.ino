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

// ----------------- PHOTO SENSITIVE RESISTOR ----------------- //
constexpr static int LDR_PIN = A0; // <<< PIN <<< //  

// An LDR with readings between [0, 1023] that will return ten light levels
// and will consider readings within 100 of each other to be the same level.
DebouncedLDR ldr(1023, 9, 100);

// ------------------------------------------------------------ //

// ------------------------- DHT22 ---------------------------- //
#define DHTPIN 2 // <<< PIN <<< // 
DHT dht(DHTPIN, DHTTYPE);
// ------------------------------------------------------------ //

// ------------------------ DS18B20 --------------------------- //
#define ONE_WIRE_BUS 3
#define TEMPERATURE_PRECISION 9 // Lower resolution

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int numberOfDevices;
DeviceAddress tempDeviceAddress;
// ------------------------------------------------------------ //

struct sensorData {
  float DHT22TempC[20];
  float DHT22TempF[20];
  float DHT22Humid[20];
  float DS18B20TempC[20];
  auto LDR[20]; // PHOTOSENSITIVE RESISTOR
  bool FloatR[10];
  bool FloatM[10];
}

void setup() {
  Serial.begin(115200);

  pinMode(LDR_PIN, INPUT);
  dht.begin();

  sensors.begin();
  numberOfDevices = sensors.getDeviceCount();
  for (int i = 0; i < numberOfDevices; i++) {
    if (sensors.getAddress(tempDeviceAddress, i)) {
      sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
    }
  }
}

void loop() {

  // _________________ PHOTO SENSITIVE RESISTOR __________________ //
  static DebouncedLDR::Level prev_level = 0;

  sensorData.LDR = ldr.update(analogRead(LDR_PIN));

  if (sensorData.LDR != prev_level) {
    Serial.print("Light level now ");
    Serial.println(level);
    prev_level = sensorData.LDR;
  }

  delay(50);
  
  // ____________________________________________________________ //

  // ___________________________ DHT22 ___________________________ //
  sensorData.DHT22Humid = dht.readHumidity();
  sensorData.DHT22TempC = dht.readTemperature();
  sensorData.DHT22TempF = dht.readTemperature(true);

  // DHT 22 TESTER
  // // Check if any reads failed and exit early (to try again).
  // if (isnan(h) || isnan(t) || isnan(f)) {
  //   Serial.println(F("Failed to read from DHT sensor!"));
  //   return;
  // }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(sensorData.DHT22TempF, sensorData.DHT22Humid);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(sensorData.DHT22TempC, sensorData.DHT22Humid, false);
  // ____________________________________________________________ //

  // ________________________ DS18B20 ___________________________ //
  for (int i = 0; i < numberOfDevices; i++) {
    if (sensors.getAddress(tempDeviceAddress, i)) {
      float tempCelsius = getDS18B20Temp(tempDeviceAddress);
      Serial.print("Temperature for device ");
      Serial.print(i);
      Serial.print(" in Celsius: ");
      Serial.println(tempCelsius);
    }
  }
  // ____________________________________________________________ //

  // ______________________ Float(reserved) _____________________ //
  sensorData.FloatR = digitalRead(SENSOR_PIN); // Read the sensor value
    
  if (sensorData.FloatR == HIGH) {
    Serial.println("Float switch is open");
  } else {
    Serial.println("Float switch is closed");
  }
  
  delay(100); // Debounce delay

  // ____________________________________________________________ //

  // ________________________ Float(main) ________________________ //
  sensorData.FloatM = digitalRead(SENSOR_PIN); // Read the sensor value
    
  if (sensorData.FloatM == HIGH) {
    controlWaterPumpM(true);
    Serial.println("Float switch is open");
  } else {
    controlWaterPumpM(false);
    Serial.println("Float switch is closed");
  }
  
  delay(100); // Debounce delay
  
  // ____________________________________________________________ //

  
}

float getDS18B20Temp(DeviceAddress deviceAddress)
{
  sensorData.DS18B20TempC = sensors.getTempC(deviceAddress);
  if (sensorData.DS18B20TempC == DEVICE_DISCONNECTED_C)
  {
    Serial.println("Error: Could not read temperature data");
    return -127.0; // Error code indicating failure to read temperature
  }

  return sensorData.DS18B20TempC;
}

//________________________ WATER PUMP ________________________ //

void controlWaterPumpM(bool state) {
  // Control the water pump based on the desired state
  digitalWrite(PUMP_PIN, state ? HIGH : LOW);
}
//________________________________________________________________________//

