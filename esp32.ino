#include <HardwareSerial.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

/* ================== BLYNK ================== */
#define BLYNK_TEMPLATE_ID   "TMPL6omEKuLcS"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN    "JPMDphcRk51qUVzCl95tWOasDLr5IEtz"

#define BLYNK_PRINT Serial
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Alex";
char pass[] = "1q2w3e4r";

/* ================== SERIAL ================== */
HardwareSerial SerialESP(2);

/* ================== TIMER ================== */
BlynkTimer timer;

/* ================== COMMAND ================== */
int pumpCommand[2]      = {0, 0};
int shinyCommand[2]     = {0, 0};

int lastPumpCommand[2]  = {-1, -1};
int lastShinyCommand[2] = {-1, -1};

/* ================== SEND FUNCTION ================== */
void sendToMega(const char* tag, int *command, int len) {
  SerialESP.print("<");
  SerialESP.print(tag);
  SerialESP.print(":");

  for (int i = 0; i < len; i++) {
    SerialESP.print(command[i]);
    if (i < len - 1) SerialESP.print(",");
  }
  SerialESP.println(">");
}

/* ================== CHECK & SEND ================== */
void checkAndSend() {
  bool pumpChanged = false;
  bool shinyChanged = false;

  for (int i = 0; i < 2; i++) {
    if (pumpCommand[i] != lastPumpCommand[i]) {
      lastPumpCommand[i] = pumpCommand[i];
      pumpChanged = true;
    }
    if (shinyCommand[i] != lastShinyCommand[i]) {
      lastShinyCommand[i] = shinyCommand[i];
      shinyChanged = true;
    }
  }

  if (pumpChanged)  sendToMega("PUMP", pumpCommand, 2);
  if (shinyChanged) sendToMega("SHINY", shinyCommand, 2);
}

/* ================== BLYNK INPUT ================== */
// Pump
BLYNK_WRITE(V0) { pumpCommand[0] = param.asInt(); }
BLYNK_WRITE(V1) { pumpCommand[1] = param.asInt(); }

// Shiny
BLYNK_WRITE(V2) { shinyCommand[0] = param.asInt(); }
BLYNK_WRITE(V3) { shinyCommand[1] = param.asInt(); }

/* ================== SETUP ================== */
void setup() {
  Serial.begin(115200);

  SerialESP.begin(9600, SERIAL_8N1, 16, 17);

  Blynk.begin(auth, ssid, pass);

  timer.setInterval(200L, checkAndSend);  // เช็กทุก 200ms

  Serial.println("ESP32 Started");
}

/* ================== LOOP ================== */
void loop() {
  Blynk.run();
  timer.run();
}
