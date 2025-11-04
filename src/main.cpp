/**
 * GlassDial - 触れる破壊、手の中の再生
 * 
 * "壊したい"という衝動を、「創造のプロセス」に変換するデバイス
 * 破壊とは、形を失うことではなく、「意味」を一度リセットする行為である
 */

#include <M5Unified.h>
#include <vector>
#include <cmath>

// ========================================
// 状態定義（State Model）
// ========================================
enum State {
  NORMAL,    // 初期状態 - 完全透明な静止画面
  CRACK,     // ひび割れ生成
  SHATTER,   // 粉砕
  SILENCE,   // 無音の余韻
  REBUILD,   // 修復
  RECOVERY   // 完全修復
};

// ========================================
// ひび構造体
// ========================================
struct Crack {
  float startX, startY;  // 開始点
  float endX, endY;      // 終了点
  float angle;           // 角度
  float length;          // 長さ
  int generation;        // 世代（フラクタル深度）
  float alpha;           // 透明度
  bool active;           // アクティブ状態
};

// ========================================
// 粉末粒子構造体
// ========================================
struct Particle {
  float x, y;           // 位置
  float vx, vy;         // 速度
  float size;           // サイズ
  float alpha;          // 透明度
  bool active;          // アクティブ状態
};

// ========================================
// グローバル変数
// ========================================
State currentState = NORMAL;
State previousState = NORMAL;

// エンコーダー関連
int32_t encoderValue = 0;
int32_t lastEncoderValue = 0;
int32_t encoderDelta = 0;
float rotationSpeed = 0.0f;

// 破壊進行度
float destructionLevel = 0.0f;  // 0.0 ~ 1.0
const float CRACK_THRESHOLD = 0.15f;   // ひび割れ開始閾値
const float SHATTER_THRESHOLD = 0.65f; // 粉砕開始閾値

// ひび割れデータ
std::vector<Crack> cracks;
const int MAX_CRACKS = 80;

// 粒子データ
std::vector<Particle> particles;
const int MAX_PARTICLES = 150;

// タイマー
unsigned long lastUpdateTime = 0;
unsigned long stateStartTime = 0;
unsigned long lastInteractionTime = 0;
const unsigned long AUTO_RECOVER_TIME = 10000; // 10秒で自動修復

// 画面サイズ
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 240;
const int CENTER_X = 120;
const int CENTER_Y = 120;

// 触覚フィードバック設定
const int HAPTIC_PIN = 25; // M5Dialの振動モーター

// ========================================
// 音響周波数定義
// ========================================
const int FREQ_CRACK = 1500;
const int FREQ_SHATTER = 2000;
const int FREQ_REBUILD = 800;
const int FREQ_RECOVERY = 1200;

// ========================================
// 関数プロトタイプ
// ========================================
void updateEncoder();
void updateState();
void updateDestruction();
void renderState();
void renderNormal();
void renderCrack();
void renderShatter();
void renderSilence();
void renderRebuild();
void renderRecovery();
void generateCrack(float centerX, float centerY, float angle, int generation);
void generateParticles();
void updateParticles();
void playSound(int frequency, int duration);
void hapticFeedback(int duration, int strength);
void handleButton();
void autoRecover();

// ========================================
// Setup
// ========================================
void setup() {
  // M5Dial初期化 - 安全な設定で起動
  auto cfg = M5.config();
  
  // I2C設定を明示的に指定
  cfg.internal_imu = false;  // IMU無効化
  cfg.external_imu = false;  // 外部IMU無効化
  
  M5.begin(cfg);
  
  // シリアル初期化（デバッグ用）
  Serial.begin(115200);
  delay(500);
  Serial.println("GlassDial - Starting...");
  
  // ディスプレイ設定
  M5.Display.setRotation(0);
  M5.Display.setBrightness(200);
  M5.Display.fillScreen(TFT_BLACK);
  Serial.println("Display OK");
  
  // スピーカー初期化
  M5.Speaker.begin();
  M5.Speaker.setVolume(128);
  Serial.println("Speaker OK");
  
  // エンコーダー初期化（M5Dialのロータリーエンコーダー用変数）
  encoderValue = 0;
  lastEncoderValue = 0;
  Serial.println("Encoder OK");
  
  // 触覚モーター初期化（M5Dialは内蔵）
  
  // 初期化完了音
  playSound(FREQ_RECOVERY, 100);
  delay(150);
  
  lastUpdateTime = millis();
  lastInteractionTime = millis();
  stateStartTime = millis();
  
  Serial.println("GlassDial - Initialized!");
}

