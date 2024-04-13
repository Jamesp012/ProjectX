#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <FS.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

// Define EEPROM address to store user data
#define EEPROM_SIZE 512
#define EEPROM_USER_DATA_ADDRESS 0

struct UserData {
  char email[50];
  char username[50];
  char password[50];
};

ESP8266WebServer server(80);
bool loggedIn = false;
bool accountCreated = false;

WiFiUDP udp;
IPAddress timeServer(132, 163, 4, 101); // NTP server address (NTP.PH.NET)

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

// Function prototypes
void handleLoginPost();
bool checkCredentials(const String &username, const String &password);
void handleLogin();
void handleLogout();
void handleForgotPassword();
void handleCreateAccount();
void handleRegister();
void redirectToLogin();
void clearEEPROM();
void sendNTPpacket();
void printTime(unsigned long epoch);
void handleNotFound();
void handleGetSensorData();

void setup() {
  Serial.begin(9600);
  WiFiManager wm;
  wm.resetSettings(); // For testing, to clear stored credentials

  bool res = wm.autoConnect("AutoConnectAP", "password");
  if (!res) {
    Serial.println("Failed to connect");
  } else {
    Serial.println("Connected to WiFi");
    if (!SPIFFS.begin()) {
      Serial.println("Failed to mount SPIFFS");
      return;
    }
    EEPROM.begin(EEPROM_SIZE);

    server.on("/style.css", HTTP_GET, []() {
      serveFile("dist/style.css");
    });
    server.on("/login", HTTP_POST, handleLoginPost);
    server.on("/", HTTP_GET, handleLogin);
    server.on("/dashboard", HTTP_GET, []() {
      if (loggedIn) {
        serveFile("Web-App/dashboard.html");
      } else {
        redirectToLogin();
      }
    });
    server.on("/forgot-password", HTTP_GET, handleForgotPassword);
    server.on("/create-account", HTTP_GET, handleCreateAccount);
    server.on("/logout", HTTP_GET, handleLogout);
    server.on("/register", HTTP_POST, handleRegister);
    server.on("/get-sensor-data", HTTP_GET, handleGetSensorData);
    server.on("/images", HTTP_GET, []() {
      serveImages();
    });

    server.begin();
    Serial.println("HTTP server started");

    udp.begin(123);

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void loop() {
  sendNTPpacket();
  delay(1000);
  int packetSize = udp.parsePacket();
  if (packetSize) {
    udp.read(packetBuffer, NTP_PACKET_SIZE);
    unsigned long secsSince1900 = (unsigned long)packetBuffer[40] << 24 |
                                    (unsigned long)packetBuffer[41] << 16 |
                                    (unsigned long)packetBuffer[42] << 8 |
                                    (unsigned long)packetBuffer[43];
    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;
    epoch += 8 * 3600;
    Serial.print("Current time (GMT+8): ");
    printTime(epoch);
  }
  server.handleClient();
}

void serveFile(String filePath) {
  File file = SPIFFS.open("/" + filePath, "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  server.streamFile(file, getContentType(filePath));
  file.close();
}

void serveImages() {
  Serial.println("Scanning images directory");
  Dir dir = SPIFFS.openDir("/assets");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Content-Disposition", "inline");
  server.send(200, "image/png", ""); // Set content type to image/png
  while (dir.next()) {
    String fileName = dir.fileName();
    if (fileName.endsWith(".png")) {
      Serial.print("Serving image: ");
      Serial.println(fileName);
      File file = SPIFFS.open(fileName, "r");
      if (!file) {
        Serial.println("Failed to open file");
        return;
      }
      server.streamFile(file, "image/png");
      file.close();
    }
  }
}

String getContentType(String filename) {
  if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".png")) return "image/png"; // Fixed syntax here
  return "text/html"; // Default to HTML content type
}

void handleLoginPost() {
  if (server.method() == HTTP_POST) {
    String user = server.arg("username");
    String pass = server.arg("password");

    Serial.print("Received Username: ");
    Serial.println(user);
    Serial.print("Received Password: ");
    Serial.println(pass);

    if (checkCredentials(user, pass)) {
      loggedIn = true;
      server.sendHeader("Location", "/dashboard", true); // Redirect to dashboard after login
      server.send(302, "text/plain", "");
    } else {
      server.send(401, "text/plain", "Unauthorized"); // Unauthorized
    }
  }
}

bool checkCredentials(const String &username, const String &password) {
  if (server.method() == HTTP_POST) {
    String user = server.arg("username");
    String pass = server.arg("password");

    Serial.print("Received Username: ");
    Serial.println(user);
    Serial.print("Received Password: ");
    Serial.println(pass);

    if (checkCredentials(user, pass)) {
      loggedIn = true;
      server.sendHeader("Location", "/dashboard", true); // Redirect to dashboard after login
      server.send(302, "text/plain", "");
    } else {
      server.send(401, "text/plain", "Unauthorized"); // Unauthorized
    }
  }
}

void handleLogin() {
  Serial.println("Serving login.html");
  serveFile("Web-App/login.html");
}

void handleLogout() {
  loggedIn = false;
  server.sendHeader("Location", "/", true); // Redirect to login page after logout
  server.send(302, "text/plain", "");
}

void handleForgotPassword() {
  Serial.println("Serving forgotPassword.html");
  serveFile("Web-App/forgotPassword.html");
}

void handleCreateAccount() {
  if (!accountCreated) {
    Serial.println("Serving createAccount.html");
    serveFile("Web-App/createAccount.html");
  } else {
    // If an account has already been created, display a message indicating that only 1 account is allowed
    server.send(200, "text/html", "<h1>Sorry, only 1 account is allowed.</h1>");
  }
}

void handleRegister() {
  if (server.method() == HTTP_POST) {
    String email = server.arg("email");
    String username = server.arg("username");
    String password = server.arg("password");

    // Validate input (e.g., check if email/username is unique, password meets requirements, etc.)
    // Add your validation logic here

    // Create user data structure
    UserData userData;
    email.toCharArray(userData.email, sizeof(userData.email));
    username.toCharArray(userData.username, sizeof(userData.username));
    password.toCharArray(userData.password, sizeof(userData.password));

    // Store user information in EEPROM
    storeUserData(userData);

    // Set the flag to indicate that an account has been created
    accountCreated = true;

    // Redirect to login page after registration
    redirectToLogin();
  } else {
    // If the request method is not POST, send a 405 Method Not Allowed response
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

void storeUserData(const UserData &userData) {
  EEPROM.put(EEPROM_USER_DATA_ADDRESS, userData);
  EEPROM.commit();
}

void redirectToLogin() {
  server.sendHeader("Location", "/", true); // Redirect to login page
  server.send(302, "text/plain", "");
}

void clearEEPROM() {
  UserData emptyUserData;
  memset(emptyUserData.email, 0, sizeof(emptyUserData.email)); // Clear email
  memset(emptyUserData.username, 0, sizeof(emptyUserData.username)); // Clear username
  memset(emptyUserData.password, 0, sizeof(emptyUserData.password)); // Clear password

  EEPROM.put(EEPROM_USER_DATA_ADDRESS, emptyUserData); // Store empty data in EEPROM
  EEPROM.commit();
}

void sendNTPpacket() {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  udp.beginPacket(timeServer, 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void printTime(unsigned long epoch) {
  // Convert epoch time to local time
  setTime(epoch);
  
  // Print time in HH:MM:SS format
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println();
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void handleGetSensorData() {
  // Create a JSON object
  StaticJsonDocument<200> jsonDocument;

  //Populate JSON object with sensor data
  jsonDocument["DHT22TempC"] = sensor.DHT22TempC;
  jsonDocument["DHT22Humid"] = sensor.DHT22Humid;
  jsonDocument["DS18B20TempC"] = sensor.DS18B20TempC;

  //DHT22 remarks
  if(sensor.DHT22TempC > 0){
    jsonDocument["DHT22State"] = "Working";
  }else{
    jsonDocument["DHT22State"] = "Not working";
  }

  //DS18B20 remarks 
  if(sensor.DS18B20TempC > 0){
    jsonDocument["DS18B20State"] = "Working";
  }else{
    jsonDocument["DS18B20State"] = "Not working";
  }

  //LDR remarks
  if(sensor.LDR > 0){
    jsonDocument["LDRState"] = "Working";
  }else{
    jsonDocument["LDRState"] = "Not working";
  }

  // Float Switch RESERVED remarks
  if(sensor.FloatR == true){
    jsonDocument["FloatR"] = "OFF";
  }else{
    jsonDocument["FloatR"] = "ON";
  }

  // Float Switch MAIN remarks
  if(sensor.FloatM == true){
    jsonDocument["FloatM"] = "OFF";
  }else{
    jsonDocument["FloatM"] = "ON";
  }

  // Heating Element remarks
  if(sensor.DS18B20TempC > 0 && DHTReading == 1){
    jsonDocument["HeatingElement"] = "Heating";
  }else if(sensor.DS18B20TempC > 0 && DHTReading == 2){
    jsonDocument["HeatingElement"] = "Stabilizing"
  }else if(sensor.DS18B20TempC > 0 && DHTReading == 3){
    jsonDocument["HeatingElement"] = "Cooling" 
  }

  // LED_Light remarks
  if(actuator.LED_Light == true){
    jsonDocument["LEDLight"] = "ON";
  }else{
    jsonDocument["LEDLight"] = "OFF";
  }

  // Ultasonic Atomizer remarks
  if(actuator.Ultrasonic_Atomizer == true){
    jsonDocument["UltrasonicAtomizer"] = "ON";
  }else{
    jsonDocument["UltrasonicAtomizer"] = "OFF";
  }

  // Water_Pump
  if(actuator.Water_Pump == true){
    jsonDocument["WaterPump"] = "ON";
  }else{
    jsonDocument["WaterPump"] = "OFF";
  }

  // CPU Fan1
  if(actuator.Cpu_Fan1 > 0 ){
    jsonDocument["Fan1"] = "Working";
  }else{
    jsonDocument["Fan1"] = "Not Working";
  }

  // CPU Fan2
  if(actuator.Cpu_Fan2 > 0 ){
    jsonDocument["Fan2"] = "Working";
  }else{
    jsonDocument["Fan2"] = "Not Working";
  }
  
  // Serialize JSON to a string
  String jsonString;
  serializeJson(jsonDocument, jsonString);

  // Send JSON response
  server.send(200, "application/json", jsonString)
}
