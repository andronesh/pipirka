#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <time.h>
#include <LittleFS.h>

// ================== CHANGE THESE ==================
const char* ssid     = "netis-1C3E9C";
const char* password = "999666333";

const char* mdnsName = "pipirka";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int  daylightOffset_sec = 0;

// BMP180 I2C pins (recommended ESP32 defaults)
#define SDA_PIN 7
#define SCL_PIN 6
// ==================================================

WebServer server(80);

bool bmpOk = false;
float tempC = NAN;
float press_hPa = NAN;
unsigned long lastRead = 0;

// ---------- TIME ----------
String getTimeString() {
  struct tm t;
  if (!getLocalTime(&t)) return "--:--:--";
  char b[16];
  strftime(b, sizeof(b), "%H:%M:%S", &t);
  return String(b);
}

String getDateString() {
  struct tm t;
  if (!getLocalTime(&t)) return "";
  char b[64];
  strftime(b, sizeof(b), "%A, %d %B %Y", &t);
  return String(b);
}

// ---------- SENSOR ----------
void updateSensor() {
  if (!bmpOk) return;
  if (millis() - lastRead < 1000) return;
  lastRead = millis();

  tempC = 33;
  press_hPa = 1122.33f;
}

String getTemp() {
  updateSensor();
//   if (!bmpOk || isnan(tempC)) return "--.- °C";
  char b[16];
  snprintf(b, sizeof(b), "%.1f °C", tempC);
  return String(b);
}

String getPress() {
  updateSensor();
//   if (!bmpOk || isnan(press_hPa)) return "----.- hPa";
  char b[20];
  snprintf(b, sizeof(b), "%.1f hPa", press_hPa);
  return String(b);
}

// ---------- WEB ----------
void handleRoot() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Ambient</title>

<link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;600;700&display=swap" rel="stylesheet">

<style>
:root{
  --glass: rgba(255,255,255,0.045);
  --glass2: rgba(255,255,255,0.07);
  --border: rgba(255,255,255,0.08);
  --text: rgba(255,255,255,0.88);
  --muted: rgba(255,255,255,0.55);
}

* { box-sizing: border-box; }

html,body{
  height:100%;
  margin:0;
  font-family: Inter, system-ui, sans-serif;
  color: var(--text);
}

body{
  background:
    radial-gradient(900px 600px at 20% 10%, rgba(120,120,255,0.12), transparent 60%),
    radial-gradient(900px 600px at 80% 90%, rgba(255,120,200,0.10), transparent 60%),
    linear-gradient(135deg, #0b1020, #0e1628);
}

.wrap{
  height:100%;
  display:grid;
  place-items:center;
  padding:24px;
}

.card{
  width:min(880px,100%);
  display:grid;
  grid-template-columns: 1.2fr 0.8fr;
  gap:16px;
  padding:18px;
  border-radius:24px;
  background: linear-gradient(180deg,var(--glass2),var(--glass));
  border:1px solid var(--border);
  backdrop-filter: blur(10px);
}

@media(max-width:820px){
  .card{grid-template-columns:1fr;}
}

.panel{
  padding:18px;
  border-radius:18px;
  background: rgba(255,255,255,0.04);
  border:1px solid var(--border);
}

.title{
  font-size:.85rem;
  letter-spacing:.08em;
  text-transform:uppercase;
  color:var(--muted);
}

.temp{
  margin-top:10px;
  font-size:clamp(4.2rem,9vw,6.8rem);
  font-weight:700;
  opacity:.92;
}

.chip{
  display:inline-block;
  margin-top:12px;
  padding:8px 12px;
  border-radius:999px;
  font-weight:600;
  background: rgba(255,255,255,0.05);
  border:1px solid var(--border);
  opacity:.85;
}

.time{
  margin-top:14px;
  font-size:clamp(2.1rem,5vw,3rem);
  font-weight:600;
  opacity:.85;
}

.date{
  margin-top:8px;
  font-size:1rem;
  color:var(--muted);
}

.kv{
  display:flex;
  justify-content:space-between;
  padding:10px 12px;
  margin-top:10px;
  border-radius:14px;
  background: rgba(255,255,255,0.04);
  border:1px solid var(--border);
}

.k{ color:var(--muted); }
.v{ font-weight:600; opacity:.9; }

.footer{
  margin-top:14px;
  font-size:.9rem;
  color:var(--muted);
}
</style>
</head>

<body>
<div class="wrap">
  <div class="card">

    <div class="panel">
      <div class="title">Indoor climate</div>
      <div class="temp" id="temp">--.- °C</div>
      <div class="chip" id="press">----.- hPa</div>
      <div class="time" id="time">--:--:--</div>
      <div class="date" id="date"></div>
      <div class="footer">myhome.local • ESP32</div>
    </div>

    <div class="panel">
      <div class="title">Details</div>

      <div class="kv"><div class="k">Temperature</div><div class="v" id="t2"></div></div>
      <div class="kv"><div class="k">Pressure</div><div class="v" id="p2"></div></div>
      <div class="kv"><div class="k">Time</div><div class="v" id="time2"></div></div>
      <div class="kv"><div class="k">Date</div><div class="v" id="date2"></div></div>
    </div>

  </div>
</div>

<script>
async function update(){
  try{
    const t = await (await fetch('/temp')).text();
    const p = await (await fetch('/press')).text();
    const ti = await (await fetch('/time')).text();
    const d = await (await fetch('/date')).text();

    temp.textContent = t;
    press.textContent = p;
    time.textContent = ti;
    date.textContent = d;

    t2.textContent = t;
    p2.textContent = p;
    time2.textContent = ti;
    date2.textContent = d;
  }catch(e){}
}
update();
setInterval(update,1000);
</script>
</body>
</html>
)rawliteral";

  server.send(200,"text/html",page);
}

void setup(){
  Serial.begin(115200);

  if(!LittleFS.begin(true)){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED) delay(500);

  MDNS.begin(mdnsName);
  configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);

  server.on("/style.css", HTTP_GET, []() {
    File file = LittleFS.open("/style.css", "r");
    if (!file) {
      server.send(404, "text/plain", "File not found");
      return;
    }
    server.streamFile(file, "text/css");
    file.close();
  });

  server.on("/", HTTP_GET, []() {
    File file = LittleFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  });

//   server.on("/",handleRoot);
  server.on("/time",[](){server.send(200,"text/plain",getTimeString());});
  server.on("/date",[](){server.send(200,"text/plain",getDateString());});
  server.on("/temp",[](){server.send(200,"text/plain",getTemp());});
  server.on("/press",[](){server.send(200,"text/plain",getPress());});

  server.begin();
}

void loop(){
  server.handleClient();
  updateSensor();
}