// ========================================
// Main Loop
// ========================================
void loop() {
  M5.update();
  
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;
  lastUpdateTime = currentTime;
  
  // エンコーダー更新
  updateEncoder();
  
  // ボタン処理
  handleButton();
  
  // 状態更新
  updateState();
  
  // 描画
  renderState();
  
  // 自動修復チェック
  autoRecover();
  
  delay(16); // 約60FPS
}

// ========================================
// エンコーダー更新
// ========================================
void updateEncoder() {
  // M5Dialのロータリーエンコーダー読み取り
  // I2C経由でエンコーダーチップ(0x40)から読み取り
  
  // エンコーダー値読み取り（安全な実装）
  uint8_t reg_data[4] = {0};
  
  // I2C通信でエンコーダーの値を取得
  // M5Dialのエンコーダーは0x40アドレス、レジスタ0x10から4バイト読み取り
  bool success = false;
  
  // I2C通信を安全に実行
  if (M5.In_I2C.readRegister(0x40, 0x10, reg_data, 4, 400000)) {
    // ビッグエンディアンで2バイト目と3バイト目を結合
    int16_t newEncVal = (int16_t)((reg_data[2] << 8) | reg_data[3]);
    encoderDelta = newEncVal - encoderValue;
    encoderValue = newEncVal;
    success = true;
  }
  
  // I2C通信が失敗した場合はエンコーダー変化なしとする
  if (!success) {
    encoderDelta = 0;
  }
  
  if (encoderDelta != 0) {
    lastInteractionTime = millis();
    
    // 回転速度計算（-1.0 ~ 1.0）
    rotationSpeed = constrain(encoderDelta / 10.0f, -1.0f, 1.0f);
    
    // 破壊進行度更新
    updateDestruction();
    
    lastEncoderValue = encoderValue;
  } else {
    rotationSpeed *= 0.9f; // 減衰
  }
}

// ========================================
// 破壊進行度更新
// ========================================
void updateDestruction() {
  // 正回転で破壊、逆回転で修復
  destructionLevel += encoderDelta * 0.003f;
  destructionLevel = constrain(destructionLevel, 0.0f, 1.0f);
}

// ========================================
// 状態更新ロジック
// ========================================
void updateState() {
  previousState = currentState;
  
  switch (currentState) {
    case NORMAL:
      if (destructionLevel > CRACK_THRESHOLD) {
        currentState = CRACK;
        stateStartTime = millis();
        playSound(FREQ_CRACK, 50);
        hapticFeedback(60, 30);
        Serial.println("State: NORMAL -> CRACK");
      }
      break;
      
    case CRACK:
      if (destructionLevel > SHATTER_THRESHOLD) {
        currentState = SHATTER;
        stateStartTime = millis();
        generateParticles();
        playSound(FREQ_SHATTER, 200);
        hapticFeedback(300, 10);
        Serial.println("State: CRACK -> SHATTER");
      } else if (destructionLevel < CRACK_THRESHOLD) {
        currentState = NORMAL;
        cracks.clear();
        stateStartTime = millis();
        Serial.println("State: CRACK -> NORMAL");
      }
      break;
      
    case SHATTER:
      // 粒子更新
      updateParticles();
      
      // 逆回転で修復開始
      if (encoderDelta < -2) {
        currentState = REBUILD;
        stateStartTime = millis();
        playSound(FREQ_REBUILD, 100);
        hapticFeedback(500, 15);
        Serial.println("State: SHATTER -> REBUILD");
      }
      
      // 静止で余韻
      if (millis() - lastInteractionTime > 1000 && abs(rotationSpeed) < 0.01f) {
        currentState = SILENCE;
        stateStartTime = millis();
        Serial.println("State: SHATTER -> SILENCE");
      }
      break;
      
    case SILENCE:
      // 逆回転で修復
      if (encoderDelta < 0) {
        currentState = REBUILD;
        stateStartTime = millis();
        playSound(FREQ_REBUILD, 100);
        Serial.println("State: SILENCE -> REBUILD");
      }
      break;
      
    case REBUILD:
      // 修復進行
      updateParticles();
      
      if (destructionLevel < 0.05f) {
        currentState = RECOVERY;
        stateStartTime = millis();
        playSound(FREQ_RECOVERY, 150);
        hapticFeedback(40, 20);
        Serial.println("State: REBUILD -> RECOVERY");
      }
      break;
      
    case RECOVERY:
      // 完全修復後、NORMALへ
      if (millis() - stateStartTime > 800) {
        currentState = NORMAL;
        particles.clear();
        cracks.clear();
        destructionLevel = 0.0f;
        stateStartTime = millis();
        Serial.println("State: RECOVERY -> NORMAL");
      }
      break;
  }
}

