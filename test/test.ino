#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <EEPROM.h>

// Define EEPROM address to store user data
#define EEPROM_SIZE 512
#define EEPROM_USER_DATA_ADDRESS 0

struct UserData {
  char email[50];
  char username[50];
  char password[50];
};

const char* ssid = "GomezCorRegidor";
const char* password = "MOneKTreyding";

// Define the username and password
const char* username = "admin";
const char* user_password = "123456";

ESP8266WebServer server(80);
bool loggedIn = false;
bool accountCreated = false; // Flag to track if an account has been created

void setup() {
  Serial.begin(9600);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  // Clear EEPROM data
  clearEEPROM();
  // Set up routes to serve CSS and image files from SPIFFS
  server.on("/style.css", HTTP_GET, []() {
    Serial.println("Serving style.css");
    serveFile("dist/style.css");
  });

  // Login route
  server.on("/", HTTP_POST, handleLoginPost);
  server.on("/", HTTP_GET, handleLogin);

  // Dashboard route - accessible only if logged in
  server.on("/dashboard", HTTP_GET, []() {
    if (loggedIn) {
      Serial.println("Serving dashboard.html");
      serveFile("Web-App/dashboard.html");
    } else {
      redirectToLogin();
    }
  });

  // Forgot password route
  server.on("/forgot-password", HTTP_GET, handleForgotPassword);

  // Create account route
  server.on("/create-account", HTTP_GET, handleCreateAccount);

  // Logout route
  server.on("/logout", HTTP_GET, handleLogout);

  // Registration route
  server.on("/register", HTTP_POST, handleRegister);

  // Set up endpoint to serve PNG images
  server.on("/images", HTTP_GET, []() {
    Serial.println("Serving images");
    serveImages();
  });

  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
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
  // Add more content types as needed
  return "text/html"; // Default to HTML content type
}

void handleLoginPost() {
  if (server.method() == HTTP_POST) {
    String user = server.arg("username");
    String pass = server.arg("password");

    if (checkCredentials(user, pass)) {
      loggedIn = true;
      server.sendHeader("Location", "/dashboard", true); // Redirect to dashboard after login
      server.send(302, "text/plain", "");
    } else {
      server.send(401, "text/plain", "Unauthorized"); // Unauthorized
    }
  }
}

bool checkCredentials(String username, String password) {
  // Read user data from EEPROM
  UserData storedUserData;
  EEPROM.get(EEPROM_USER_DATA_ADDRESS, storedUserData);

  // Compare with provided credentials
  return (strcmp(username.c_str(), storedUserData.username) == 0 &&
          strcmp(password.c_str(), storedUserData.password) == 0);
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


void storeUserData(UserData userData) {
  EEPROM.put(EEPROM_USER_DATA_ADDRESS, userData);
  EEPROM.commit();
}

void redirectToLogin() {
  server.sendHeader("Location", "/", true); // Redirect to login page
  server.send(302, "text/plain", "");
}

// Removing username, password, and email from EEPROM
void clearEEPROM() {
  UserData emptyUserData;
  memset(emptyUserData.email, 0, sizeof(emptyUserData.email)); // Clear email
  memset(emptyUserData.username, 0, sizeof(emptyUserData.username)); // Clear username
  memset(emptyUserData.password, 0, sizeof(emptyUserData.password)); // Clear password

  EEPROM.put(EEPROM_USER_DATA_ADDRESS, emptyUserData); // Store empty data in EEPROM
  EEPROM.commit();
}


void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}
