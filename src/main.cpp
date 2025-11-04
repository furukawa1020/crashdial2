/**
 * M5Dial Test - 最小限の動作確認
 */

#include <M5Unified.h>

void setup() {
  // シリアル初期化
  Serial.begin(115200);
  delay(2000);
  Serial.println("=== M5Dial MINIMAL Test ===");
  
  // M5Dial初期化 - 最小限の設定
  Serial.println("Calling M5.begin()...");
  M5.begin();  // デフォルト設定で起動
  Serial.println("M5.begin() completed");
  
  // ディスプレイ設定
  Serial.println("Configuring display...");
  M5.Display.setRotation(0);
  M5.Display.setBrightness(255);  // 最大輝度
  M5.Display.fillScreen(TFT_RED);  // 赤で塗りつぶし（動作確認用）
  Serial.println("Display configured - RED screen");
  
  delay(1000);
  
  // 白い円を描画
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.fillCircle(120, 120, 50, TFT_WHITE);
  Serial.println("White circle drawn");
  
  // スピーカーテスト
  Serial.println("Testing speaker...");
  M5.Speaker.begin();
  M5.Speaker.setVolume(128);
  M5.Speaker.tone(1000, 200);
  Serial.println("Speaker test done");
  
  delay(500);
  
  // テキスト表示
  M5.Display.setTextColor(TFT_GREEN);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(60, 100);
  M5.Display.println("M5Dial");
  M5.Display.setCursor(80, 130);
  M5.Display.println("OK!");
  
  Serial.println("=== Setup Complete ===");
}

void loop() {
  M5.update();
  
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  // 1秒ごとにLED点滅（動作確認）
  if (millis() - lastBlink > 1000) {
    lastBlink = millis();
    ledState = !ledState;
    
    if (ledState) {
      M5.Display.fillCircle(120, 120, 10, TFT_GREEN);
      Serial.println("Loop running... [ON]");
    } else {
      M5.Display.fillCircle(120, 120, 10, TFT_BLACK);
      Serial.println("Loop running... [OFF]");
    }
  }
  
  // ボタンチェック
  if (M5.BtnA.wasPressed()) {
    Serial.println("Button pressed!");
    M5.Speaker.tone(1500, 100);
  }
  
  delay(10);
}