// ========================================
// 描画メイン
// ========================================
void renderState() {
  M5.Display.fillScreen(TFT_BLACK);
  
  switch (currentState) {
    case NORMAL:
      renderNormal();
      break;
    case CRACK:
      renderCrack();
      break;
    case SHATTER:
      renderShatter();
      break;
    case SILENCE:
      renderSilence();
      break;
    case REBUILD:
      renderRebuild();
      break;
    case RECOVERY:
      renderRecovery();
      break;
  }
  
  // デバッグ情報（オプション）
  // M5.Display.setCursor(5, 5);
  // M5.Display.printf("D:%.2f S:%d", destructionLevel, currentState);
}

// ========================================
// NORMAL状態の描画
// ========================================
void renderNormal() {
  // 完全透明な静止画面
  // 中央に薄く円を描画（ガラスの存在を示唆）
  M5.Display.drawCircle(CENTER_X, CENTER_Y, 80, TFT_DARKGREY);
  M5.Display.drawCircle(CENTER_X, CENTER_Y, 81, TFT_DARKGREY);
  
  // 呼吸するような光（自動修復後の余韻）
  if (millis() - stateStartTime < 2000) {
    float breathe = sin((millis() - stateStartTime) * 0.003f) * 0.5f + 0.5f;
    uint8_t brightness = (uint8_t)(breathe * 30);
    uint32_t color = M5.Display.color565(brightness, brightness, brightness + 20);
    M5.Display.fillCircle(CENTER_X, CENTER_Y, 5, color);
  }
}

// ========================================
// CRACK状態の描画（ひび割れ）
// ========================================
void renderCrack() {
  // ベースガラス
  M5.Display.drawCircle(CENTER_X, CENTER_Y, 80, TFT_DARKGREY);
  
  // ひび割れ生成（破壊進行度に応じて）
  int targetCracks = (int)((destructionLevel - CRACK_THRESHOLD) / 
                           (SHATTER_THRESHOLD - CRACK_THRESHOLD) * MAX_CRACKS);
  
  while (cracks.size() < targetCracks && cracks.size() < MAX_CRACKS) {
    float angle = random(0, 360) * DEG_TO_RAD;
    generateCrack(CENTER_X, CENTER_Y, angle, 0);
  }
  
  // ひび割れ描画
  for (auto& crack : cracks) {
    if (crack.active) {
      uint32_t color = M5.Display.color565(200, 200, 255);
      M5.Display.drawLine((int)crack.startX, (int)crack.startY,
                         (int)crack.endX, (int)crack.endY, color);
      
      // 分岐ひび（フラクタル）
      if (crack.generation < 2 && random(100) < 30) {
        float newAngle = crack.angle + random(-30, 30) * DEG_TO_RAD;
        generateCrack(crack.endX, crack.endY, newAngle, crack.generation + 1);
      }
    }
  }
}

// ========================================
// ひび割れ生成
// ========================================
void generateCrack(float centerX, float centerY, float angle, int generation) {
  if (cracks.size() >= MAX_CRACKS) return;
  
  Crack crack;
  crack.startX = centerX;
  crack.startY = centerY;
  crack.length = random(15, 40) / (generation + 1.0f);
  crack.angle = angle;
  crack.endX = centerX + cos(angle) * crack.length;
  crack.endY = centerY + sin(angle) * crack.length;
  crack.generation = generation;
  crack.alpha = 1.0f;
  crack.active = true;
  
  cracks.push_back(crack);
}

