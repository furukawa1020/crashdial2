/**
 * ESP32 Raw Test - M5Unified抜きで動作確認
 */

#include <Arduino.h>

void setup() {
  // シリアル初期化
  Serial.begin(115200);
  delay(2000);
  Serial.println("=== ESP32 RAW Test ===");
  Serial.println("Setup starting...");
  
  delay(1000);
  Serial.println("Setup complete!");
  delay(1000);
  Serial.println("Setup complete!");
}

void loop() {
  static unsigned long lastPrint = 0;
  static int count = 0;
  
  // 1秒ごとにカウント表示
  if (millis() - lastPrint > 1000) {
    lastPrint = millis();
    count++;
    Serial.print("Loop running... count: ");
    Serial.println(count);
  }
  
  delay(10);
}
