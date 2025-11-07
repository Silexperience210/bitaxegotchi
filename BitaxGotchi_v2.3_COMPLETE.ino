/*
 * BitaxGotchi BitaxeS3 - VERSION COMPLÈTE
 * Portail Web + Jeu Complet + Évolution basée sur shares
 * 
 * Évolution:
 * - Œuf: 0 shares
 * - Bébé: 50,000 shares
 * - Enfant: 100,000 shares
 * - Ado: 200,000 shares
 * - Adulte: 350,000 shares
 * - ELITE: 500,000 shares
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#define TFT_BL 38
#define POWER_ON_PIN 15
#define BUTTON_1 0
#define BUTTON_2 14

#define AP_SSID "BitaxGotchi-Setup"

DNSServer dnsServer;
WebServer webServer(80);
Preferences prefs;

// ============== CONFIGURATION ==============

struct Config {
  char wifiSSID[64];
  char wifiPassword[64];
  int numBitaxe;
  char bitaxeIPs[10][16];
  char bitaxeNames[10][32];
  bool configured;
} config;

struct BitaxeStats {
  unsigned long shares;
  unsigned long bestDiff;
  float hashrate;
  float temp;
  bool isOnline;
  unsigned long lastUpdate;
} bitaxeStats[10];

// ============== PET ==============

enum PetState { 
  STATE_EGG,      // 0 shares
  STATE_BABY,     // 50k shares
  STATE_CHILD,    // 100k shares
  STATE_TEEN,     // 200k shares
  STATE_ADULT,    // 350k shares
  STATE_ELITE,    // 500k shares
  STATE_DEAD 
};

struct Pet {
  PetState state;
  int hunger;           // 0-100
  int happiness;        // 0-100
  int health;           // 0-100
  int energy;           // 0-100 (nouveau)
  int weight;
  unsigned long totalLifetimeShares;
  unsigned long lastFeed;
  unsigned long lastPlay;
  unsigned long lastClean;
  unsigned long bornTime;
  bool isDirty;
  bool isSick;
  bool isSleeping;
} myPet;

TFT_eSPI tft = TFT_eSPI();

unsigned long totalShares = 0;
unsigned long lastShareCount = 0;
unsigned long lastPetUpdate = 0;
unsigned long lastShareCheck = 0;
unsigned long sessionStartShares = 0;
unsigned long lastScreenRedraw = 0;

bool configMode = false;
bool needsRedraw = true;
int currentScreen = 0; // 0=main, 1=stats, 2=menu
int lastScreen = -1;

// ============== SAUVEGARDER/CHARGER ==============

void loadConfig() {
  prefs.begin("bitaxgotchi", true);
  config.configured = prefs.getBool("configured", false);
  
  if(config.configured) {
    prefs.getString("ssid", config.wifiSSID, 64);
    prefs.getString("pass", config.wifiPassword, 64);
    config.numBitaxe = prefs.getInt("num", 0);
    
    for(int i = 0; i < config.numBitaxe; i++) {
      prefs.getString(("ip" + String(i)).c_str(), config.bitaxeIPs[i], 16);
      prefs.getString(("name" + String(i)).c_str(), config.bitaxeNames[i], 32);
    }
    
    // Charger stats du pet
    myPet.hunger = prefs.getInt("hunger", 50);
    myPet.happiness = prefs.getInt("happy", 100);
    myPet.health = prefs.getInt("health", 100);
    myPet.energy = prefs.getInt("energy", 100);
    myPet.weight = prefs.getInt("weight", 10);
    myPet.totalLifetimeShares = prefs.getULong("lifeshares", 0);
    myPet.bornTime = prefs.getULong("born", millis());
    myPet.isDirty = prefs.getBool("dirty", false);
    myPet.isSick = prefs.getBool("sick", false);
    sessionStartShares = myPet.totalLifetimeShares;
  }
  
  prefs.end();
}

void saveConfig() {
  prefs.begin("bitaxgotchi", false);
  
  prefs.putBool("configured", true);
  prefs.putString("ssid", config.wifiSSID);
  prefs.putString("pass", config.wifiPassword);
  prefs.putInt("num", config.numBitaxe);
  
  for(int i = 0; i < config.numBitaxe; i++) {
    prefs.putString(("ip" + String(i)).c_str(), config.bitaxeIPs[i]);
    prefs.putString(("name" + String(i)).c_str(), config.bitaxeNames[i]);
  }
  
  prefs.end();
}

void savePetState() {
  prefs.begin("bitaxgotchi", false);
  
  prefs.putInt("hunger", myPet.hunger);
  prefs.putInt("happy", myPet.happiness);
  prefs.putInt("health", myPet.health);
  prefs.putInt("energy", myPet.energy);
  prefs.putInt("weight", myPet.weight);
  prefs.putULong("lifeshares", myPet.totalLifetimeShares);
  prefs.putULong("born", myPet.bornTime);
  prefs.putBool("dirty", myPet.isDirty);
  prefs.putBool("sick", myPet.isSick);
  
  prefs.end();
}

// ============== PORTAIL WEB STYLÉ ==============

String getWebStyle() {
  return R"rawliteral(
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }
  body {
    font-family: 'Courier New', monospace;
    background: linear-gradient(135deg, #0f0c29 0%, #302b63 50%, #24243e 100%);
    color: #00ff41;
    min-height: 100vh;
    padding: 20px;
  }
  .container {
    max-width: 900px;
    margin: 0 auto;
  }
  header {
    text-align: center;
    padding: 30px 0;
    border-bottom: 2px solid #00ff41;
    margin-bottom: 30px;
  }
  h1 {
    font-size: 3em;
    color: #00ff41;
    text-shadow: 0 0 20px #00ff41;
    letter-spacing: 5px;
  }
  .subtitle {
    color: #ffa500;
    margin-top: 10px;
    font-size: 1.2em;
  }
  .card {
    background: rgba(0, 0, 0, 0.6);
    border: 2px solid #00ff41;
    border-radius: 10px;
    padding: 25px;
    margin: 20px 0;
    box-shadow: 0 0 30px rgba(0, 255, 65, 0.3);
  }
  h2 {
    color: #ffa500;
    border-bottom: 1px solid #ffa500;
    padding-bottom: 10px;
    margin-bottom: 20px;
    font-size: 1.5em;
  }
  label {
    display: block;
    color: #00ff41;
    margin: 15px 0 5px 0;
    font-weight: bold;
  }
  input, select, button {
    width: 100%;
    padding: 12px;
    margin: 5px 0;
    background: rgba(0, 0, 0, 0.8);
    border: 2px solid #00ff41;
    border-radius: 5px;
    color: #00ff41;
    font-family: 'Courier New', monospace;
    font-size: 16px;
  }
  input:focus, select:focus {
    outline: none;
    box-shadow: 0 0 15px #00ff41;
  }
  button {
    background: linear-gradient(135deg, #00ff41 0%, #00aa00 100%);
    color: #000;
    font-weight: bold;
    cursor: pointer;
    text-transform: uppercase;
    letter-spacing: 2px;
    transition: all 0.3s;
    margin-top: 10px;
  }
  button:hover {
    transform: translateY(-2px);
    box-shadow: 0 5px 20px rgba(0, 255, 65, 0.5);
  }
  .danger {
    background: linear-gradient(135deg, #ff0000 0%, #aa0000 100%);
    border-color: #ff0000;
    color: #fff;
  }
  .danger:hover {
    box-shadow: 0 5px 20px rgba(255, 0, 0, 0.5);
  }
  .status-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
    gap: 15px;
    margin: 20px 0;
  }
  .stat-box {
    background: rgba(0, 0, 0, 0.8);
    border: 2px solid #ffa500;
    border-radius: 8px;
    padding: 15px;
    text-align: center;
  }
  .stat-value {
    font-size: 2em;
    color: #00ff41;
    font-weight: bold;
    text-shadow: 0 0 10px #00ff41;
  }
  .stat-label {
    color: #ffa500;
    margin-top: 5px;
    font-size: 0.9em;
  }
  .miner-card {
    background: rgba(0, 0, 0, 0.8);
    border: 2px solid #00ff41;
    border-radius: 8px;
    padding: 15px;
    margin: 10px 0;
    display: flex;
    justify-content: space-between;
    align-items: center;
  }
  .miner-info {
    flex: 1;
  }
  .miner-name {
    color: #00ff41;
    font-size: 1.2em;
    font-weight: bold;
  }
  .miner-ip {
    color: #888;
    font-size: 0.9em;
  }
  .miner-stats {
    text-align: right;
  }
  .hashrate {
    color: #ffa500;
    font-size: 1.5em;
    font-weight: bold;
  }
  .shares {
    color: #00ff41;
    font-size: 0.9em;
  }
  .status-badge {
    display: inline-block;
    padding: 5px 15px;
    border-radius: 20px;
    font-size: 0.8em;
    font-weight: bold;
    margin: 5px 0;
  }
  .online {
    background: #00ff41;
    color: #000;
  }
  .offline {
    background: #ff0000;
    color: #fff;
  }
  .pet-stage {
    text-align: center;
    padding: 20px;
    background: rgba(0, 0, 0, 0.8);
    border: 3px solid #ffa500;
    border-radius: 15px;
    margin: 20px 0;
  }
  .stage-name {
    font-size: 2em;
    color: #ffa500;
    text-shadow: 0 0 15px #ffa500;
  }
  .progress-bar {
    width: 100%;
    height: 30px;
    background: rgba(0, 0, 0, 0.8);
    border: 2px solid #00ff41;
    border-radius: 15px;
    overflow: hidden;
    margin: 10px 0;
  }
  .progress-fill {
    height: 100%;
    background: linear-gradient(90deg, #00ff41 0%, #00aa00 100%);
    transition: width 0.3s;
    display: flex;
    align-items: center;
    justify-content: center;
    color: #000;
    font-weight: bold;
  }
  .ascii-art {
    text-align: center;
    color: #00ff41;
    font-size: 0.8em;
    line-height: 1.2;
    margin: 20px 0;
  }
  @media (max-width: 600px) {
    h1 { font-size: 2em; }
    .status-grid { grid-template-columns: 1fr; }
  }
</style>
)rawliteral";
}

String getWebHTML() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>BitaxGotchi Miner Dashboard</title>";
  html += getWebStyle();
  html += "</head><body><div class='container'>";
  
  // Header
  html += "<header>";
  html += "<div class='ascii-art'>⚡ ⛏️ ⚡</div>";
  html += "<h1>BITAXGOTCHI</h1>";
  html += "<div class='subtitle'>BITCOIN MINING PET</div>";
  html += "</header>";
  
  if(config.configured && !configMode) {
    // Pet Status
    html += "<div class='card'><h2>🐾 PET STATUS</h2>";
    html += "<div class='pet-stage'>";
    html += "<div class='stage-name'>";
    
    const char* stageName[] = {"🥚 ŒUF", "🐣 BÉBÉ", "🐥 ENFANT", "🐤 ADO", "🦅 ADULTE", "👑 ELITE", "💀 MORT"};
    html += stageName[myPet.state];
    html += "</div>";
    
    unsigned long nextLevel[] = {50000, 100000, 200000, 350000, 500000};
    if(myPet.state < STATE_ELITE) {
      unsigned long target = nextLevel[myPet.state];
      int progress = (myPet.totalLifetimeShares * 100) / target;
      html += "<div class='progress-bar'><div class='progress-fill' style='width:" + String(progress) + "%'>";
      html += String(progress) + "%</div></div>";
      html += "<small>" + String(myPet.totalLifetimeShares) + " / " + String(target) + " shares</small>";
    } else {
      html += "<div style='color:#ffa500;margin-top:10px'>⭐ MAX LEVEL ATTEINT! ⭐</div>";
    }
    html += "</div>";
    
    // Stats du pet
    html += "<div class='status-grid'>";
    html += "<div class='stat-box'><div class='stat-value'>" + String(myPet.hunger) + "%</div><div class='stat-label'>🍖 FAIM</div></div>";
    html += "<div class='stat-box'><div class='stat-value'>" + String(myPet.happiness) + "%</div><div class='stat-label'>😊 JOIE</div></div>";
    html += "<div class='stat-box'><div class='stat-value'>" + String(myPet.health) + "%</div><div class='stat-label'>❤️ SANTÉ</div></div>";
    html += "<div class='stat-box'><div class='stat-value'>" + String(myPet.energy) + "%</div><div class='stat-label'>⚡ ÉNERGIE</div></div>";
    html += "</div></div>";
    
    // Mining Stats
    html += "<div class='card'><h2>⛏️ MINING STATS</h2>";
    html += "<div class='status-grid'>";
    
    float totalHash = 0;
    for(int i = 0; i < config.numBitaxe; i++) {
      if(bitaxeStats[i].isOnline) totalHash += bitaxeStats[i].hashrate;
    }
    
    html += "<div class='stat-box'><div class='stat-value'>" + String(totalShares) + "</div><div class='stat-label'>📊 TOTAL SHARES</div></div>";
    html += "<div class='stat-box'><div class='stat-value'>" + String(totalHash, 1) + "</div><div class='stat-label'>⚡ GH/s TOTAL</div></div>";
    html += "<div class='stat-box'><div class='stat-value'>" + String(myPet.totalLifetimeShares) + "</div><div class='stat-label'>🏆 LIFETIME</div></div>";
    html += "<div class='stat-box'><div class='stat-value'>" + String(config.numBitaxe) + "</div><div class='stat-label'>🖥️ MINEURS</div></div>";
    html += "</div>";
    
    // Liste des mineurs
    for(int i = 0; i < config.numBitaxe; i++) {
      html += "<div class='miner-card'>";
      html += "<div class='miner-info'>";
      html += "<div class='miner-name'>" + String(config.bitaxeNames[i][0] ? config.bitaxeNames[i] : ("Bitaxe " + String(i+1))) + "</div>";
      html += "<div class='miner-ip'>" + String(config.bitaxeIPs[i]) + "</div>";
      html += "<span class='status-badge " + String(bitaxeStats[i].isOnline ? "online" : "offline") + "'>";
      html += bitaxeStats[i].isOnline ? "ONLINE" : "OFFLINE";
      html += "</span></div>";
      
      if(bitaxeStats[i].isOnline) {
        html += "<div class='miner-stats'>";
        html += "<div class='hashrate'>" + String(bitaxeStats[i].hashrate, 1) + " GH/s</div>";
        html += "<div class='shares'>" + String(bitaxeStats[i].shares) + " shares</div>";
        if(bitaxeStats[i].temp > 0) {
          html += "<div style='color:#ff6b6b;font-size:0.8em'>" + String(bitaxeStats[i].temp, 0) + "°C</div>";
        }
        html += "</div>";
      }
      html += "</div>";
    }
    html += "</div>";
  }
  
  // Config WiFi
  html += "<div class='card'><h2>📡 CONFIGURATION WiFi</h2>";
  html += "<form action='/savewifi' method='POST'>";
  html += "<label>SSID</label><input name='ssid' value='" + String(config.wifiSSID) + "' required>";
  html += "<label>PASSWORD</label><input type='password' name='pass' value='" + String(config.wifiPassword) + "'>";
  html += "<button type='submit'>💾 SAUVEGARDER WiFi</button></form></div>";
  
  // Config Bitaxe
  html += "<div class='card'><h2>⛏️ CONFIGURATION MINEURS</h2>";
  html += "<form action='/savebitaxe' method='POST'>";
  html += "<label>NOMBRE DE MINEURS</label>";
  html += "<input type='number' name='num' value='" + String(config.numBitaxe) + "' min='1' max='10' required>";
  html += "<div id='miners'>";
  
  for(int i = 0; i < max(1, config.numBitaxe); i++) {
    html += "<div style='margin:20px 0;padding:15px;border:1px solid #ffa500;border-radius:5px'>";
    html += "<h3 style='color:#ffa500'>MINEUR " + String(i+1) + "</h3>";
    html += "<label>NOM (optionnel)</label>";
    html += "<input name='name" + String(i) + "' value='" + String(config.bitaxeNames[i]) + "' placeholder='Bitaxe " + String(i+1) + "'>";
    html += "<label>ADRESSE IP</label>";
    html += "<input name='ip" + String(i) + "' value='" + String(config.bitaxeIPs[i]) + "' placeholder='192.168.1.100' required>";
    html += "</div>";
  }
  
  html += "</div><button type='submit'>💾 SAUVEGARDER MINEURS</button></form></div>";
  
  // Actions
  if(config.configured) {
    html += "<div class='card'><h2>⚙️ ACTIONS</h2>";
    html += "<button onclick=\"location.href='/reboot'\">🔄 REDÉMARRER</button>";
    html += "<button class='danger' onclick=\"if(confirm('Reset complet?'))location.href='/reset'\">⚠️ RESET CONFIG</button>";
    html += "</div>";
  }
  
  html += "<div style='text-align:center;margin:30px 0;color:#888'>";
  html += "<small>BitaxGotchi v2.0 | Made with ⚡ for Bitcoin Miners</small></div>";
  
  html += "</div></body></html>";
  return html;
}

void handleRoot() {
  webServer.send(200, "text/html", getWebHTML());
}

void handleSaveWiFi() {
  if(webServer.hasArg("ssid")) {
    webServer.arg("ssid").toCharArray(config.wifiSSID, 64);
    webServer.arg("pass").toCharArray(config.wifiPassword, 64);
    config.configured = true;
    saveConfig();
    
    String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>" + getWebStyle();
    html += "</head><body><div class='container'><div class='card' style='text-align:center'>";
    html += "<h1 style='color:#00ff41'>✓ WiFi SAUVEGARDÉ</h1>";
    html += "<p>Redémarrage en cours...</p>";
    html += "<script>setTimeout(()=>location.href='/',3000)</script>";
    html += "</div></div></body></html>";
    
    webServer.send(200, "text/html", html);
    delay(2000);
    ESP.restart();
  }
}

void handleSaveBitaxe() {
  if(webServer.hasArg("num")) {
    config.numBitaxe = webServer.arg("num").toInt();
    
    for(int i = 0; i < config.numBitaxe; i++) {
      String argName = "name" + String(i);
      String argIP = "ip" + String(i);
      
      if(webServer.hasArg(argName)) {
        webServer.arg(argName).toCharArray(config.bitaxeNames[i], 32);
      }
      if(webServer.hasArg(argIP)) {
        webServer.arg(argIP).toCharArray(config.bitaxeIPs[i], 16);
      }
    }
    
    saveConfig();
    
    String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>" + getWebStyle();
    html += "</head><body><div class='container'><div class='card' style='text-align:center'>";
    html += "<h1 style='color:#00ff41'>✓ MINEURS SAUVEGARDÉS</h1>";
    html += "<script>setTimeout(()=>location.href='/',2000)</script>";
    html += "</div></div></body></html>";
    
    webServer.send(200, "text/html", html);
  }
}

void handleReset() {
  prefs.begin("bitaxgotchi", false);
  prefs.clear();
  prefs.end();
  
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>" + getWebStyle();
  html += "</head><body><div class='container'><div class='card' style='text-align:center'>";
  html += "<h1 style='color:#ff0000'>⚠️ RESET COMPLET</h1>";
  html += "<p>Redémarrage...</p></div></div></body></html>";
  
  webServer.send(200, "text/html", html);
  delay(2000);
  ESP.restart();
}

void handleReboot() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'>" + getWebStyle();
  html += "</head><body><div class='container'><div class='card' style='text-align:center'>";
  html += "<h1 style='color:#ffa500'>🔄 REDÉMARRAGE</h1></div></div></body></html>";
  
  webServer.send(200, "text/html", html);
  delay(1000);
  ESP.restart();
}

void startConfigMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);
  
  dnsServer.start(53, "*", WiFi.softAPIP());
  
  webServer.on("/", handleRoot);
  webServer.on("/savewifi", HTTP_POST, handleSaveWiFi);
  webServer.on("/savebitaxe", HTTP_POST, handleSaveBitaxe);
  webServer.on("/reset", handleReset);
  webServer.on("/reboot", handleReboot);
  webServer.onNotFound(handleRoot);
  webServer.begin();
  
  configMode = true;
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(20, 40);
  tft.println("MODE CONFIG");
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 75);
  tft.println("1. Connectez au WiFi:");
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(10, 90);
  tft.println("   BitaxGotchi-Setup");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 110);
  tft.println("2. Ouvrez navigateur:");
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(10, 125);
  tft.println("   192.168.4.1");
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(10, 145);
  tft.println("Page s'ouvre auto!");
}

void startNormalMode() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifiSSID, config.wifiPassword);
  
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 70);
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN);
  tft.println("Connexion WiFi");
  tft.setTextSize(1);
  
  int attempts = 0;
  while(WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    tft.setTextColor(TFT_GREEN);
    tft.print(".");
    attempts++;
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    webServer.on("/", handleRoot);
    webServer.on("/savewifi", HTTP_POST, handleSaveWiFi);
    webServer.on("/savebitaxe", HTTP_POST, handleSaveBitaxe);
    webServer.on("/reset", handleReset);
    webServer.on("/reboot", handleReboot);
    webServer.begin();
    
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(10, 50);
    tft.println("WiFi OK!");
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 75);
    tft.print("IP: ");
    tft.setTextColor(TFT_CYAN);
    tft.println(WiFi.localIP());
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 95);
    tft.println("Web: http://" + WiFi.localIP().toString());
    delay(3000);
  } else {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(10, 70);
    tft.println("WiFi ERREUR!");
    tft.setTextSize(1);
    tft.setCursor(10, 95);
    tft.println("Mode config...");
    delay(2000);
    startConfigMode();
  }
}

// ============== BITAXE ==============

void updateBitaxeStats() {
  if(WiFi.status() != WL_CONNECTED || configMode) return;
  
  totalShares = 0;
  
  for(int i = 0; i < config.numBitaxe; i++) {
    HTTPClient http;
    
    // ENDPOINT OFFICIEL ESPMiner
    String url = "http://" + String(config.bitaxeIPs[i]) + "/api/system/info";
    
    Serial.printf("[%d] GET %s\n", i+1, url.c_str());
    
    http.begin(url);
    http.setTimeout(5000);
    
    int code = http.GET();
    Serial.printf("[%d] HTTP %d\n", i+1, code);
    
    if(code == 200) {
      String payload = http.getString();
      Serial.printf("[%d] Response: %s...\n", i+1, payload.substring(0, min(100, (int)payload.length())).c_str());
      
      StaticJsonDocument<2048> doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if(!error) {
        // CLÉS JSON OFFICIELLES ESPMiner
        bitaxeStats[i].shares = doc["sharesAccepted"] | 0;
        bitaxeStats[i].bestDiff = doc["bestDiff"] | 0;
        bitaxeStats[i].hashrate = doc["hashRate"] | 0.0;
        bitaxeStats[i].temp = doc["temp"] | 0.0;
        bitaxeStats[i].isOnline = true;
        bitaxeStats[i].lastUpdate = millis();
        
        totalShares += bitaxeStats[i].shares;
        
        Serial.printf("[%d] ✓ %lu shares, %.1f GH/s, %.0f°C\n", 
                      i+1, bitaxeStats[i].shares, bitaxeStats[i].hashrate, bitaxeStats[i].temp);
      } else {
        Serial.printf("[%d] JSON error: %s\n", i+1, error.c_str());
        bitaxeStats[i].isOnline = false;
      }
    } else {
      Serial.printf("[%d] HTTP error\n", i+1);
      bitaxeStats[i].isOnline = false;
    }
    
    http.end();
  }
  
  Serial.printf("Total: %lu shares\n", totalShares);
}

void checkForNewShares() {
  if(totalShares > lastShareCount) {
    unsigned long newShares = totalShares - lastShareCount;
    myPet.totalLifetimeShares += newShares;
    
    // Nourrir le pet
    int foodAmount = newShares * 5;
    myPet.hunger = min(100, myPet.hunger + foodAmount);
    myPet.happiness = min(100, myPet.happiness + 3);
    myPet.energy = min(100, myPet.energy + 2);
    
    // Animation
    for(int i = 0; i < 3; i++) {
      tft.fillScreen(TFT_GREEN);
      tft.setTextSize(3);
      tft.setTextColor(TFT_WHITE);
      tft.setCursor(40, 50);
      tft.printf("+%lu", newShares);
      tft.setTextSize(2);
      tft.setCursor(60, 85);
      tft.print("SHARE");
      if(newShares > 1) tft.print("S");
      tft.print("!");
      delay(250);
      tft.fillScreen(TFT_BLACK);
      delay(250);
    }
    
    lastShareCount = totalShares;
    savePetState();
  }
}

// ============== PET ==============

void initPet() {
  if(myPet.bornTime == 0 || myPet.totalLifetimeShares == 0) {
    myPet.state = STATE_EGG;
    myPet.hunger = 50;
    myPet.happiness = 100;
    myPet.health = 100;
    myPet.energy = 100;
    myPet.weight = 10;
    myPet.totalLifetimeShares = 0;
    myPet.bornTime = millis();
    myPet.isDirty = false;
    myPet.isSick = false;
    myPet.isSleeping = false;
  }
  
  updatePetState();
}

void updatePetState() {
  // Évolution basée sur shares
  if(myPet.totalLifetimeShares >= 500000 && myPet.state < STATE_ELITE) {
    myPet.state = STATE_ELITE;
    showEvolutionAnimation("ELITE!");
  } else if(myPet.totalLifetimeShares >= 350000 && myPet.state < STATE_ADULT) {
    myPet.state = STATE_ADULT;
    showEvolutionAnimation("ADULTE!");
  } else if(myPet.totalLifetimeShares >= 200000 && myPet.state < STATE_TEEN) {
    myPet.state = STATE_TEEN;
    showEvolutionAnimation("ADO!");
  } else if(myPet.totalLifetimeShares >= 100000 && myPet.state < STATE_CHILD) {
    myPet.state = STATE_CHILD;
    showEvolutionAnimation("ENFANT!");
  } else if(myPet.totalLifetimeShares >= 50000 && myPet.state < STATE_BABY) {
    myPet.state = STATE_BABY;
    showEvolutionAnimation("BEBE!");
  }
}

void updatePetStats() {
  unsigned long now = millis();
  
  // Dégradation lente
  if(now - myPet.lastFeed >= 120000) {  // 2 minutes
    myPet.hunger = max(0, myPet.hunger - 1);
    myPet.lastFeed = now;
  }
  
  if(now - myPet.lastPlay >= 180000) {  // 3 minutes
    myPet.happiness = max(0, myPet.happiness - 1);
    myPet.energy = max(0, myPet.energy - 1);
    myPet.lastPlay = now;
  }
  
  // Santé
  if(myPet.hunger < 20 || myPet.happiness < 20 || myPet.energy < 20) {
    myPet.health = max(0, myPet.health - 1);
    if(random(100) < 3) myPet.isSick = true;
  } else if(myPet.health < 100) {
    myPet.health = min(100, myPet.health + 1);
  }
  
  // Sale
  if(now - myPet.lastClean >= 600000 && random(100) < 5) {  // 10 min
    myPet.isDirty = true;
  }
  
  // Mort
  if(myPet.health <= 0 && myPet.state != STATE_DEAD) {
    myPet.state = STATE_DEAD;
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(4);
    tft.setTextColor(TFT_RED);
    tft.setCursor(70, 60);
    tft.print("R.I.P.");
    delay(3000);
  }
  
  // Sauvegarder périodiquement
  static unsigned long lastSave = 0;
  if(now - lastSave >= 300000) {  // 5 minutes
    lastSave = now;
    savePetState();
  }
}

void playWithPet() {
  if(myPet.state == STATE_DEAD) return;
  
  myPet.happiness = min(100, myPet.happiness + 15);
  myPet.energy = max(0, myPet.energy - 5);
  myPet.hunger = max(0, myPet.hunger - 3);
  
  // Animation
  tft.fillScreen(TFT_BLACK);
  for(int i = 0; i < 8; i++) {
    int x = random(50, 270);
    int y = random(30, 140);
    int size = random(10, 25);
    uint16_t color = tft.color565(random(150, 255), random(150, 255), random(150, 255));
    tft.fillCircle(x, y, size, color);
    delay(100);
  }
  delay(500);
}

void cleanPet() {
  if(myPet.state == STATE_DEAD) return;
  
  myPet.isDirty = false;
  myPet.happiness = min(100, myPet.happiness + 10);
  myPet.lastClean = millis();
  
  // Animation
  tft.fillScreen(TFT_CYAN);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(60, 60);
  tft.print("PROPRE!");
  delay(1000);
}

void giveMedicine() {
  if(myPet.state == STATE_DEAD || !myPet.isSick) return;
  
  myPet.isSick = false;
  myPet.health = min(100, myPet.health + 30);
  
  // Animation
  for(int i = 0; i < 4; i++) {
    tft.fillScreen(i % 2 ? TFT_RED : TFT_WHITE);
    delay(200);
  }
  tft.fillScreen(TFT_GREEN);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(50, 60);
  tft.print("GUERI!");
  delay(1000);
}

void showEvolutionAnimation(const char* stage) {
  tft.fillScreen(TFT_BLACK);
  
  for(int i = 0; i < 5; i++) {
    tft.fillScreen(TFT_YELLOW);
    delay(150);
    tft.fillScreen(TFT_BLACK);
    delay(150);
  }
  
  tft.setTextSize(3);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(30, 40);
  tft.print("EVOLUTION!");
  
  tft.setTextSize(4);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(40, 80);
  tft.print(stage);
  
  delay(3000);
  savePetState();
}

// ============== AFFICHAGE ==============

void drawMainScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, 320, 170, TFT_CYAN);
  
  // Barres de stats
  int barY = 5;
  int barH = 6;
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  
  // Faim
  tft.setCursor(5, barY);
  tft.print("FAIM");
  tft.drawRect(45, barY, 70, barH, TFT_WHITE);
  tft.fillRect(46, barY+1, (myPet.hunger * 68) / 100, barH-2, TFT_YELLOW);
  
  // Joie
  tft.setCursor(120, barY);
  tft.print("JOIE");
  tft.drawRect(155, barY, 70, barH, TFT_WHITE);
  tft.fillRect(156, barY+1, (myPet.happiness * 68) / 100, barH-2, TFT_PINK);
  
  // Santé
  tft.setCursor(230, barY);
  tft.print("SANTE");
  tft.drawRect(275, barY, 40, barH, TFT_WHITE);
  tft.fillRect(276, barY+1, (myPet.health * 38) / 100, barH-2, TFT_GREEN);
  
  barY += 10;
  
  // Énergie
  tft.setCursor(5, barY);
  tft.print("NRG");
  tft.drawRect(35, barY, 50, barH, TFT_WHITE);
  tft.fillRect(36, barY+1, (myPet.energy * 48) / 100, barH-2, TFT_CYAN);
  
  // Stage
  const char* stageName[] = {"OEF", "BEBE", "ENFANT", "ADO", "ADULTE", "ELITE", "MORT"};
  tft.setCursor(90, barY);
  tft.setTextColor(TFT_YELLOW);
  tft.print(stageName[myPet.state]);
  
  // Next level
  unsigned long nextLevel[] = {50000, 100000, 200000, 350000, 500000};
  if(myPet.state < STATE_ELITE) {
    unsigned long target = nextLevel[myPet.state];
    int progress = (myPet.totalLifetimeShares * 100) / target;
    tft.setCursor(140, barY);
    tft.setTextColor(TFT_CYAN);
    tft.printf("Next:%d%%", progress);
  } else {
    tft.setCursor(140, barY);
    tft.setTextColor(TFT_GOLD);
    tft.print("MAX!");
  }
  
  // Pet (centre)
  int petX = 160;
  int petY = 70;
  int petSize = 35;
  
  if(myPet.state == STATE_DEAD) {
    tft.setTextSize(4);
    tft.setTextColor(TFT_RED);
    tft.setCursor(120, 55);
    tft.print("RIP");
  } else {
    // Couleur selon stage
    uint16_t petColor = TFT_GREEN;
    if(myPet.state == STATE_EGG) petColor = TFT_WHITE;
    else if(myPet.state == STATE_BABY) petColor = 0xFFE0;  // Jaune
    else if(myPet.state == STATE_CHILD) petColor = TFT_CYAN;
    else if(myPet.state == STATE_TEEN) petColor = TFT_BLUE;
    else if(myPet.state == STATE_ADULT) petColor = TFT_GREEN;
    else if(myPet.state == STATE_ELITE) petColor = TFT_GOLD;
    
    tft.fillCircle(petX, petY, petSize, petColor);
    
    // Yeux
    if(myPet.state != STATE_EGG) {
      tft.fillCircle(petX-12, petY-8, 6, TFT_WHITE);
      tft.fillCircle(petX+12, petY-8, 6, TFT_WHITE);
      tft.fillCircle(petX-12, petY-8, 3, TFT_BLACK);
      tft.fillCircle(petX+12, petY-8, 3, TFT_BLACK);
      
      // Bouche
      if(myPet.happiness > 50) {
        tft.drawArc(petX, petY+5, 18, 12, 0, 180, TFT_BLACK, TFT_BLACK);
      } else {
        tft.drawArc(petX, petY+18, 18, 12, 180, 360, TFT_BLACK, TFT_BLACK);
      }
    }
    
    // Indicateurs
    if(myPet.isDirty) {
      tft.fillCircle(petX+28, petY+20, 6, 0x7BEF);  // Gris
      tft.setTextSize(1);
      tft.setTextColor(TFT_BLACK);
      tft.setCursor(petX+25, petY+17);
      tft.print("!");
    }
    
    if(myPet.isSick) {
      tft.fillCircle(petX-28, petY-20, 6, TFT_RED);
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE);
      tft.setCursor(petX-31, petY-23);
      tft.print("+");
    }
  }
  
  // Shares info
  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(5, 145);
  tft.printf("Session: %lu", totalShares - sessionStartShares);
  
  tft.setCursor(5, 155);
  tft.setTextColor(TFT_YELLOW);
  tft.printf("Lifetime: %lu", myPet.totalLifetimeShares);
  
  // Bitaxe status
  float totalHash = 0;
  int onlineCount = 0;
  for(int i = 0; i < config.numBitaxe; i++) {
    if(bitaxeStats[i].isOnline) {
      totalHash += bitaxeStats[i].hashrate;
      onlineCount++;
    }
  }
  
  tft.setCursor(180, 145);
  tft.setTextColor(TFT_GREEN);
  tft.printf("%.1f GH/s", totalHash);
  
  tft.setCursor(180, 155);
  tft.setTextColor(onlineCount == config.numBitaxe ? TFT_GREEN : TFT_YELLOW);
  tft.printf("%d/%d Online", onlineCount, config.numBitaxe);
  
  // Boutons
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(5, 162);
  tft.print("[1:Menu]");
  tft.setCursor(120, 162);
  tft.print("[2:Stats]");
}

void drawStatsScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN);
  
  int y = 5;
  tft.setCursor(5, y); y += 12;
  tft.print("===== STATISTIQUES =====");
  
  tft.setTextColor(TFT_WHITE);
  
  const char* stageNames[] = {"Oeuf", "Bebe", "Enfant", "Ado", "Adulte", "ELITE", "Mort"};
  tft.setCursor(5, y); y += 12;
  tft.printf("Stade: %s", stageNames[myPet.state]);
  
  tft.setCursor(5, y); y += 12;
  tft.printf("Faim: %d/100", myPet.hunger);
  
  tft.setCursor(5, y); y += 12;
  tft.printf("Joie: %d/100", myPet.happiness);
  
  tft.setCursor(5, y); y += 12;
  tft.printf("Sante: %d/100", myPet.health);
  
  tft.setCursor(5, y); y += 12;
  tft.printf("Energie: %d/100", myPet.energy);
  
  tft.setCursor(5, y); y += 12;
  tft.printf("Poids: %d", myPet.weight);
  
  tft.setCursor(5, y); y += 15;
  tft.setTextColor(TFT_YELLOW);
  tft.print("===== MINING =====");
  
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(5, y); y += 12;
  tft.printf("Lifetime: %lu", myPet.totalLifetimeShares);
  
  tft.setCursor(5, y); y += 12;
  tft.printf("Session: %lu", totalShares - sessionStartShares);
  
  tft.setCursor(5, y); y += 12;
  tft.printf("Total: %lu", totalShares);
  
  unsigned long nextLevel[] = {50000, 100000, 200000, 350000, 500000};
  if(myPet.state < STATE_ELITE) {
    unsigned long target = nextLevel[myPet.state];
    unsigned long needed = target - myPet.totalLifetimeShares;
    tft.setCursor(5, y); y += 12;
    tft.setTextColor(TFT_CYAN);
    tft.printf("Next: %lu shares", needed);
  }
  
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(5, 158);
  tft.print("[Retour avec BTN1 ou BTN2]");
}

void drawMenuScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(100, 10);
  tft.print("MENU");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  
  int y = 40;
  tft.setCursor(20, y); y += 20;
  tft.print("BTN1: Jouer avec le pet");
  
  tft.setCursor(20, y); y += 20;
  if(myPet.isDirty) {
    tft.setTextColor(TFT_YELLOW);
    tft.print("BTN2: Nettoyer [!]");
  } else {
    tft.setTextColor(TFT_WHITE);
    tft.print("BTN2: Nettoyer");
  }
  
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, y); y += 20;
  if(myPet.isSick) {
    tft.setTextColor(TFT_RED);
    tft.print("BTN1+2: Soigner [!!!]");
  } else {
    tft.setTextColor(TFT_WHITE);
    tft.print("BTN1+2: Soigner");
  }
  
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(20, 140);
  tft.print("Web: http://");
  tft.println(WiFi.localIP());
  
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 160);
  tft.print("[Retour: BTN1 ou BTN2]");
}

// ============== BOUTONS ==============

void handleButtons() {
  static bool btn1Pressed = false;
  static bool btn2Pressed = false;
  static unsigned long bothPressedTime = 0;
  
  bool btn1 = !digitalRead(BUTTON_1);
  bool btn2 = !digitalRead(BUTTON_2);
  
  // Les deux boutons ensemble
  if(btn1 && btn2) {
    if(bothPressedTime == 0) {
      bothPressedTime = millis();
    } else if(millis() - bothPressedTime > 1000) {
      // Action: soigner
      if(currentScreen == 2) {  // Menu
        giveMedicine();
        currentScreen = 0;
        needsRedraw = true;
      }
      bothPressedTime = 0;
    }
  } else {
    bothPressedTime = 0;
  }
  
  // BTN1
  if(btn1 && !btn1Pressed && !btn2) {
    btn1Pressed = true;
    
    if(currentScreen == 0) {
      currentScreen = 2;  // Aller au menu
      needsRedraw = true;
    } else if(currentScreen == 1) {
      currentScreen = 0;  // Retour
      needsRedraw = true;
    } else if(currentScreen == 2) {
      playWithPet();
      currentScreen = 0;
      needsRedraw = true;
    }
  } else if(!btn1) {
    btn1Pressed = false;
  }
  
  // BTN2
  if(btn2 && !btn2Pressed && !btn1) {
    btn2Pressed = true;
    
    if(currentScreen == 0) {
      currentScreen = 1;  // Stats
      needsRedraw = true;
    } else if(currentScreen == 1) {
      currentScreen = 0;  // Retour
      needsRedraw = true;
    } else if(currentScreen == 2) {
      cleanPet();
      currentScreen = 0;
      needsRedraw = true;
    }
  } else if(!btn2) {
    btn2Pressed = false;
  }
}

// ============== SETUP ==============

void setup() {
  Serial.begin(115200);
  
  pinMode(POWER_ON_PIN, OUTPUT);
  digitalWrite(POWER_ON_PIN, HIGH);
  
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  
  // Splash
  tft.fillScreen(TFT_BLACK);
  for(int i = 0; i < 3; i++) {
    tft.fillScreen(i % 2 ? TFT_BLACK : TFT_CYAN);
    delay(200);
  }
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 40);
  tft.print("BITAXGOTCHI");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(60, 75);
  tft.print("Bitcoin Mining Pet");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(80, 120);
  tft.print("Loading...");
  
  delay(2000);
  
  loadConfig();
  
  // Reset config si BTN1+BTN2
  if(!digitalRead(BUTTON_1) && !digitalRead(BUTTON_2)) {
    tft.fillScreen(TFT_RED);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(30, 70);
    tft.print("RESET CONFIG");
    delay(3000);
    
    if(!digitalRead(BUTTON_1) && !digitalRead(BUTTON_2)) {
      prefs.begin("bitaxgotchi", false);
      prefs.clear();
      prefs.end();
      
      tft.fillScreen(TFT_GREEN);
      tft.setCursor(50, 70);
      tft.print("RESET OK!");
      delay(2000);
      ESP.restart();
    }
  }
  
  if(!config.configured) {
    startConfigMode();
  } else {
    startNormalMode();
    initPet();
    updateBitaxeStats();
    lastShareCount = totalShares;
    sessionStartShares = myPet.totalLifetimeShares;
  }
}

// ============== LOOP ==============

void loop() {
  unsigned long now = millis();
  
  if(configMode) {
    dnsServer.processNextRequest();
  }
  
  webServer.handleClient();
  
  if(!configMode) {
    // Update Bitaxe
    if(now - lastShareCheck >= 10000) {
      lastShareCheck = now;
      updateBitaxeStats();
      checkForNewShares();
      updatePetState();
      needsRedraw = true; // Redessiner après mise à jour
    }
    
    // Update Pet
    if(now - lastPetUpdate >= 60000) {
      lastPetUpdate = now;
      updatePetStats();
      needsRedraw = true;
    }
    
    handleButtons();
    
    // Redessiner seulement si nécessaire
    bool screenChanged = (currentScreen != lastScreen);
    bool timeToRedraw = (now - lastScreenRedraw >= 5000); // Toutes les 5 secondes
    
    if(needsRedraw || screenChanged || timeToRedraw) {
      // Affichage selon l'écran
      if(currentScreen == 0) {
        drawMainScreen();
      } else if(currentScreen == 1) {
        drawStatsScreen();
      } else if(currentScreen == 2) {
        drawMenuScreen();
      }
      
      lastScreen = currentScreen;
      lastScreenRedraw = now;
      needsRedraw = false;
    }
  }
  
  delay(100);
}
