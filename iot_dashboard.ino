/*
   ESP32 Web Server dengan Data Sensor Analog Real-time + OTA Update
   Fitur:
   - Data sensor analog (ADC) real-time
   - Grafik interaktif
   - Data historis
   - Auto-refresh
   - OTA firmware update dengan autentikasi
   - Interface yang user-friendly
*/

#include <WiFi.h>
#include <NetworkClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Update.h>

// Konfigurasi WiFi
const char *ssid = "IoT-M2M";
const char *password = "bh38ie4g8NM";
const char *host = "esp32-sensor";

// Konfigurasi OTA
const char *authUser = "admin";
const char *authPass = "sensor123";

// Konfigurasi sensor analog
#define ANALOG_PIN A0  // Pin ADC (GPIO36 pada ESP32)

WebServer server(80);

const int led = 2; // Built-in LED ESP32

// Array untuk menyimpan data historis
const int MAX_DATA_POINTS = 50;
float sensorData[MAX_DATA_POINTS];
unsigned long timestamps[MAX_DATA_POINTS];
int dataIndex = 0;
int dataCount = 0;

// Variable untuk OTA
const char *csrfHeaders[2] = {"Origin", "Host"};
static bool authenticated = false;

// Fungsi untuk membaca sensor analog
float readAnalogSensor() {
  int rawValue = analogRead(ANALOG_PIN);
  // Konversi ke voltage (0-3.3V)
  float voltage = (rawValue * 3.3) / 4095.0;
  return voltage;
}

// Fungsi untuk menambah data ke array
void addDataPoint(float value) {
  sensorData[dataIndex] = value;
  timestamps[dataIndex] = millis();
  
  dataIndex = (dataIndex + 1) % MAX_DATA_POINTS;
  if (dataCount < MAX_DATA_POINTS) {
    dataCount++;
  }
}

// Fungsi untuk mendapatkan nilai minimum dan maksimum
float getMinValue() {
  if (dataCount == 0) return 0.0;
  float minVal = sensorData[0];
  for (int i = 1; i < dataCount; i++) {
    if (sensorData[i] < minVal) minVal = sensorData[i];
  }
  return minVal;
}

float getMaxValue() {
  if (dataCount == 0) return 3.3;
  float maxVal = sensorData[0];
  for (int i = 1; i < dataCount; i++) {
    if (sensorData[i] > maxVal) maxVal = sensorData[i];
  }
  return maxVal;
}

// Fungsi untuk generate SVG chart
String generateSVGChart() {
  String svg = "";
  
  if (dataCount < 2) return svg;
  
  // Hitung skala untuk voltage (0-3.3V)
  float minVal = 0.0;
  float maxVal = 3.3;
  
  // Generate path untuk sensor data
  svg += "<path class=\"sensor-line\" d=\"M";
  for (int i = 0; i < dataCount; i++) {
    float x = 50 + (i * 700.0 / (MAX_DATA_POINTS - 1));
    float y = 240 - (sensorData[i] - minVal) * 220.0 / (maxVal - minVal);
    if (i == 0) {
      svg += String(x) + "," + String(y);
    } else {
      svg += " L" + String(x) + "," + String(y);
    }
  }
  svg += "\"/>";
  
  // Tambahkan titik data
  for (int i = 0; i < dataCount; i++) {
    float x = 50 + (i * 700.0 / (MAX_DATA_POINTS - 1));
    float y = 240 - (sensorData[i] - minVal) * 220.0 / (maxVal - minVal);
    svg += "<circle class=\"data-point\" cx=\"" + String(x) + "\" cy=\"" + String(y) + "\"/>";
  }
  
  return svg;
}