// ========================================
// SHATTER状態の描画（粉砕）
// ========================================
void renderShatter() {
  // 全てのひび割れを描画
  for (auto& crack : cracks) {
    uint32_t color = M5.Display.color565(180, 180, 220);
    M5.Display.drawLine((int)crack.startX, (int)crack.startY,
                       (int)crack.endX, (int)crack.endY, color);
  }
  
  // 粒子描画
  for (auto& p : particles) {
    if (p.active) {
      uint8_t alpha = (uint8_t)(p.alpha * 255);
      uint32_t color = M5.Display.color565(alpha, alpha, alpha);
      M5.Display.fillCircle((int)p.x, (int)p.y, (int)p.size, color);
    }
  }
  
  // フラッシュ効果（粉砕直後）
  if (millis() - stateStartTime < 200) {
    float flash = 1.0f - (millis() - stateStartTime) / 200.0f;
    uint8_t brightness = (uint8_t)(flash * 100);
    M5.Display.fillCircle(CENTER_X, CENTER_Y, 50, 
                         M5.Display.color565(brightness, brightness, brightness));
  }
}

// ========================================
// 粒子生成
// ========================================
void generateParticles() {
  particles.clear();
  
  for (int i = 0; i < MAX_PARTICLES; i++) {
    Particle p;
    
    // 中心からランダムな位置
    float angle = random(0, 360) * DEG_TO_RAD;
    float distance = random(10, 60);
    p.x = CENTER_X + cos(angle) * distance;
    p.y = CENTER_Y + sin(angle) * distance;
    
    // 外向きの速度
    p.vx = cos(angle) * random(1, 4);
    p.vy = sin(angle) * random(1, 4);
    
    p.size = random(1, 3);
    p.alpha = 1.0f;
    p.active = true;
    
    particles.push_back(p);
  }
}

// ========================================
// 粒子更新
// ========================================
void updateParticles() {
  for (auto& p : particles) {
    if (!p.active) continue;
    
    if (currentState == SHATTER || currentState == SILENCE) {
      // 拡散
      p.x += p.vx;
      p.y += p.vy;
      p.vx *= 0.98f; // 減衰
      p.vy *= 0.98f;
      p.alpha *= 0.995f;
      
      // 画面外チェック
      if (p.x < 0 || p.x > SCREEN_WIDTH || p.y < 0 || p.y > SCREEN_HEIGHT) {
        p.alpha *= 0.9f;
      }
      
      if (p.alpha < 0.1f) {
        p.active = false;
      }
      
    } else if (currentState == REBUILD) {
      // 中央に収束
      float dx = CENTER_X - p.x;
      float dy = CENTER_Y - p.y;
      float distance = sqrt(dx * dx + dy * dy);
      
      if (distance > 5) {
        p.vx = dx * 0.05f;
        p.vy = dy * 0.05f;
        p.x += p.vx;
        p.y += p.vy;
      } else {
        p.active = false;
      }
    }
  }
}

// ========================================
// SILENCE状態の描画（余韻）
// ========================================
void renderSilence() {
  // 残光の粒子のみ
  for (auto& p : particles) {
    if (p.active && p.alpha > 0.3f) {
      uint8_t brightness = (uint8_t)(p.alpha * 150);
      uint32_t color = M5.Display.color565(brightness, brightness, brightness + 50);
      M5.Display.fillCircle((int)p.x, (int)p.y, (int)p.size, color);
    }
  }
}

