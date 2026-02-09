/*************** GLOBAL ****************/
float pH_Value;

int pumpState[2]  = {0, 0};
int shinyState[4] = {0, 0, 0, 0};

unsigned long lastPH = 0;

/*************** RELAY ****************/
const int shinyRelayPin[4] = {4, 5, 6, 7};

/*************** READ ADC **************/
int readPH_ADC() {
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(A0);
    delay(10);
  }
  return sum / 10;
}

/*************** PH SENSOR *************/
void PH_sen() {
  int adc = readPH_ADC();
  float voltage = adc * (5.0 / 1023.0);

  pH_Value = 3.5 * voltage;   // ยังไม่ calibrate
  if (pH_Value < 0 || pH_Value > 14) return;

  Serial.print("[MEGA] pH = ");
  Serial.println(pH_Value, 2);

  Serial1.print("<PH:");
  Serial1.print(pH_Value, 2);
  Serial1.println(">");
}

/*************** READ FROM ESP32 *******/
void readFromESP32() {
  static String buffer = "";

  while (Serial1.available()) {
    char c = Serial1.read();

    if (c == '<') {
      buffer = "";
    }
    else if (c == '>') {

      // ===== PUMP =====
      if (buffer.startsWith("PUMP:")) {
        sscanf(buffer.c_str(), "PUMP:%d,%d",
               &pumpState[0], &pumpState[1]);

        Serial.print("[MEGA] PUMP = ");
        Serial.print(pumpState[0]);
        Serial.print(", ");
        Serial.println(pumpState[1]);

        // 👉 ถ้าจะคุมรีเลย์ปั๊ม ใส่เพิ่มตรงนี้
      }

      // ===== SHINY =====
      else if (buffer.startsWith("SHINY:")) {
        sscanf(buffer.c_str(), "SHINY:%d,%d,%d,%d",
               &shinyState[0], &shinyState[1],
               &shinyState[2], &shinyState[3]);

        Serial.print("[MEGA] SHINY = ");
        for (int i = 0; i < 4; i++) {
          Serial.print(shinyState[i]);
          if (i < 3) Serial.print(",");
        }
        Serial.println();

        // ===== CONTROL RELAY 4-5-6-7 =====
        for (int i = 0; i < 4; i++) {
          if (shinyState[i] == 1) {
            digitalWrite(shinyRelayPin[i], LOW);   // ON (Active LOW)
          } else {
            digitalWrite(shinyRelayPin[i], HIGH);  // OFF
          }
        }
      }

      buffer = "";
    }
    else {
      buffer += c;
      if (buffer.length() > 50) buffer = "";
    }
  }
}

/*************** SETUP *****************/
void setup() {
  Serial.begin(115200);     // USB Debug
  Serial1.begin(115200);    // UART ไป ESP32

  // Relay setup
  for (int i = 0; i < 4; i++) {
    pinMode(shinyRelayPin[i], OUTPUT);
    digitalWrite(shinyRelayPin[i], HIGH); // ปิดรีเลย์เริ่มต้น
  }

  Serial.println("MEGA Started");
}

/*************** LOOP ******************/
void loop() {

  // รับคำสั่งจาก ESP32 ตลอด
  readFromESP32();

  // ส่งค่า pH ทุก 1 วินาที
  if (millis() - lastPH >= 1000) {
    lastPH = millis();
    PH_sen();
  }
}