// Halaman utama dengan navigasi
void handleRoot() {
  digitalWrite(led, HIGH);
  
  // Baca data sensor analog
  float sensorValue = readAnalogSensor();
  int rawValue = analogRead(ANALOG_PIN);
  
  // Tambahkan data ke array historis
  addDataPoint(sensorValue);
  
  // Hitung uptime
  unsigned long uptime = millis() / 1000;
  int hours = uptime / 3600;
  int minutes = (uptime / 60) % 60;
  int seconds = uptime % 60;

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta http-equiv='refresh' content='5'/>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32 Sensor Dashboard</title>";
  html += "<style>";
  html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  html += "body { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; color: white; min-height: 100vh; }";
  html += ".navbar { background: rgba(0,0,0,0.2); padding: 1rem; backdrop-filter: blur(10px); }";
  html += ".nav-container { max-width: 1200px; margin: 0 auto; display: flex; justify-content: space-between; align-items: center; }";
  html += ".nav-title { font-size: 1.5rem; font-weight: bold; }";
  html += ".nav-links { display: flex; gap: 20px; }";
  html += ".nav-links a { color: white; text-decoration: none; padding: 8px 16px; border-radius: 5px; transition: background 0.3s; }";
  html += ".nav-links a:hover { background: rgba(255,255,255,0.2); }";
  html += ".container { max-width: 1200px; margin: 0 auto; padding: 20px; }";
  html += ".main-card { background: rgba(255,255,255,0.1); backdrop-filter: blur(10px); border-radius: 15px; padding: 30px; box-shadow: 0 8px 32px rgba(0,0,0,0.3); }";
  html += "h1 { text-align: center; margin-bottom: 30px; font-size: 2.5em; text-shadow: 2px 2px 4px rgba(0,0,0,0.5); }";
  html += ".stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; margin-bottom: 30px; }";
  html += ".stat-card { background: rgba(255,255,255,0.2); padding: 20px; border-radius: 10px; text-align: center; border: 1px solid rgba(255,255,255,0.3); }";
  html += ".stat-value { font-size: 2em; font-weight: bold; margin: 10px 0; }";
  html += ".stat-label { font-size: 0.9em; opacity: 0.8; }";
  html += ".chart-container { background: rgba(255,255,255,0.1); border-radius: 10px; padding: 20px; margin: 20px 0; }";
  html += ".chart-title { text-align: center; margin-bottom: 15px; font-size: 1.2em; }";
  html += "svg { width: 100%; height: auto; }";
  html += ".sensor-line { stroke: #ff6b6b; stroke-width: 3; fill: none; }";
  html += ".grid-line { stroke: rgba(255,255,255,0.2); stroke-width: 1; }";
  html += ".axis-text { fill: white; font-size: 12px; }";
  html += ".data-point { fill: #ff6b6b; r: 3; }";
  html += ".status-badge { display: inline-block; padding: 5px 15px; border-radius: 20px; background: #4CAF50; color: white; font-size: 0.8em; margin-left: 10px; }";
  html += "@media (max-width: 768px) { .nav-container { flex-direction: column; gap: 10px; } .stats { grid-template-columns: 1fr; } }";
  html += "</style></head><body>";
  
  // Navigation bar
  html += "<nav class='navbar'>";
  html += "<div class='nav-container'>";
  html += "<div class='nav-title'>ESP32 Sensor Dashboard</div>";
  html += "<div class='nav-links'>";
  html += "<a href='/'>Dashboard</a>";
  html += "<a href='/api'>API</a>";
  html += "<a href='/update'>OTA Update</a>";
  html += "</div></div></nav>";
  
  html += "<div class='container'>";
  html += "<div class='main-card'>";
  html += "<h1>Analog Sensor Monitor <span class='status-badge'>ONLINE</span></h1>";
  html += "<div class='stats'>";
  html += "<div class='stat-card'><div class='stat-label'>Voltage</div><div class='stat-value'>" + String(sensorValue, 3) + " V</div></div>";
  html += "<div class='stat-card'><div class='stat-label'>Raw ADC</div><div class='stat-value'>" + String(rawValue) + "</div></div>";
  html += "<div class='stat-card'><div class='stat-label'>Percentage</div><div class='stat-value'>" + String((sensorValue/3.3)*100, 1) + "%</div></div>";
  html += "<div class='stat-card'><div class='stat-label'>Uptime</div><div class='stat-value'>" + String(hours) + ":" + String(minutes) + ":" + String(seconds) + "</div></div>";
  html += "</div>";
  html += "<div class='chart-container'>";
  html += "<div class='chart-title'>📈 Real-time Sensor Data</div>";
  html += "<svg viewBox='0 0 800 300' xmlns='http://www.w3.org/2000/svg'>";
  html += "<defs><pattern id='grid' width='40' height='30' patternUnits='userSpaceOnUse'>";
  html += "<path d='M 40 0 L 0 0 0 30' class='grid-line'/></pattern></defs>";
  html += "<rect width='800' height='300' fill='url(#grid)' opacity='0.1'/>";
  html += "<rect x='50' y='20' width='700' height='220' fill='rgba(0,0,0,0.1)' rx='5'/>";
  html += generateSVGChart();
  html += "<text x='40' y='25' class='axis-text'>3.3V</text>";
  html += "<text x='40' y='85' class='axis-text'>2.5V</text>";
  html += "<text x='40' y='145' class='axis-text'>1.6V</text>";
  html += "<text x='40' y='205' class='axis-text'>0.8V</text>";
  html += "<text x='40' y='265' class='axis-text'>0V</text>";
  html += "</svg>";
  html += "<div style='text-align: center; margin-top: 15px;'>";
  html += "<div style='display: inline-flex; align-items: center; gap: 10px;'>";
  html += "<div style='width: 30px; height: 3px; background: #ff6b6b; border-radius: 2px;'></div>";
  html += "<span>Sensor Value (Voltage)</span></div></div>";
  html += "</div>";
  html += "<div style='text-align: center; margin-top: 20px; opacity: 0.8;'>";
  html += "<p>Auto-refresh: 5s | Data Points: " + String(dataCount) + " | Min: " + String(getMinValue(), 3) + "V | Max: " + String(getMaxValue(), 3) + "V</p>";
  html += "</div></div></div></body></html>";

  server.send(200, "text/html", html);
  digitalWrite(led, LOW);
}

