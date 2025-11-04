/**
 * M5Dial Encoder Test - Minimal Debug Version
 * 繧ｷ繝ｪ繧｢繝ｫ蜃ｺ蜉帙→繧ｨ繝ｳ繧ｳ繝ｼ繝隱ｭ縺ｿ蜿悶ｊ縺ｮ繝・せ繝・
 */

#include <M5Unified.h>

int32_t encoderValue = 0;

void setup() {
  // 繧ｷ繝ｪ繧｢繝ｫ蛻晄悄蛹厄ｼ域怙蜆ｪ蜈茨ｼ・
  Serial.begin(115200);
  delay(1000);  // 繧ｷ繝ｪ繧｢繝ｫ繝昴・繝亥ｮ牙ｮ壼喧蠕・■
  
  Serial.println("\n\n=== M5Dial Encoder Test ===");
  Serial.println("Starting M5.begin()...");
  
  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.external_imu = false;
  M5.begin(cfg);
  
  Serial.println("M5.begin() OK!");
  Serial.printf("Board: %d\n", (int)M5.getBoard());
  
  // I2C繧ｹ繧ｭ繝｣繝ｳ
  Serial.println("\n--- I2C Scan ---");
  uint8_t testbuf[1];
  for (uint8_t addr = 1; addr < 127; ++addr) {
    if (M5.In_I2C.readRegister(addr, 0x00, testbuf, 1, 100000)) {
      Serial.printf("Found device at 0x%02X\n", addr);
    }
    delay(2);
  }
  Serial.println("--- Scan done ---\n");
  
  // 繝・ぅ繧ｹ繝励Ξ繧､蛻晄悄蛹・
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(10, 10);
  M5.Display.println("Encoder Test");
  M5.Display.println("Rotate dial!");
  
  Serial.println("Setup complete. Rotate the dial now!\n");
}

void loop() {
  M5.update();
  
  // I2C縺九ｉ4繝舌う繝郁ｪｭ縺ｿ蜿悶ｊ
  uint8_t data[4] = {0};
  static int loopCount = 0;
  loopCount++;
  
  bool success = M5.In_I2C.readRegister(0x40, 0x10, data, 4, 400000);
  
  if (success) {
    // 蜈ｨ縺ｦ縺ｮ繝舌う繝育ｵ・∩蜷医ｏ縺帙ｒ陦ｨ遉ｺ
    int16_t val01 = (int16_t)((data[0] << 8) | data[1]);
    int16_t val10 = (int16_t)((data[1] << 8) | data[0]);
    int16_t val23 = (int16_t)((data[2] << 8) | data[3]);
    int16_t val32 = (int16_t)((data[3] << 8) | data[2]);
    
    // 蛟､縺ｮ螟牙喧繧呈､懷・
    static int16_t last_val23 = 0;
    int16_t delta = val23 - last_val23;
    
    if (loopCount % 30 == 0 || delta != 0) {
      Serial.printf("[%d] Raw: %02X %02X %02X %02X | ", loopCount, data[0], data[1], data[2], data[3]);
      Serial.printf("[0:1]=%d [1:0]=%d [2:3]=%d [3:2]=%d", val01, val10, val23, val32);
      
      if (delta != 0) {
        Serial.printf(" <<< DELTA=%d", delta);
        
        // 逕ｻ髱｢譖ｴ譁ｰ
        M5.Display.fillRect(10, 60, 220, 100, TFT_BLACK);
        M5.Display.setCursor(10, 60);
        M5.Display.printf("Enc: %d\n", val23);
        M5.Display.printf("Delta: %d\n", delta);
        M5.Display.printf("Loop: %d", loopCount);
      }
      Serial.println();
      
      last_val23 = val23;
    }
  } else {
    if (loopCount % 100 == 0) {
      Serial.printf("[%d] I2C read FAILED\n", loopCount);
    }
  }
  
  delay(20);  // 50Hz譖ｴ譁ｰ

}
