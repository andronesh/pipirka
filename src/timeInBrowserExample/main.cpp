#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <ESPmDNS.h>

// ====== CHANGE THESE ======
const char* ssid     = "";
const char* password = "";

// Time / NTP
const char* ntpServer = "pool.ntp.org";

// Timezone: Croatia (Europe/Zagreb) is usually UTC+1, and UTC+2 in DST.
// Simplest fixed offset: set UTC+1 (3600). If you want DST auto, tell me and I’ll give that version.
const long gmtOffset_sec = 3600;      // UTC+1
const int daylightOffset_sec = 0;     // Set 3600 if you want to force DST (not automatic)

// Web server
WebServer server(80);

String formatTimeNow() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return String("Time not available (NTP not synced yet)");
  }

  char buf[32];
  // YYYY-MM-DD HH:MM:SS
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buf);
}

void handleRoot() {
  // Simple HTML page that fetches /time every second
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Time</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 24px; }
    .card { padding: 16px; border: 1px solid #ddd; border-radius: 12px; max-width: 420px; }
    #t { font-size: 1.6rem; font-weight: 700; }
    .small { color: #666; margin-top: 8px; }
  </style>
</head>
<body>
  <div class="card">
    <div>Current time:</div>
    <div id="t">Loading...</div>
    <div class="small">Updates every second</div>
  </div>

  <script>
    async function updateTime(){
      try{
        const r = await fetch('/time', { cache: 'no-store' });
        const text = await r.text();
        document.getElementById('t').textContent = text;
      } catch(e){
        document.getElementById('t').textContent = 'Error';
      }
    }
    updateTime();
    setInterval(updateTime, 1000);
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleTime() {
  server.send(200, "text/plain", formatTimeNow());
}

void setup() {
  Serial.begin(115200);

  // Connect Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("Connecting to WiFi: ");
    Serial.print(ssid);
    Serial.print(" with password: ");
    Serial.println(password);
  }
  Serial.println("\nWiFi connected!");
  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin("pipirka")) {
    Serial.println("mDNS failed");
  } else {
    Serial.println("mDNS started: http://pipirka.local/");
  }

  // NTP time init
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Optional: wait a bit for first sync
  Serial.println("Syncing time...");
  for (int i = 0; i < 15; i++) {
    if (formatTimeNow().indexOf("not available") == -1) break;
    delay(500);
  }
  Serial.println("Current time: " + formatTimeNow());

  // Web routes
  server.on("/", handleRoot);
  server.on("/time", handleTime);

  server.begin();
  Serial.println("Web server started. Open the IP in your browser.");
}

void loop() {
  server.handleClient();
}
