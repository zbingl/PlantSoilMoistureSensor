#include <WiFi.h>

const char* ssid = "NETGEAR69";
const char* password = "mightyzoo702";

WiFiServer server(80);

// Soil moisture sensor pins (classic ESP32 ADC1)
const int soilSensor1Pin = 32;
const int soilSensor2Pin = 33;
const int soilSensor3Pin = 34;
const int soilSensor4Pin = 35;

// Light output pins
const int light1 = 14;
const int light2 = 27;
const int light3 = 26;
const int light4 = 25;

// Soil moisture calibration values
const int AirValue = 3523;
const int WaterValue = 1882;

// Timing
unsigned long previousLightsCheck = 0;
const unsigned long lightsCheckInterval = 1000;

unsigned long previousWiFiCheck = 0;
const unsigned long wifiCheckInterval = 10000;

// Cached moisture readings
int moisture1 = 0;
int moisture2 = 0;
int moisture3 = 0;
int moisture4 = 0;

// --------------------------------------------------
// SETUP
// --------------------------------------------------

void setup() {
  Serial.begin(115200);

  pinMode(light1, OUTPUT);
  pinMode(light2, OUTPUT);
  pinMode(light3, OUTPUT);
  pinMode(light4, OUTPUT);

  // Start with all lights off (active-HIGH hardware)
  digitalWrite(light1, LOW);
  digitalWrite(light2, LOW);
  digitalWrite(light3, LOW);
  digitalWrite(light4, LOW);

  analogReadResolution(12);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  server.begin();

  Serial.println("ESP32 started.");
}

// --------------------------------------------------
// MAIN LOOP
// --------------------------------------------------

void loop() {
  unsigned long now = millis();

  // Read sensors and update lights every second,
  // regardless of Wi-Fi connection.
  if (now - previousLightsCheck >= lightsCheckInterval) {
    previousLightsCheck = now;

    moisture1 = readSoilMoisturePercent(soilSensor1Pin);
    moisture2 = readSoilMoisturePercent(soilSensor2Pin);
    moisture3 = readSoilMoisturePercent(soilSensor3Pin);
    moisture4 = readSoilMoisturePercent(soilSensor4Pin);

    lightsCheck(moisture1, moisture2, moisture3, moisture4);

    Serial.printf(
      "Moisture: %d%%, %d%%, %d%%, %d%%\n",
      moisture1, moisture2, moisture3, moisture4
    );
  }

  // Retry Wi-Fi connection periodically if disconnected.
  if (now - previousWiFiCheck >= wifiCheckInterval) {
    previousWiFiCheck = now;

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wi-Fi disconnected. Reconnecting...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
  }

  // Serve web pages only when Wi-Fi clients connect.
  WiFiClient client = server.available();

  if (client) {
    handleClient(client);
  }
}

// --------------------------------------------------
// SOIL MOISTURE READING
// --------------------------------------------------

int readSoilMoisturePercent(int pin) {
  int rawValue = analogRead(pin);

  Serial.printf("ADC pin %d: %d\n", pin, rawValue);

  int percent = map(rawValue, AirValue, WaterValue, 0, 100);

  return constrain(percent, 0, 100);
}

// --------------------------------------------------
// LIGHT CONTROL
// --------------------------------------------------

void lightsCheck(int perc1, int perc2, int perc3, int perc4) {
  digitalWrite(light1, perc1 > 50 ? LOW : HIGH);
  digitalWrite(light2, perc2 > 50 ? LOW : HIGH);
  digitalWrite(light3, perc3 > 50 ? LOW : HIGH);
  digitalWrite(light4, perc4 > 50 ? LOW : HIGH);

  Serial.printf(
    "Light states: %d, %d, %d, %d\n",
    perc1 < 50,
    perc2 < 50,
    perc3 < 50,
    perc4 < 50
  );
}

// --------------------------------------------------
// WEB SERVER
// --------------------------------------------------

void handleClient(WiFiClient& client) {
  unsigned long startTime = millis();
  String currentLine = "";

  while (client.connected() &&
         millis() - startTime < 2000) {

    if (client.available()) {
      char c = client.read();

      if (c == '\n') {
        if (currentLine.length() == 0) {
          sendResponse(client);
          break;
        } else {
          currentLine = "";
        }
      } else if (c != '\r') {
        currentLine += c;
      }
    }
  }

  client.stop();
}

void sendResponse(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type: text/html; charset=utf-8");
  client.println("Connection: close");
  client.println();

  client.println(buildHtmlPage());
}

String buildHtmlPage() {
  String page = "<!DOCTYPE html><html><head>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  page += "<meta http-equiv='refresh' content='5'>";
  page += "<title>ESP32 Soil Monitor</title>";

  page += "<style>";
  page += "body{font-family:Helvetica;text-align:center;margin:20px;}";
  page += ".reading{font-size:30px;margin:10px;padding:16px 40px;";
  page += "border-radius:8px;display:inline-block;color:white;";
  page += "background-color:#4CAF50;}";
  page += "</style></head><body>";

  page += "<h1>ESP32 Soil Moisture Monitor</h1>";

  page += buildSensorSection("Sensor 1", moisture1);
  page += buildSensorSection("Sensor 2", moisture2);
  page += buildSensorSection("Sensor 3", moisture3);
  page += buildSensorSection("Sensor 4", moisture4);

  page += "</body></html>";

  return page;
}

String buildSensorSection(const String& label, int percent) {
  String section = "<p>" + label + "</p>";
  section += "<p><span class='reading'>";
  section += String(percent);
  section += " %</span></p>";

  return section;
}