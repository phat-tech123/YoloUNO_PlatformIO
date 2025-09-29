#include <WiFi.h>
#include <WebServer.h>

// Access Point credentials
const char* ap_ssid = "ESP32S3_Setup";
const char* ap_password = "12345678";

// Pin for BOOT button (GPIO 0 - you may need to use INPUT_PULLUP)
#define BOOT_PIN 0

WebServer server(80);

// Track current mode
enum WiFiModeEnum { MODE_AP, MODE_STA };
WiFiModeEnum currentMode = MODE_AP;

void setupAPMode();
void setupSTAMode();

// Track credentials
String sta_ssid = "";
String sta_password = "";

// HTML WiFi Provisioning Page
const char* htmlForm = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>ESP32 WiFi Setup</title></head>
<body>
    <h1>Configure WiFi</h1>
    <form action="/connect" method="post">
        <label>WiFi Name (SSID):<input type="text" name="ssid" required></label><br>
        <label>Password:<input type="password" name="password" required></label><br>
        <input type="submit" value="CONNECT">
    </form>
</body>
</html>
)rawliteral";

// HTML Success Page
const char* htmlSuccess = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>ESP32 WiFi Setup</title></head>
<body>
    <h1>WiFi Connecting...</h1>
    <p>ESP32 is attempting to join your WiFi network.<br>Check the serial output for status.</p>
</body>
</html>
)rawliteral";


void setupSTAMode() {
  Serial.println("Switching to Station mode...");
  server.close();
  WiFi.softAPdisconnect(true);
  delay(100);

  WiFi.mode(WIFI_STA);
  WiFi.begin(sta_ssid.c_str(), sta_password.c_str());

  int maxTry = 20;
  while (WiFi.status() != WL_CONNECTED && maxTry--) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connect failed. Switching back to AP mode...");
    setupAPMode();
    return;
  }

  // Optional: You can add a simple page in STA mode
  server.on("/", HTTP_GET, [](){
    String page = "<!DOCTYPE html><html><body><h1>ESP32 Connected as STA</h1>";
    page += "<p>IP: " + WiFi.localIP().toString() + "</p></body></html>";
    server.send(200, "text/html", page);
  });
  server.begin();

  currentMode = MODE_STA;
}

// Utility function to switch to AP mode
void setupAPMode() {
  Serial.println("Switching to Access Point mode...");
  WiFi.disconnect(true);
  WiFi.softAP(ap_ssid, ap_password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  server.on("/", HTTP_GET, [](){
    server.send(200, "text/html", htmlForm);
  });
  server.on("/connect", HTTP_POST, [](){
    sta_ssid = server.arg("ssid");
    sta_password = server.arg("password");
    Serial.println("Received credentials:");
    Serial.print("SSID: "); Serial.println(sta_ssid);
    Serial.print("PASS: "); Serial.println(sta_password);

    server.send(200, "text/html", htmlSuccess);
    delay(1500); // Show message before changing mode
    setupSTAMode();
  });
  server.begin();

  currentMode = MODE_AP;
}

// Utility function to switch to Station mode


void setup() {
  Serial.begin(115200);

  pinMode(BOOT_PIN, INPUT_PULLUP); // HIGH by default, LOW when pressed

  setupAPMode();
}

void loop() {
  server.handleClient();

  static unsigned long lastBootCheck = 0;
  if (millis() - lastBootCheck > 200) { // check every 200ms
    lastBootCheck = millis();

    // If BOOT button pressed: switch to AP mode if not already there
    if(digitalRead(BOOT_PIN) == LOW && currentMode != MODE_AP){
      Serial.println("BOOT button pressed! Switching to Access Point mode.");
      delay(300); // debounce
      setupAPMode();
    }
  }
}