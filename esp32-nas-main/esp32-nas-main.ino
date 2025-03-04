#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <FS.h>
#include <ArduinoJson.h>

// WiFi Credentials
const char* WIFI_SSID = "MundoPacifico_xttkpg";
const char* WIFI_PASSWORD = "TQGP3RdQ";

// Pin Configurations
const uint8_t SD_CS_PIN = 5;  // Chip Select pin for SD card
const uint8_t SD_MOSI_PIN = 23;
const uint8_t SD_MISO_PIN = 19;
const uint8_t SD_SCK_PIN = 18;

// Web Server Configuration
const uint16_t SERVER_PORT = 80;
WebServer server(SERVER_PORT);

// File Upload Configuration
const size_t MAX_FILE_SIZE = 10240 * 1024 * 1024;  // 10MB max file size
const uint8_t MAX_UPLOAD_CONCURRENT = 700;  // Max concurrent uploads

// Global variables
File uploadFile;
String currentUploadPath = "/";
bool isUploading = false;

// Function declarations
void handleFileUpload();
void handleFileList();
void handleFileDelete();
void handleFileDownload();
void handleCreateDirectory();
void serveFile(const String& path);

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  
  // Initialize SD Card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card Mount Failed");
    return;
  }
  
  // Ensure basic directory structure
  if (!SD.exists("/")) SD.mkdir("/");
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Setup Server Routes
  server.on("/", HTTP_GET, [](){
    if (SD.exists("/index.html")) {
      serveFile("/index.html");
    } else {
      server.send(404, "text/plain", "index.html not found");
    }
  });
  
  server.on("/list", HTTP_GET, handleFileList);
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "Upload complete");
  }, handleFileUpload);
  server.on("/delete", HTTP_POST, handleFileDelete);
  server.on("/download", HTTP_GET, handleFileDownload);
  server.on("/mkdir", HTTP_POST, handleCreateDirectory);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

// Serve static files from SD card
void serveFile(const String& path) {
  if (SD.exists(path)) {
    File file = SD.open(path, FILE_READ);
    if (file) {
      String contentType = "text/html";
      if (path.endsWith(".css")) contentType = "text/css";
      else if (path.endsWith(".js")) contentType = "application/javascript";
      
      server.streamFile(file, contentType);
      file.close();
    } else {
      server.send(500, "text/plain", "Failed to open file");
    }
  } else {
    server.send(404, "text/plain", "File not found");
  }
}
