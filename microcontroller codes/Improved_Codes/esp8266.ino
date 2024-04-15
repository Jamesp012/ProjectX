#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <FS.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <TimeLib.h>


// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
//static const char ntpServerName[] = "time.nist.gov";
//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";


const int timeZone = 8;     // Philippine Standard Time 

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

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

    Serial.print("Local Website Started @");
    Serial.println(WiFi.localIP());
    // Connecting UDP port
    Serial.println("Starting UDP");
    Udp.begin(localPort);
    Serial.print("Local port: ");
    Serial.println(Udp.localPort());
    Serial.println("waiting for sync");
    setSyncProvider(getNtpTime);
    setSyncInterval(300);
  }
}

time_t prevDisplay = 0; // when the digital clock was displayed

void loop() {
  server.handleClient();


  // For the timer display
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();
    }
  }
}

void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();
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
  // Serial.println("Scanning images directory");
  Dir dir = SPIFFS.openDir("/assets");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Content-Disposition", "inline");
  server.send(200, "image/png", ""); // Set content type to image/png
  while (dir.next()) {
    String fileName = dir.fileName();
    if (fileName.endsWith(".png")) {
      // Serial.print("Serving image: ");
      // Serial.println(fileName);
      File file = SPIFFS.open(fileName, "r");
      if (!file) {
        // Serial.println("Failed to open file");
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
      return true;
    } else {
      server.send(401, "text/plain", "Unauthorized", false); // Unauthorized
      return false;
    }
  }
  return false;
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

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address){
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void printTime(unsigned long epoch) {
  // Convert epoch time to local time
  setTime(epoch);
  
  // Print time in HH:MM:SS format
  Serial.print(hour());
  // printDigits(minute());
  // printDigits(second());


  if(hour() >= 7 && hour() <= 17){
    // Send the Time in the Serial.
    Serial.print(hour());
    Serial.println();
  }
}

void printDigits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

time_t getNtpTime(){
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}


//Receive the data from the arduino and pass to webserver using AJAX
void handleGetSensorData() {

  // Read the incoming data
  String jsonData = Serial.readStringUntil('\n');

  // Send the JSON string in the HTTP response
  server.send(200, "application/json", jsonData);

}
