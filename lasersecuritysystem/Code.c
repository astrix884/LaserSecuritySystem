#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// WiFi credentials
const char* ssid = "Airtel_suya_5090";
const char* password = "air16733";

// Server and NTP setup
ESP8266WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); // 19800 = IST offset

// Hardware connections
#define LDR_PIN A0
#define BUZZER_PIN D7
#define LED D8

int threshold = 80;
bool intruderDetected = false;
bool systemEnabled = true;  // Control system ON/OFF

#define MAX_LOGS 50
String intrusionLogs[MAX_LOGS];
int logCount = 0;

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP address: " + WiFi.localIP().toString());

  timeClient.begin();
  timeClient.setTimeOffset(19800);

  // Root page
  server.on("/", []() {
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <title>Laser Security System</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          body { font-family: Arial; background: #f0f0f0; padding: 20px; }
          h2 { color: #333; }
          button {
            padding: 10px 20px;
            font-size: 16px;
            margin: 10px 0;
            background: #007BFF;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
          }
          button.off { background: #DC3545; }
          table { border-collapse: collapse; width: 100%; margin-top: 20px; }
          th, td { padding: 10px; border: 1px solid #ccc; text-align: center; }
          #status { font-size: 1.5em; margin-top: 10px; }
        </style>
        <script>
          function fetchData() {
            fetch("/data")
              .then(res => res.json())
              .then(data => {
                document.getElementById("status").innerHTML = data.status;
                document.getElementById("toggleBtn").innerText = data.enabled ? "Turn OFF System" : "Turn ON System";
                document.getElementById("toggleBtn").className = data.enabled ? "" : "off";

                let logHtml = "";
                for (let i = 0; i < data.logs.length; i++) {
                  logHtml += "<tr><td>" + (i + 1) + "</td><td>" + data.logs[i] + "</td></tr>";
                }
                document.getElementById("logTable").innerHTML = logHtml;
              });
          }

          function toggleSystem() {
            fetch("/toggle", { method: "POST" }).then(fetchData);
          }

          setInterval(fetchData, 1000);
          window.onload = fetchData;
        </script>
      </head>
      <body>
        <h2>Laser Security Monitoring System</h2>
        <button id="toggleBtn" onclick="toggleSystem()">Loading...</button>
        <p id="status">Loading...</p>
        <h3>Intrusion Log (Real-Time)</h3>
        <table>
          <thead>
            <tr><th>#</th><th>Time (IST)</th></tr>
          </thead>
          <tbody id="logTable"></tbody>
        </table>
      </body>
      </html>
    )rawliteral";

    server.send(200, "text/html", html);
  });

  // AJAX data
  server.on("/data", []() {
    String json = "{";
    json += "\"status\":\"" + String(systemEnabled ? (intruderDetected ? "🚨 Intruder Detected!" : "✅ All Clear") : "❌ System OFF") + "\",";
    json += "\"enabled\":" + String(systemEnabled ? "true" : "false") + ",";
    json += "\"logs\":[";
    for (int i = 0; i < logCount; i++) {
      json += "\"" + intrusionLogs[i] + "\"";
      if (i < logCount - 1) json += ",";
    }
    json += "]}";
    server.send(200, "application/json", json);
  });

  // Toggle ON/OFF
  server.on("/toggle", HTTP_POST, []() {
    systemEnabled = !systemEnabled;
    if (!systemEnabled) {
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(LED, LOW);
      intruderDetected = false;
    }
    server.send(200, "text/plain", "Toggled");
  });

  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
  timeClient.update();

  if (systemEnabled) {
    int ldrValue = analogRead(LDR_PIN);
    Serial.println(ldrValue);

    if (ldrValue < threshold) {
      if (!intruderDetected) {
        intruderDetected = true;
        digitalWrite(BUZZER_PIN, HIGH);
        digitalWrite(LED, HIGH);

        if (logCount < MAX_LOGS) {
          intrusionLogs[logCount++] = timeClient.getFormattedTime();
        }
      }
    } else {
      intruderDetected = false;
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(LED, LOW);
    }
  } else {
    // Ensure buzzer and LED stay off
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED, LOW);
    intruderDetected = false;
  }

  delay(200);
}