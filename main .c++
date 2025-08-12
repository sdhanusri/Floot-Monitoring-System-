#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int trigPin = 5;  
const int echoPin = 4; 

SoftwareSerial gsmSerial(14, 12);


const char* ssid = "Your_WiFi_SSID";      
const char* password = "Your_WiFi_Password";  
const char* server = "example.com";

const int waterLevelThreshold = 50;

void setup() {
  Serial.begin(115200);
  gsmSerial.begin(9600);

  lcd.init();
  lcd.backlight();

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  sendCommand("AT");
  sendCommand("AT+CMGF=1"); 
  connectToWiFi();

  lcd.setCursor(0, 0);
  lcd.print("Flood Monitor");
  lcd.setCursor(0, 1);
  lcd.print("System Ready");
  delay(2000);
  lcd.clear();
}

void loop() {
  long duration = measureDistance();
  int waterLevel = duration / 29 / 2; // Convert to cm

  lcd.setCursor(0, 0);
  lcd.print("Water Level:");
  lcd.setCursor(0, 1);
  lcd.print(waterLevel);
  lcd.print(" cm");

  if (waterLevel < waterLevelThreshold) {
    sendSMS("+919043948404", "ALERT: Water level critical! Immediate action required."); 

  
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ALERT: Flood!");
    lcd.setCursor(0, 1);
    lcd.print("SMS Sent!");

    delay(60000); 
    lcd.clear();
  }

  sendDataToServer(waterLevel);

  delay(2000); 
}

long measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  return pulseIn(echoPin, HIGH);
}

void sendCommand(const char *command) {
  gsmSerial.println(command);
  delay(1000);
}

void sendSMS(const char *number, const char *message) {
  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(number);
  gsmSerial.println("\"");
  delay(500);

  gsmSerial.print(message);
  delay(500);

  gsmSerial.write(26); // CTRL+Z to send SMS
  delay(5000);
}

void connectToWiFi() {
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  WiFi.begin(ssid, password);

  int retryCount = 0; 
  while (WiFi.status() != WL_CONNECTED && retryCount < 60) { // Timeout after ~30 seconds
    delay(500);
    lcd.setCursor(14 + (retryCount % 2), 0);
    lcd.print(".");
    Serial.print(".");
    retryCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    Serial.println("\nWiFi Connected!");
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
    Serial.println("\nFailed to connect to WiFi.");
  }
  delay(2000);
  lcd.clear();
}

void sendDataToServer(int waterLevel) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    if (client.connect(server, 80)) {
      String data = "level=" + String(waterLevel);
      client.println("POST /upload HTTP/1.1");
      client.println("Host: example.com");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: ");
      client.println(data.length());
      client.println();
      client.print(data);

      Serial.println("Data sent to server: " + data);
      client.stop();
    } else {
      Serial.println("Failed to connect to server");
    }
  } else {
    Serial.println("WiFi not connected");
  }
}
