#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

WiFiUDP udp;
IPAddress timeServer(132, 163, 4, 101); // NTP server address (NTP.PH.NET)

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

void setup() {
  Serial.begin(115200);
  delay(100);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Initialize UDP
  udp.begin(123);
}

void loop() {
  // Send NTP request
  sendNTPpacket();

  // Wait for response
  delay(1000);

  // Check if a response is available
  int packetSize = udp.parsePacket();
  if (packetSize) {
    // Read response into buffer
    udp.read(packetBuffer, NTP_PACKET_SIZE);

    // Extract time from response (seconds since Jan 1, 1900)
    unsigned long secsSince1900 = (unsigned long)packetBuffer[40] << 24 |
                                   (unsigned long)packetBuffer[41] << 16 |
                                   (unsigned long)packetBuffer[42] << 8 |
                                   (unsigned long)packetBuffer[43];

    // Convert NTP time to Unix time (seconds since Jan 1, 1970)
    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;

    // Apply GMT+8 offset
    epoch += 8 * 3600;

    // Print current time
    Serial.print("Current time (GMT+8): ");
    printTime(epoch);
  }
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

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
