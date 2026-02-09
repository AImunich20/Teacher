/*************** BLYNK ****************/
#define BLYNK_TEMPLATE_ID   "TMPL6omEKuLcS"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN    "JPMDphcRk51qUVzCl95tWOasDLr5IEtz"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <HardwareSerial.h>
#include <BlynkSimpleEsp32.h>

#include <HTTPClient.h>

String GAS_URL = "https://script.google.com/macros/s/AKfycbyp_5ekqHvcOATMBxkFcGAutoPuE2MAC6C6dCHg85jwYEx6_F1XGJeby1rJ1AXC0-Dl/exec";


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

/*************** PH BUFFER FOR GSHEET *************/
volatile bool phUpdated = false;
float phToSend = 0.0;


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

BLYNK_WRITE(V11) {
  if (param.asInt() == 1) {   // กดปุ่ม Reset

    // reset pump
    pumpCommand[0] = 0;
    pumpCommand[1] = 0;

    // reset shiny
    for (int i = 0; i < 4; i++) {
      shinyCommand[i] = 0;
    }

    Serial.println("[BLYNK] RESET ALL");

    // ส่งไป MEGA ทันที
    sendToMega("PUMP",  pumpCommand,  2);
    sendToMega("SHINY", shinyCommand, 4);

    // เด้งปุ่มกลับภายใน 1 วิ
    timer.setTimeout(1000L, []() {
      Blynk.virtualWrite(V11, 0);
    });
  }
}

// void sendPHtoGoogleSheet(float ph, int pump1, int pump2) {
//   if (WiFi.status() != WL_CONNECTED) {
//     Serial.println("[GSHEET] WiFi not connected");
//     return;
//   }

//   HTTPClient http;

//   String url = GAS_URL +
//                "?ph=" + String(ph, 2) +
//                "&pump1=" + String(pump1) +
//                "&pump2=" + String(pump2)+
//                "&shiny1=" + String(shiny1) +

//   Serial.println("[GSHEET] " + url);

//   http.begin(url);
//   int httpCode = http.GET();

//   if (httpCode > 0) {
//     Serial.println("[GSHEET] OK");
//   } else {
//     Serial.println("[GSHEET] FAIL");
//   }

//   http.end();
// }

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

        // ✅ แจ้งว่ามีค่าใหม่
        phToSend  = pH_Value;
        phUpdated = true;
      }
      buffer = "";
    }
    else {
      buffer += c;
      if (buffer.length() > 30) buffer = "";
    }
  }
}

void sendPHtoGoogleSheetNonBlocking() {
  if (!phUpdated) return;
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;

  String url = GAS_URL + "?ph=" + String(phToSend, 2) +"&pump1=" + String(pumpCommand[0]) +"&pump2=" + String(pumpCommand[1]) + "&shiny1=" + String(shinyCommand[0])+ "&shiny2=" + String(shinyCommand[1])+ "&shiny3=" + String(shinyCommand[2])+ "&shiny4=" + String(shinyCommand[3]);

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.println("[GSHEET] OK");
    phUpdated = false;   // ✅ clear เมื่อส่งสำเร็จ
  } else {
    Serial.println("[GSHEET] FAIL (retry later)");
  }

  http.end();
}


/*************** KEEP ALIVE ************/
void sendAlways() {
  sendToMega("PUMP",  pumpCommand,  2);
  sendToMega("SHINY", shinyCommand, 4);
}

void syncBlynkState() {
  // pump
  Blynk.virtualWrite(V0, pumpCommand[0]);
  Blynk.virtualWrite(V1, pumpCommand[1]);

  // shiny
  Blynk.virtualWrite(V2, shinyCommand[0]);
  Blynk.virtualWrite(V3, shinyCommand[1]);
  Blynk.virtualWrite(V4, shinyCommand[2]);
  Blynk.virtualWrite(V5, shinyCommand[3]);
}

/*************** SETUP *****************/
void setup() {
  Serial.begin(115200);
  SerialESP.begin(115200, SERIAL_8N1, RXD2, TXD2);

  Blynk.begin(auth, ssid, pass);

  timer.setInterval(500L,  checkAndSend);   // ส่งเมื่อเปลี่ยน
  timer.setInterval(5000L, sendPHtoGoogleSheetNonBlocking); // ส่งทุก 10 วิ
  timer.setInterval(1000L,  syncBlynkState);
  timer.setInterval(2000L,  sendAlways);     // keep alive
  timer.setInterval(2000L,  readPHfromMega); // อ่าน pH

  Serial.println("ESP32 Started");
}

/*************** LOOP ******************/
void loop() {
  Blynk.run();
  timer.run();
}
