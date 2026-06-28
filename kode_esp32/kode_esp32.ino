// =====================================================
// GPS TRACKER — Blynk + Fonnte WhatsApp
// Board   : ESP32
// Modem   : SIM7600
// Updated : Fonnte WA (telegram dihapus)
// =====================================================

#define BLYNK_TEMPLATE_ID "TMPL6ujtg4BdO"
#define BLYNK_TEMPLATE_NAME "GPS TRACK"
#define BLYNK_AUTH_TOKEN "szWjV74ZQEVeKoRojSZAC8s-pTA97WxV"

#define BLYNK_HEARTBEAT 60
#define BLYNK_PRINT Serial

#define TINY_GSM_RX_BUFFER 2048
#define TINY_GSM_MODEM_SIM7600

#include <TinyGsmClient.h>
#include <BlynkSimpleTinyGSM.h>
#include <math.h>

// =====================================================
// KONFIGURASI
// =====================================================
char apn[] = "internet";
const char FONNTE_TOKEN[] = "D66safKZqTvQfgEBiYCD";
const char FONNTE_TARGET[] = "6289653249118";

// =====================================================
// PIN
// =====================================================
#define MODEM_RX 16
#define MODEM_TX 17
#define MODEM_PWRKEY 4
#define RELAY_PIN 25
#define BUZZER_PIN 33

// =====================================================
// OBJEK
// =====================================================
HardwareSerial SerialAT(2);
TinyGsm modem(SerialAT);
TinyGsmClient blynkClient(modem, 0);
BlynkTimer timer;

// =====================================================
// STATE GPS
// =====================================================
double gpsLat = 0;
double gpsLon = 0;
bool gpsValid = false;

// =====================================================
// STATE GEOFENCE
// =====================================================
bool geoActive = false;
bool alertSent = false;
double latCenter = 0;
double lonCenter = 0;
float radius = 5.0;

// =====================================================
// STATE FONNTE (antrian kirim WA)
// =====================================================
bool sendPending = false;
bool gpsQueryRunning = false;
String pendingMsg = "";

// =====================================================
// HAVERSINE — Jarak dalam meter
// =====================================================
double distanceMeter(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0;
  double dLat = (lat2 - lat1) * PI / 180.0;
  double dLon = (lon2 - lon1) * PI / 180.0;
  double a = sin(dLat / 2) * sin(dLat / 2)
             + cos(lat1 * PI / 180.0) * cos(lat2 * PI / 180.0)
                 * sin(dLon / 2) * sin(dLon / 2);
  return R * 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
}

// =====================================================
// FLUSH SERIAL AT
// =====================================================
void flushSerialAT() {
  unsigned long t = millis();
  while (millis() - t < 100) {
    while (SerialAT.available()) SerialAT.read();
    delay(1);
  }
}

// =====================================================
// KIRIM AT COMMAND
// =====================================================
String sendAT(String cmd, int timeout = 1000) {
  flushSerialAT();
  SerialAT.println(cmd);
  String resp = "";
  unsigned long t = millis();
  while (millis() - t < timeout) {
    while (SerialAT.available()) resp += (char)SerialAT.read();
    delay(5);
  }
  Serial.println("[AT] " + cmd + " => " + resp);
  return resp;
}