// Halaman OTA Update
void handleOTAPage() {
  if (!server.authenticate(authUser, authPass)) {
    return server.requestAuthentication();
  }
  
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32 OTA Update</title>";
  html += "<style>";
  html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  html += "body { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; color: white; min-height: 100vh; padding: 20px; }";
  html += ".container { max-width: 600px; margin: 0 auto; }";
  html += ".card { background: rgba(255,255,255,0.1); backdrop-filter: blur(10px); border-radius: 15px; padding: 30px; box-shadow: 0 8px 32px rgba(0,0,0,0.3); }";
  html += "h1 { text-align: center; margin-bottom: 30px; font-size: 2.5em; text-shadow: 2px 2px 4px rgba(0,0,0,0.5); }";
  html += ".upload-area { background: rgba(255,255,255,0.2); border: 2px dashed rgba(255,255,255,0.5); border-radius: 10px; padding: 40px; text-align: center; margin: 20px 0; }";
  html += "input[type=file] { margin: 20px 0; padding: 10px; background: rgba(255,255,255,0.2); border: 1px solid rgba(255,255,255,0.3); border-radius: 5px; color: white; width: 100%; }";
  html += "input[type=submit] { background: #4CAF50; color: white; padding: 12px 30px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; width: 100%; margin-top: 20px; }";
  html += "input[type=submit]:hover { background: #45a049; }";
  html += ".back-btn { background: #2196F3; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; text-decoration: none; display: inline-block; margin-bottom: 20px; }";
  html += ".back-btn:hover { background: #1976D2; }";
  html += ".info { background: rgba(255,255,255,0.2); padding: 15px; border-radius: 8px; margin: 20px 0; }";
  html += ".warning { background: rgba(255,152,0,0.2); border-left: 4px solid #ff9800; padding: 15px; margin: 20px 0; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<div class='card'>";
  html += "<a href='/' class='back-btn'>← Back to Dashboard</a>";
  html += "<h1>OTA Firmware Update</h1>";
  html += "<div class='info'>";
  html += "<h3>System Information</h3>";
  html += "<p><strong>Hostname:</strong> " + String(host) + "</p>";
  html += "<p><strong>Firmware Version:</strong> v1.0.0</p>";
  html += "<p><strong>Free Heap:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
  html += "<p><strong>Chip ID:</strong> " + String((uint32_t)ESP.getEfuseMac()) + "</p>";
  html += "</div>";
  html += "<div class='warning'>";
  html += "<p><strong Warning:</strong> Only upload firmware files (.bin). Device will restart after successful update.</p>";
  html += "</div>";
  html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
  html += "<div class='upload-area'>";
  html += "<h3>Select Firmware File</h3>";
  html += "<input type='file' name='update' accept='.bin' required>";
  html += "</div>";
  html += "<input type='submit' value='🚀 Upload & Update Firmware'>";
  html += "</form>";
  html += "</div></div></body></html>";
  
  server.send(200, "text/html", html);
}

