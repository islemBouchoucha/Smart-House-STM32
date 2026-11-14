/*
 * Smart House - ESP32 : passerelle Wi-Fi + serveur web embarqué
 *
 * Rôle : relier le navigateur (HTTP) au STM32 (UART).
 *  - Reçoit du STM32 les lignes  "STATE;T=25.3;GAS=120;PIR=0;..."
 *  - Envoie au STM32 les lignes  "CMD;LIGHT=1"
 *
 * Câblage UART (3,3 V des deux côtés) :
 *   ESP32 GPIO17 (TX2) -> STM32 PA10 (USART1_RX)
 *   ESP32 GPIO16 (RX2) <- STM32 PA9  (USART1_TX)
 *   GND commun obligatoire
 */
#include <WiFi.h>
#include <WebServer.h>
#include "secrets.h"

#define UART_RX_PIN 16
#define UART_TX_PIN 17
#define UART_BAUD   115200
#define STATE_TIMEOUT_MS 3000

WebServer server(80);
String rxLine;
String lastState;          // dernière ligne STATE reçue
unsigned long lastStateMs = 0;

const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smart House</title>
<style>
 body{font-family:system-ui,sans-serif;background:#0f172a;color:#e2e8f0;margin:0;padding:16px}
 h1{font-size:1.3rem;margin:0 0 12px}
 .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:12px}
 .card{background:#1e293b;border-radius:12px;padding:14px}
 .lbl{font-size:.75rem;color:#94a3b8}.val{font-size:1.6rem;font-weight:600}
 button{background:#2563eb;color:#fff;border:0;border-radius:8px;padding:10px;margin:4px 4px 0 0;font-size:.9rem;cursor:pointer}
 button.off{background:#475569}
 .bad{color:#f87171}.ok{color:#4ade80}
 #st{font-size:.8rem;margin-bottom:12px}
</style></head><body>
<h1>Smart House</h1>
<div id="st">Connexion...</div>
<div class="grid">
 <div class="card"><div class="lbl">Température</div><div class="val" id="T">--</div></div>
 <div class="card"><div class="lbl">Gaz (brut ADC)</div><div class="val" id="GAS">--</div></div>
 <div class="card"><div class="lbl">Présence (PIR)</div><div class="val" id="PIR">--</div></div>
 <div class="card"><div class="lbl">Alarme</div><div class="val" id="ALARM">--</div></div>
</div>
<div class="grid" style="margin-top:12px">
 <div class="card"><div class="lbl">Lumière : <b id="LIGHT">--</b></div>
  <button onclick="cmd('LIGHT',1)">Allumer</button><button class="off" onclick="cmd('LIGHT',0)">Éteindre</button></div>
 <div class="card"><div class="lbl">Ventilateur : <b id="FAN">--</b> (mode <b id="FANMODE">--</b>)</div>
  <button onclick="cmd('FANMODE',1)">ON</button><button class="off" onclick="cmd('FANMODE',0)">OFF</button><button onclick="cmd('FANMODE',2)">AUTO</button></div>
 <div class="card"><div class="lbl">Porte : <b id="DOOR">--</b></div>
  <button onclick="cmd('DOOR',1)">Ouvrir</button><button class="off" onclick="cmd('DOOR',0)">Fermer</button></div>
 <div class="card"><div class="lbl">Buzzer muet : <b id="MUTE">--</b></div>
  <button onclick="cmd('MUTE',1)">Couper</button><button class="off" onclick="cmd('MUTE',0)">Réactiver</button></div>
</div>
<script>
const modes=['OFF','ON','AUTO'];
function set(id,v){document.getElementById(id).textContent=v}
async function refresh(){
 const st=document.getElementById('st');
 try{
  const d=await (await fetch('/api/state')).json();
  if(!d.online){st.textContent='STM32 hors ligne';st.className='bad';return}
  st.textContent='En ligne';st.className='ok';
  set('T',d.T+' °C');set('GAS',d.GAS);set('PIR',d.PIR?'Détectée':'Aucune');
  set('ALARM',d.ALARM?'GAZ !':'OK');
  set('LIGHT',d.LIGHT?'ON':'OFF');set('FAN',d.FAN?'ON':'OFF');
  set('FANMODE',modes[d.FANMODE]);set('DOOR',d.DOOR?'Ouverte':'Fermée');
  set('MUTE',d.MUTE?'Oui':'Non');
 }catch(e){st.textContent='ESP32 injoignable';st.className='bad'}
}
async function cmd(c,v){await fetch('/api/cmd?c='+c+'&v='+v);setTimeout(refresh,300)}
refresh();setInterval(refresh,1000);
</script></body></html>
)rawliteral";

// Transforme "STATE;T=25.3;GAS=120;..." en JSON {"T":25.3,"GAS":120,...,"online":true}
String stateToJson() {
  bool online = lastState.length() > 0 && (millis() - lastStateMs) < STATE_TIMEOUT_MS;
  String json = "{";
  int start = lastState.indexOf(';') + 1;       // saute "STATE"
  while (start > 0 && start < (int)lastState.length()) {
    int end = lastState.indexOf(';', start);
    if (end < 0) end = lastState.length();
    String pair = lastState.substring(start, end);
    int eq = pair.indexOf('=');
    if (eq > 0) {
      json += "\"" + pair.substring(0, eq) + "\":" + pair.substring(eq + 1) + ",";
    }
    start = end + 1;
  }
  json += String("\"online\":") + (online ? "true" : "false") + "}";
  return json;
}

bool isAllowedKey(const String &k) {
  return k == "LIGHT" || k == "FANMODE" || k == "DOOR" || k == "MUTE";
}

void handleRoot()  { server.send_P(200, "text/html; charset=utf-8", PAGE); }
void handleState() { server.send(200, "application/json", stateToJson()); }

void handleCmd() {
  String c = server.arg("c");
  String v = server.arg("v");
  bool digit = v.length() == 1 && isDigit(v[0]);
  if (!isAllowedKey(c) || !digit) {
    server.send(400, "text/plain", "commande invalide");
    return;
  }
  Serial2.printf("CMD;%s=%s\n", c.c_str(), v.c_str());
  server.send(200, "text/plain", "ok");
}

void readStm32() {
  while (Serial2.available()) {
    char ch = (char)Serial2.read();
    if (ch == '\n') {
      rxLine.trim();
      if (rxLine.startsWith("STATE;")) {
        lastState = rxLine;
        lastStateMs = millis();
      }
      rxLine = "";
    } else if (ch != '\r' && rxLine.length() < 200) {
      rxLine += ch;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connexion Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  Serial.print("Adresse IP : "); Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/api/state", handleState);
  server.on("/api/cmd", handleCmd);
  server.begin();
}

void loop() {
  readStm32();
  server.handleClient();
  if (WiFi.status() != WL_CONNECTED) {      // reconnexion simple
    WiFi.reconnect();
    delay(1000);
  }
}