// =====================================================
// BACA GPS — AT+CGPSINFO
// Format: ddmm.mmmm,N/S,dddmm.mmmm,E/W,...
// =====================================================
bool getGPS() {
  gpsValid = false;

  String resp = sendAT("AT+CGPSINFO", 3000);

  int idx = resp.indexOf("+CGPSINFO:");
  if (idx == -1) {
    Serial.println("[GPS] Tidak ada respons CGPSINFO");
    return false;
  }

  String data = resp.substring(idx + 10);
  data.trim();
  Serial.println("[GPS] RAW: " + data);

  // Kalau data kosong / belum fix
  if (data.startsWith(",") || data.length() < 10) {
    Serial.println("[GPS] Belum fix");
    return false;
  }

  char buf[200];
  data.toCharArray(buf, sizeof(buf));
  char* token;

  // ddmm.mmmm
  token = strtok(buf, ",");
  if (!token) return false;
  String latStr = String(token);

  // N / S
  token = strtok(NULL, ",");
  if (!token) return false;
  String latDir = String(token);

  // dddmm.mmmm
  token = strtok(NULL, ",");
  if (!token) return false;
  String lonStr = String(token);

  // E / W
  token = strtok(NULL, ",");
  if (!token) return false;
  String lonDir = String(token);

  if (latStr.length() < 4 || lonStr.length() < 5) {
    Serial.println("[GPS] Data invalid");
    return false;
  }

  // Konversi ddmm.mmmm → decimal degree
  double latDeg = latStr.substring(0, 2).toDouble();
  double latMin = latStr.substring(2).toDouble();
  gpsLat = latDeg + (latMin / 60.0);
  if (latDir == "S") gpsLat = -gpsLat;

  double lonDeg = lonStr.substring(0, 3).toDouble();
  double lonMin = lonStr.substring(3).toDouble();
  gpsLon = lonDeg + (lonMin / 60.0);
  if (lonDir == "W") gpsLon = -gpsLon;

  gpsValid = true;
  Serial.printf("[GPS] FIX OK | LAT: %.6f | LON: %.6f\n", gpsLat, gpsLon);
  return true;
}

// =====================================================
// JADWALKAN KIRIM WA via Fonnte
// =====================================================
void sendFonnte(double lat, double lon, String pesan) {
  pendingMsg = pesan
               + "\nhttps://maps.google.com/?q="
               + String(lat, 6) + "," + String(lon, 6);
  sendPending = true;
  Serial.println("[Fonnte] Dijadwalkan: " + pendingMsg);
}

