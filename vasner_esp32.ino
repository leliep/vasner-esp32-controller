#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "ir_codes.h"
#include "secrets.h"

const char* USERNAME = "admin";

// IR-Sender ueber Transistorstufe; blaue Board-LED als Sendeanzeige.
// Beide Ausgaenge sind beim konkret getesteten Board active HIGH.
constexpr uint8_t IR_LED_PIN = 4;
constexpr uint8_t STATUS_LED_PIN = LED_BUILTIN;
constexpr uint16_t COMMAND_GAP_MS = 300;
constexpr uint16_t TIMER_GAP_MS = 150;

IRsend irsend(IR_LED_PIN, false);
WebServer server(80);

const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="de"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#17120f"><title>Terrassenheizer</title><style>
:root{color-scheme:dark;--bg:#17120f;--card:#241c18;--line:#3d3029;--text:#fff8f2;--muted:#b9aaa0;--hot:#ef7b2d;--hot2:#ffad45;--off:#4a3b35}
*{box-sizing:border-box}body{margin:0;background:radial-gradient(circle at top,#3b2418 0,var(--bg) 42%);color:var(--text);font-family:system-ui,-apple-system,sans-serif;min-height:100vh}
main{width:min(100% - 28px,520px);margin:auto;padding:30px 0 40px}header{margin:0 2px 24px}h1{margin:0;font-size:1.7rem;letter-spacing:-.03em}header p{margin:6px 0 0;color:var(--muted);font-size:.92rem}
.card{background:rgba(36,28,24,.94);border:1px solid var(--line);border-radius:20px;padding:18px;margin:14px 0;box-shadow:0 14px 35px #0005}.label{display:block;margin:0 0 13px;color:var(--muted);font-size:.78rem;font-weight:700;letter-spacing:.12em;text-transform:uppercase}
.grid{display:grid;grid-template-columns:repeat(2,1fr);gap:10px}.levels{grid-template-columns:repeat(4,1fr)}button,select{font:inherit;border:0;border-radius:13px;min-height:52px}button{font-weight:750;color:var(--text);background:#392c26;cursor:pointer;transition:transform .08s,filter .15s}button:active{transform:scale(.97)}button:disabled{opacity:.55;cursor:wait}.on{background:linear-gradient(135deg,var(--hot),var(--hot2));color:#281307}.off{background:var(--off)}.level{background:#342923;border:1px solid #4b3a32}.max{width:100%;margin-top:10px;background:#5a2d18;color:#ffd8bd}
.timer{display:grid;grid-template-columns:1fr auto;gap:10px}select{width:100%;padding:0 42px 0 14px;background:#342923;color:var(--text);border:1px solid #4b3a32}.apply{padding:0 20px;background:var(--hot);color:#281307}
#msg{position:fixed;left:50%;bottom:22px;transform:translate(-50%,18px);width:max-content;max-width:calc(100% - 32px);padding:11px 17px;border-radius:99px;background:#f8eee8;color:#281b16;font-size:.9rem;font-weight:650;opacity:0;pointer-events:none;transition:.2s;box-shadow:0 8px 30px #0008}#msg.show{opacity:1;transform:translate(-50%,0)}
@media(max-width:370px){.levels{grid-template-columns:repeat(2,1fr)}}
</style></head><body><main><header><h1>Terrassenheizer</h1><p>WLAN-Fernbedienung</p></header>
<section class="card"><span class="label">Betrieb</span><div class="grid">
<button class="on" onclick="go('/api/on','Heizer eingeschaltet')">Einschalten</button>
<button class="off" onclick="go('/api/off','Heizer ausgeschaltet')">Ausschalten</button></div></section>
<section class="card"><span class="label">Heizleistung</span><div class="grid levels">
<button class="level" onclick="go('/api/level?n=1','Heizstufe L1')">L1</button>
<button class="level" onclick="go('/api/level?n=2','Heizstufe L2')">L2</button>
<button class="level" onclick="go('/api/level?n=3','Heizstufe L3')">L3</button>
<button class="level" onclick="go('/api/level?n=4','Heizstufe L4')">L4</button></div>
<button class="max" onclick="go('/api/max','Maximalleistung')">Maximalleistung</button></section>
<section class="card"><label class="label" for="timer">Abschalttimer</label><div class="timer">
<select id="timer"><option value="0">Dauerbetrieb</option><option value="1">1 Stunde</option><option value="2">2 Stunden</option><option value="3">3 Stunden</option><option value="4">4 Stunden</option><option value="5">5 Stunden</option><option value="6">6 Stunden</option><option value="7">7 Stunden</option><option value="8">8 Stunden</option></select>
<button class="apply" onclick="setTimer()">Übernehmen</button></div></section></main><div id="msg"></div>
<script>
let hideMsg;async function go(url,success){const msg=document.getElementById('msg'),buttons=document.querySelectorAll('button');buttons.forEach(b=>b.disabled=true);clearTimeout(hideMsg);msg.textContent='Befehl wird gesendet …';msg.classList.add('show');try{const r=await fetch(url,{method:'POST'});if(!r.ok)throw Error();msg.textContent=success}catch(e){msg.textContent='Übertragung fehlgeschlagen'}finally{buttons.forEach(b=>b.disabled=false);hideMsg=setTimeout(()=>msg.classList.remove('show'),2600)}}
function setTimer(){const h=document.getElementById('timer').value;go('/api/timer?h='+h,h==='0'?'Dauerbetrieb gewählt':'Abschaltung nach '+h+' h')}
</script></body></html>)HTML";

bool authenticate() {
  if (server.authenticate(USERNAME, WEB_PASSWORD)) return true;
  server.requestAuthentication();
  return false;
}

void sendCommand(uint8_t command, uint16_t gapMs = COMMAND_GAP_MS) {
  const uint32_t data = irsend.encodeNEC(IR_ADDRESS, command);
  digitalWrite(STATUS_LED_PIN, HIGH);
  irsend.sendNEC(data);
  digitalWrite(STATUS_LED_PIN, LOW);
  delay(gapMs);
}

void setLevel(uint8_t level) {
  // ON ist eine eigene Taste und wird als idempotenter Einschaltbefehl vorausgesetzt.
  sendCommand(IR_CMD_ON);
  delay(500);                       // Aufwachzeit des Heizers
  for (uint8_t i=0; i<3; i++) sendCommand(IR_CMD_DOWN); // sicher L1
  for (uint8_t i=1; i<level; i++) sendCommand(IR_CMD_UP);
}

void setTimer(uint8_t hours) {
  // Annahme: RECHTS saettigt bei 8 h und laeuft nicht auf 1 h/Dauer weiter.
  // Mehrfach RECHTS normalisiert daher auch einen unbekannten Timerzustand auf 8 h.
  for (uint8_t i=0; i<8; i++) sendCommand(IR_CMD_RIGHT, TIMER_GAP_MS);
  const uint8_t leftPresses = hours == 0 ? 8 : 8 - hours;
  for (uint8_t i=0; i<leftPresses; i++) sendCommand(IR_CMD_LEFT, TIMER_GAP_MS);
}

void replyOk(const char* text) { server.send(200, "text/plain; charset=utf-8", text); }

void setup() {
  Serial.begin(115200);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);  // blaue LED im Ruhezustand aus
  irsend.begin();
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("terrassenheizer");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(250);
  Serial.print("Weboberflaeche: http://"); Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](){ if (authenticate()) server.send_P(200, "text/html; charset=utf-8", PAGE); });
  server.on("/api/on", HTTP_POST, [](){ if (!authenticate()) return; sendCommand(IR_CMD_ON); replyOk("on"); });
  server.on("/api/off", HTTP_POST, [](){ if (!authenticate()) return; sendCommand(IR_CMD_OFF); replyOk("off"); });
  server.on("/api/max", HTTP_POST, [](){ if (!authenticate()) return; sendCommand(IR_CMD_ON); delay(500); sendCommand(IR_CMD_MAX); replyOk("max"); });
  server.on("/api/level", HTTP_POST, [](){
    if (!authenticate()) return;
    int level = server.arg("n").toInt();
    if (level < 1 || level > 4) { server.send(400, "text/plain", "level 1..4 required"); return; }
    setLevel(level); replyOk("ok");
  });
  server.on("/api/timer", HTTP_POST, [](){
    if (!authenticate()) return;
    if (!server.hasArg("h")) { server.send(400, "text/plain", "timer 0..8 required"); return; }
    int hours = server.arg("h").toInt();
    if (hours < 0 || hours > 8) { server.send(400, "text/plain", "timer 0..8 required"); return; }
    setTimer(hours); replyOk("ok");
  });
  server.onNotFound([](){ server.send(404, "text/plain", "not found"); });
  server.begin();
}

void loop() { server.handleClient(); }
