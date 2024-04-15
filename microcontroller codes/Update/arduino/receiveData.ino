#include <ArduinoJson.h>

void setup(){
    Serial.begin(9600);
}

int time = 0;
void loop(){
    String message = Serial.readStringUntil('\n');
    if(message.equals("StageTwoDone")){
        time =
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