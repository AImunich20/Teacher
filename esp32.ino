/*************** BLYNK ****************/
#define BLYNK_TEMPLATE_ID   "TMPL6omEKuLcS"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN    "JPMDphcRk51qUVzCl95tWOasDLr5IEtz"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <HardwareSerial.h>
#include <BlynkSimpleEsp32.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "MaybeYouOK";
char pass[] = "natthanathron";

/*************** SERIAL ****************/
#define RXD2 16
#define TXD2 17
HardwareSerial SerialESP(2);   // UART2

/*************** TIMER *****************/
BlynkTimer timer;

/*************** COMMAND ***************/
int pumpCommand[2]       = {0, 0};
int shinyCommand[4]      = {0, 0, 0, 0};

int lastPumpCommand[2]   = {-1, -1};
int lastShinyCommand[4]  = {-1, -1, -1, -1};

/*************** PH ********************/
float pH_Value = 0;

/*************** SEND TO MEGA **********/
void sendToMega(const char* tag, int *command, int len) {

  Serial.print("[SEND] ");
  Serial.print(tag);
  Serial.print(" -> ");
  for (int i = 0; i < len; i++) {
    Serial.print(command[i]);
    if (i < len - 1) Serial.print(",");
  }
  Serial.println();

  SerialESP.print("<");
  SerialESP.print(tag);
  SerialESP.print(":");
  for (int i = 0; i < len; i++) {
    SerialESP.print(command[i]);
    if (i < len - 1) SerialESP.print(",");
  }
  SerialESP.println(">");
}

/*************** CHECK & SEND **********/
void checkAndSend() {

  bool pumpChanged  = false;
  bool shinyChanged = false;

  for (int i = 0; i < 2; i++) {
    if (pumpCommand[i] != lastPumpCommand[i]) {
      lastPumpCommand[i] = pumpCommand[i];
      pumpChanged = true;
    }
  }

  for (int i = 0; i < 4; i++) {
    if (shinyCommand[i] != lastShinyCommand[i]) {
      lastShinyCommand[i] = shinyCommand[i];
      shinyChanged = true;
    }
  }

  if (pumpChanged)  sendToMega("PUMP",  pumpCommand,  2);
  if (shinyChanged) sendToMega("SHINY", shinyCommand, 4);
}

/*************** BLYNK INPUT ***********/
// Pump
BLYNK_WRITE(V0) { pumpCommand[0] = param.asInt(); }
BLYNK_WRITE(V1) { pumpCommand[1] = param.asInt(); }

// Shiny
BLYNK_WRITE(V2) { shinyCommand[0] = param.asInt(); }
BLYNK_WRITE(V3) { shinyCommand[1] = param.asInt(); }
BLYNK_WRITE(V4) { shinyCommand[2] = param.asInt(); }
BLYNK_WRITE(V5) { shinyCommand[3] = param.asInt(); }

/*************** READ PH FROM MEGA *****/
void readPHfromMega() {
  static String buffer = "";

  while (SerialESP.available()) {
    char c = SerialESP.read();

    if (c == '<') {
      buffer = "<";
    }
    else if (c == '>') {
      buffer += ">";
      if (buffer.startsWith("<PH:")) {
        pH_Value = buffer.substring(4, buffer.length() - 1).toFloat();
        Serial.print("[ESP] pH = ");
        Serial.println(pH_Value, 2);
        Blynk.virtualWrite(V10, pH_Value);
      }
      buffer = "";
    }
    else {
      buffer += c;
      if (buffer.length() > 30) buffer = "";
    }
  }
}

/*************** KEEP ALIVE ************/
void sendAlways() {
  sendToMega("PUMP",  pumpCommand,  2);
  sendToMega("SHINY", shinyCommand, 4);
}

/*************** SETUP *****************/
void setup() {
  Serial.begin(115200);
  SerialESP.begin(115200, SERIAL_8N1, RXD2, TXD2);

  Blynk.begin(auth, ssid, pass);

  timer.setInterval(1000L,  checkAndSend);   // ส่งเมื่อเปลี่ยน
  timer.setInterval(2000L, sendAlways);     // keep alive
  timer.setInterval(4000L,  readPHfromMega); // อ่าน pH

  Serial.println("ESP32 Started");
}

/*************** LOOP ******************/
void loop() {
  Blynk.run();
  timer.run();
}