// API endpoint untuk data JSON
void handleAPI() {
  float sensorValue = readAnalogSensor();
  int rawValue = analogRead(ANALOG_PIN);
  
  DynamicJsonDocument doc(1024);
  doc["voltage"] = sensorValue;
  doc["raw_adc"] = rawValue;
  doc["percentage"] = (sensorValue/3.3)*100;
  doc["uptime"] = millis() / 1000;
  doc["min_value"] = getMinValue();
  doc["max_value"] = getMaxValue();
  doc["data_points"] = dataCount;
  doc["free_heap"] = ESP.getFreeHeap();
  
  JsonArray dataArray = doc.createNestedArray("sensor_history");
  for (int i = 0; i < dataCount; i++) {
    dataArray.add(sensorData[i]);
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  server.send(200, "application/json", jsonString);
}

void handleNotFound() {
  digitalWrite(led, HIGH);
  String message = "404 - Page Not Found\n\n";
  message += "URI: " + server.uri() + "\n";
  message += "Method: " + String((server.method() == HTTP_GET) ? "GET" : "POST") + "\n";
  message += "Arguments: " + String(server.args()) + "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  digitalWrite(led, LOW);
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  
  Serial.begin(115200);
  Serial.println();
  Serial.println("🚀 ESP32 Sensor Dashboard Starting...");
  
  // Inisialisasi ADC
  analogReadResolution(12); // 12-bit ADC (0-4095)
  
  // Koneksi WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Inisialisasi mDNS
  if (MDNS.begin(host)) {
    Serial.println("mDNS responder started");
  }

  // Setup CSRF headers untuk OTA
  server.collectHeaders(csrfHeaders, 2);

  // Setup routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api", HTTP_GET, handleAPI);
  server.on("/update", HTTP_GET, handleOTAPage);
  
  // OTA Update handler
  server.on("/update", HTTP_POST, 
    []() {
      if (!authenticated) {
        return server.requestAuthentication();
      }
      server.sendHeader("Connection", "close");
      if (Update.hasError()) {
        server.send(500, "text/plain", "Update FAILED!");
      } else {
        server.send(200, "text/plain", "Update SUCCESS! Rebooting...");
        delay(1000);
        ESP.restart();
      }
    },
    []() {
      HTTPUpload &upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        authenticated = server.authenticate(authUser, authPass);
        if (!authenticated) {
          Serial.println("Authentication failed!");
          return;
        }
        
        // CSRF check
        String origin = server.header(String(csrfHeaders[0]));
        String host = server.header(String(csrfHeaders[1]));
        String expectedOrigin = String("http://") + host;
        if (origin != expectedOrigin && origin != "") {
          Serial.printf("CSRF check failed! Expected: %s, Got: %s\n", expectedOrigin.c_str(), origin.c_str());
          authenticated = false;
          return;
        }
        
        Serial.printf("Starting update: %s\n", upload.filename.c_str());
        if (!Update.begin()) {
          Update.printError(Serial);
        }
      } else if (authenticated && upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (authenticated && upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
          Serial.printf("Update Success: %u bytes\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
    }
  );
  
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("HTTP server started");
  Serial.printf("Dashboard: http://%s.local\n", host);
  Serial.printf("API: http://%s.local/api\n", host);
  Serial.printf("OTA: http://%s.local/update\n", host);
  Serial.printf("OTA Login: %s / %s\n", authUser, authPass);
  
  // Setup mDNS service
  MDNS.addService("http", "tcp", 80);
  
  digitalWrite(led, HIGH);
  delay(500);
  digitalWrite(led, LOW);
}

void loop(void) {
  server.handleClient();
  delay(2);
}