/**
 * M5Dial Encoder Test
 */
#include <M5Unified.h>

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n=== M5DIAL ENCODER TEST ===");
  
  M5.begin();
  Serial.println("M5.begin() OK");
  
  M5.Display.fillScreen(TFT_RED);
  delay(500);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(10, 60);
  M5.Display.println("ENCODER TEST");
  M5.Display.setCursor(10, 90);
  M5.Display.println("Rotate dial!");
  
  Serial.println("Display OK");
  Serial.println("Rotate the dial now!\n");
}

void loop() {
  M5.update();
  
  static int c = 0;
  c++;
  
  // I2Cから4バイト読み取り（エンコーダ）
  uint8_t data[4] = {0};
  bool success = M5.In_I2C.readRegister(0x40, 0x10, data, 4, 400000);
  
  // 全てのバイト組み合わせを計算
  static int16_t last_val = 0;
  int16_t val_bytes23 = (int16_t)((data[2] << 8) | data[3]);
  int16_t val_bytes01 = (int16_t)((data[0] << 8) | data[1]);
  int16_t val_bytes32 = (int16_t)((data[3] << 8) | data[2]);
  int16_t val_bytes10 = (int16_t)((data[1] << 8) | data[0]);
  int16_t delta = val_bytes23 - last_val;
  
  // 50回ごとに必ず画面更新
  if (c % 50 == 0 || delta != 0) {
    M5.Display.fillRect(0, 120, 240, 120, TFT_BLACK);
    M5.Display.setCursor(5, 120);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_WHITE);
    
    M5.Display.printf("Loop:%d I2C:%s\n", c, success?"OK":"FAIL");
    M5.Display.printf("Raw: %02X %02X %02X %02X\n", data[0], data[1], data[2], data[3]);
    M5.Display.printf("Val[2:3]: %d\n", val_bytes23);
    M5.Display.printf("Val[0:1]: %d\n", val_bytes01);
    M5.Display.printf("Val[3:2]: %d\n", val_bytes32);
    M5.Display.printf("Val[1:0]: %d\n", val_bytes10);
    
    if (delta != 0) {
      M5.Display.setTextColor(TFT_YELLOW);
      M5.Display.printf(">>> DELTA=%d <<<", delta);
    }
    
    last_val = val_bytes23;
  }
  
  delay(20);
}