// =====================================================
// PROSES ANTRIAN KIRIM WA — via AT HTTP SSL
// =====================================================
void processFonnteQueue() {
  if (!sendPending) return;
  if (!modem.isGprsConnected()) {
    Serial.println("[Fonnte] GPRS tidak terhubung, tunda...");
    return;
  }

  Serial.println("[Fonnte] Mengirim WA...");

  // Pastikan HTTP bersih sebelum mulai
  sendAT("AT+HTTPTERM", 1000);
  delay(500);

  sendAT("AT+HTTPINIT", 2000);

  // SIM7600: SSL otomatis aktif dari https:// di URL
  sendAT("AT+HTTPPARA=\"CID\",1", 1000);
  sendAT("AT+HTTPPARA=\"URL\",\"https://api.fonnte.com/send\"", 3000);
  sendAT("AT+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\"", 2000);

  // Fonnte pakai Authorization header — BUKAN token di body
  sendAT("AT+HTTPPARA=\"USERDATA\",\"Authorization: " + String(FONNTE_TOKEN) + "\"", 2000);

  // Payload hanya target & message, tanpa token
  String payload = "target=" + String(FONNTE_TARGET)
                   + "&message=" + pendingMsg;

  // URL encode karakter khusus
  payload.replace("\n", "%0A");
  payload.replace(" ", "+");
  payload.replace("📍", "");  // hapus emoji agar tidak rusak encoding
  payload.replace("⚠️", "");

  String httpDataCmd = "AT+HTTPDATA=" + String(payload.length()) + ",10000";
  String dataResp = sendAT(httpDataCmd, 3000);

  // Tunggu prompt DOWNLOAD sebelum kirim payload
  if (dataResp.indexOf("DOWNLOAD") == -1) {
    Serial.println("[Fonnte] Tidak dapat prompt DOWNLOAD, abort");
    sendAT("AT+HTTPTERM", 1000);
    return;
  }

  delay(200);
  SerialAT.print(payload);
  delay(2000);

  // Kirim POST dan parse panjang respons dari +HTTPACTION
  String actionResp = sendAT("AT+HTTPACTION=1", 25000);

  // Ambil panjang data dari: +HTTPACTION: 1,200,<len>
  int httpLen = 0;
  int haIdx = actionResp.indexOf("+HTTPACTION:");
  if (haIdx != -1) {
    // Cari koma ketiga
    int c1 = actionResp.indexOf(",", haIdx);
    int c2 = (c1 != -1) ? actionResp.indexOf(",", c1 + 1) : -1;
    if (c2 != -1) {
      int eol = actionResp.indexOf("\n", c2);
      String lenStr = actionResp.substring(c2 + 1, eol);
      lenStr.trim();
      httpLen = lenStr.toInt();
    }
  }

  Serial.printf("[Fonnte] HTTP response length: %d\n", httpLen);

  String resp = "";
  if (httpLen > 0) {
    resp = sendAT("AT+HTTPREAD=0," + String(httpLen), 5000);
  } else {
    // Fallback: coba baca tanpa panjang (kalau modem support)
    resp = sendAT("AT+HTTPREAD=0,512", 5000);
  }

  sendAT("AT+HTTPTERM", 1000);

  Serial.println("[Fonnte] Respons: " + resp);

  // Cek respons JSON dari Fonnte: {"status":true} = sukses
  if (resp.indexOf("\"status\":true") != -1) {
    Serial.println("[Fonnte] ✅ WA berhasil dikirim");
    sendPending = false;
    pendingMsg = "";
  } else if (actionResp.indexOf("+HTTPACTION: 1,200") == -1) {
    Serial.println("[Fonnte] ❌ Gagal (bukan HTTP 200), akan retry...");
  } else {
    // HTTP 200 tapi status false — token/target salah, jangan retry terus
    Serial.println("[Fonnte] ⚠️ HTTP 200 tapi Fonnte tolak: " + resp);
    Serial.println("[Fonnte] Cek token & nomor target!");
    sendPending = false;  // stop retry, bukan masalah koneksi
    pendingMsg = "";
  }
}

// =====================================================
// KIRIM DATA GPS ke Blynk + cek Geofence
// =====================================================
void sendData() {

  gpsQueryRunning = true;

  Serial.println("[Data] Query GPS...");

  bool gpsOk = getGPS();

  gpsQueryRunning = false;

  if (!gpsOk) {

    Serial.println("[Data] GPS belum lock, skip");

    return;
  }

  Serial.printf("[Data] LAT: %.6f | LON: %.6f | Signal: %d\n",
                gpsLat, gpsLon, modem.getSignalQuality());

  // ================================
  // KIRIM DATA KE BLYNK
  // ================================

  Blynk.virtualWrite(V4, gpsLat);
  Blynk.virtualWrite(V5, gpsLon);

  Blynk.virtualWrite(V6, modem.getSignalQuality());

  Blynk.virtualWrite(V7, geoActive);

  Blynk.virtualWrite(V8, modem.isGprsConnected());

  // ================================
  // GEOFENCE
  // ================================

  if (geoActive) {

    double jarak =
      distanceMeter(
        gpsLat,
        gpsLon,
        latCenter,
        lonCenter);

    Serial.printf(
      "[Geo] Jarak dari center: %.2f meter\n",
      jarak);

    if (jarak > radius && !alertSent) {

      sendFonnte(
        gpsLat,
        gpsLon,
        "⚠️ Motor keluar area geofence!");

      Blynk.logEvent(
        "maling",
        "Motor keluar area!");

      alertSent = true;
    }

    if (jarak <= radius) {
      alertSent = false;
    }
  }
}