// ========================================
// REBUILD状態の描画（修復）
// ========================================
void renderRebuild() {
  // ひび割れが徐々に消える
  for (auto& crack : cracks) {
    crack.alpha *= 0.95f;
    if (crack.alpha > 0.1f) {
      uint8_t brightness = (uint8_t)(crack.alpha * 200);
      uint32_t color = M5.Display.color565(brightness, brightness, 255);
      M5.Display.drawLine((int)crack.startX, (int)crack.startY,
                         (int)crack.endX, (int)crack.endY, color);
    }
  }
  
  // 粒子が中央に集まる
  for (auto& p : particles) {
    if (p.active) {
      uint32_t color = M5.Display.color565(180, 200, 255);
      M5.Display.fillCircle((int)p.x, (int)p.y, (int)p.size, color);
      
      // トレイル効果
      M5.Display.drawLine((int)p.x, (int)p.y, CENTER_X, CENTER_Y,
                         M5.Display.color565(50, 50, 100));
    }
  }
  
  // 中央の光
  float intensity = 1.0f - destructionLevel;
  uint8_t brightness = (uint8_t)(intensity * 100);
  M5.Display.fillCircle(CENTER_X, CENTER_Y, 10, 
                       M5.Display.color565(brightness, brightness, brightness + 50));
}

// ========================================
// RECOVERY状態の描画（完全修復）
// ========================================
void renderRecovery() {
  // フェードイン効果で透明ガラスに戻る
  float progress = (millis() - stateStartTime) / 800.0f;
  uint8_t brightness = (uint8_t)((1.0f - progress) * 150);
  
  // 中央の光が広がる
  int radius = (int)(progress * 80);
  M5.Display.drawCircle(CENTER_X, CENTER_Y, radius, 
                       M5.Display.color565(brightness, brightness, brightness + 30));
  
  // 最終的な光の明滅
  if (progress > 0.7f) {
    float pulse = sin((millis() - stateStartTime) * 0.01f) * 0.5f + 0.5f;
    uint8_t pulseBright = (uint8_t)(pulse * 80);
    M5.Display.fillCircle(CENTER_X, CENTER_Y, 5,
                         M5.Display.color565(pulseBright, pulseBright, pulseBright + 50));
  }
}

// ========================================
// サウンド再生
// ========================================
void playSound(int frequency, int duration) {
  M5.Speaker.tone(frequency, duration);
}

// ========================================
// 触覚フィードバック
// ========================================
void hapticFeedback(int duration, int strength) {
  // M5Dialの内蔵振動モーターを制御
  // 注: M5Dialの具体的な振動API実装に依存
  // 擬似的な実装（実際のハードウェアに合わせて調整）
  
  // トーン音で代替する簡易実装
  M5.Speaker.tone(100, duration);
}

// ========================================
// ボタン処理
// ========================================
void handleButton() {
  // 短押し: 一段階戻る
  if (M5.BtnA.wasPressed()) {
    lastInteractionTime = millis();
    
    switch (currentState) {
      case CRACK:
        currentState = NORMAL;
        cracks.clear();
        destructionLevel = 0.0f;
        break;
      case SHATTER:
      case SILENCE:
        currentState = CRACK;
        particles.clear();
        destructionLevel = CRACK_THRESHOLD + 0.1f;
        break;
      case REBUILD:
      case RECOVERY:
        currentState = NORMAL;
        particles.clear();
        cracks.clear();
        destructionLevel = 0.0f;
        break;
    }
    
    playSound(800, 50);
    Serial.println("Button: Step back");
  }
  
  // 長押し: 完全リセット
  if (M5.BtnA.pressedFor(1000)) {
    currentState = NORMAL;
    particles.clear();
    cracks.clear();
    destructionLevel = 0.0f;
    lastInteractionTime = millis();
    
    playSound(FREQ_RECOVERY, 200);
    Serial.println("Button: Full reset");
    
    // 長押し後の再トリガー防止
    while (M5.BtnA.isPressed()) {
      M5.update();
      delay(10);
    }
  }
}

// ========================================
// 自動修復
// ========================================
void autoRecover() {
  // 10秒間操作がない場合、自動的にNORMALに戻る
  if (currentState != NORMAL && 
      millis() - lastInteractionTime > AUTO_RECOVER_TIME) {
    
    Serial.println("Auto-recovery triggered");
    
    // ゆっくりと修復
    if (destructionLevel > 0) {
      destructionLevel -= 0.01f;
      
      if (destructionLevel <= 0) {
        destructionLevel = 0;
        currentState = RECOVERY;
        stateStartTime = millis();
        particles.clear();
        cracks.clear();
        playSound(FREQ_RECOVERY, 150);
      }
    }
    
    lastInteractionTime = millis(); // 連続実行を防ぐ
  }
}