// =====================================================
// CEK & RECONNECT KONEKSI
// =====================================================
void checkConnection() {
  static unsigned long lastGprsReconnect = 0;
  static unsigned long lastBlynkReconnect = 0;

  if (!modem.isGprsConnected()) {
    if (millis() - lastGprsReconnect > 30000) {
      lastGprsReconnect = millis();
      Serial.println("[Net] Reconnect GPRS...");
      if (!modem.isNetworkConnected()) modem.waitForNetwork(10000L);
      modem.gprsConnect(apn, "", "");
      delay(500);
      sendAT("AT+CGPS=1", 2000);
      Serial.print("[Net] IP: ");
      Serial.println(modem.localIP());
    }
  }

  if (!Blynk.connected() && !gpsQueryRunning) {
    if (millis() - lastBlynkReconnect > 60000) {
      lastBlynkReconnect = millis();
      Serial.println("[Net] Reconnect Blynk...");
      Blynk.connect(5000L);
    }
  }
}

// =====================================================
// BLYNK — V0: Toggle Geofence
// =====================================================
BLYNK_WRITE(V0) {
  geoActive = param.asInt();

  if (geoActive && gpsValid) {

    latCenter = gpsLat;
    lonCenter = gpsLon;
    alertSent = false;

    Serial.printf(
      "[Geo] AKTIF | Center: %.6f %.6f\n",
      latCenter,
      lonCenter);
  }

  else if (!geoActive) {

    Serial.println("[Geo] NONAKTIF");

  }

  else {

    Serial.println(
      "[Geo] GPS belum lock!");
  }

  // =========================
  // SYNC STATUS KE WEB
  // =========================

  Blynk.virtualWrite(V7, geoActive);
}

// =====================================================
// BLYNK — V1: Buzzer | V2: Relay
// =====================================================
BLYNK_WRITE(V1) {
  digitalWrite(BUZZER_PIN, param.asInt());
}
BLYNK_WRITE(V2) {
  digitalWrite(RELAY_PIN, !param.asInt());
}

// =====================================================
// BLYNK — V3: Kirim lokasi via WA
// =====================================================
BLYNK_WRITE(V3) {
  if (param.asInt() == 0) return;
  if (gpsValid) {
    sendFonnte(gpsLat, gpsLon, "📍 Lokasi Motor:");
  } else {
    Serial.println("[V3] GPS belum lock!");
    Blynk.logEvent("maling", "GPS belum lock! Lokasi tidak tersedia.");
  }
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MODEM_PWRKEY, OUTPUT);

  digitalWrite(RELAY_PIN, HIGH);  // Relay default OFF (active LOW)
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(1000);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(5000);

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  Serial.println("=== RESTART MODEM ===");
  modem.restart();
  delay(3000);

  // SSL Config
  sendAT("AT+CSSLCFG=\"sslversion\",0,3", 2000);
  sendAT("AT+CSSLCFG=\"authmode\",0,0", 2000);

  // Aktifkan GPS
  sendAT("AT+CGPS=1", 2000);
  delay(2000);
  sendAT("AT+CGPS?", 2000);

  // Koneksi jaringan
  Serial.println("=== TUNGGU JARINGAN ===");
  modem.waitForNetwork(60000L);

  Serial.println("=== CONNECT GPRS ===");
  modem.gprsConnect(apn, "", "");
  Serial.print("IP : ");
  Serial.println(modem.localIP());
  Serial.print("SQ : ");
  Serial.println(modem.getSignalQuality());

  // Koneksi Blynk
  Serial.println("=== CONNECT BLYNK ===");
  Blynk.config(modem, BLYNK_AUTH_TOKEN);
  Blynk.connect(10000L);

  // Timer
  timer.setInterval(30000L, sendData);           // Update GPS tiap 30 detik
  timer.setInterval(60000L, checkConnection);    // Cek koneksi tiap 60 detik
  timer.setInterval(5000L, processFonnteQueue);  // Cek antrian WA tiap 5 detik

  Serial.println("=== SISTEM SIAP ===");
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  Blynk.run();
  timer.run();
  delay(1);
}
