/**
 * GlassDial - 触れる破壊、手の中の再生
 * 完全実装版
 */
#include <M5Dial.h>
#include <vector>

// ========== 状態定義 ==========
enum GlassState {
  NORMAL,      // 通常状態 (0.0~0.05)
  TINY_CRACK,  // 極小ひび (0.05~0.15)
  SMALL_CRACK, // 小ひび (0.15~0.30)
  CRACK,       // ひび割れ (0.30~0.50)
  BIG_CRACK,   // 大ひび (0.50~0.65)
  SHATTER,     // 粉砕開始 (0.65~0.75)
  HEAVY_SHATTER, // 激粉砕 (0.75~0.85)
  SILENCE,     // 沈黙 (0.85~1.0)
  REBUILD,     // 再構築 (1.0→0.5)
  RECOVERY     // 回復 (0.5→0.0)
};

// ========== パーティクル構造体 ==========
struct Particle {
  float x, y;
  float vx, vy;
  float alpha;
  uint16_t color;
  
  Particle(float px, float py, float angle, float speed) {
    x = px;
    y = py;
    vx = cos(angle) * speed;
    vy = sin(angle) * speed;
    alpha = 255.0f;
    color = TFT_WHITE;
  }
  
  void update() {
    x += vx;
    y += vy;
    vy += 0.3f; // 重力
    vx *= 0.98f; // 減衰
    vy *= 0.98f;
    alpha *= 0.95f;
  }
  
  bool isAlive() { return alpha > 10.0f; }
};

// ========== ひび割れ構造体 ==========
struct Crack {
  float x1, y1, x2, y2;
  int generation;
  
  Crack(float px1, float py1, float px2, float py2, int gen) {
    x1 = px1; y1 = py1;
    x2 = px2; y2 = py2;
    generation = gen;
  }
};

// ========== グローバル変数 ==========
GlassState currentState = NORMAL;
float destructionLevel = 0.0f;
std::vector<Crack> cracks;
std::vector<Particle> particles;
unsigned long lastActivityTime = 0;
unsigned long lastStateChangeTime = 0;
bool wasInDestructiveState = false;

// ========== 定数定義 ==========
const float DESTRUCTION_INCREMENT = 0.015f; // 5倍に増加!もっと割れやすく
const float RECOVERY_SPEED = 0.001f;
const unsigned long IDLE_TIMEOUT = 3000; // 3秒に短縮
const int MAX_CRACKS = 300; // 300本に増加!もっと細かく
const int MAX_PARTICLES = 200;
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 240;

// ========== 音響定義 ==========
const int SOUND_CRACK = 1500;
const int SOUND_SHATTER = 2000;
const int SOUND_SILENCE = 500;
const int SOUND_REBUILD = 800;
const int SOUND_RECOVERY = 1200;

// ========== 関数プロトタイプ ==========
void updateState();
void renderGlass();
void addCrack(float x, float y, float angle, int generation);
void createShatterParticles(int count);
void playStateSound(GlassState state);
uint16_t getStateColor();

// ========== セットアップ ==========
void setup() {
  auto cfg = M5.config();
  M5Dial.begin(cfg, true, false);
  
  M5Dial.Display.fillScreen(TFT_BLACK);
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.setTextSize(1);
  M5Dial.Display.setTextColor(TFT_WHITE);
  
  const char* title = "GlassDial";
  const char* subtitle = "Rotate to break";
  M5Dial.Display.drawString(title, 120, 100);
  M5Dial.Display.drawString(subtitle, 120, 140);
  
  delay(2000);
  lastActivityTime = millis();
  lastStateChangeTime = millis();
}

// ========== メインループ ==========
void loop() {
  M5Dial.update();
  
  // エンコーダ読み取り
  static long lastEncoder = 0;
  long encoderValue = M5Dial.Encoder.read();
  long delta = abs(encoderValue - lastEncoder);
  
  if (delta > 0) {
    lastActivityTime = millis();
    
    // 破壊レベル増加
    if (currentState != REBUILD && currentState != RECOVERY) {
      destructionLevel += delta * DESTRUCTION_INCREMENT;
      if (destructionLevel > 1.0f) destructionLevel = 1.0f;
    }
    
    lastEncoder = encoderValue;
  }
  
  // アイドルタイムアウト処理
  unsigned long idleTime = millis() - lastActivityTime;
  if (idleTime > IDLE_TIMEOUT && destructionLevel > 0.0f) {
    if (currentState != REBUILD && currentState != RECOVERY) {
      wasInDestructiveState = true;
    }
  }
  
  // 自動回復
  if (wasInDestructiveState && idleTime > IDLE_TIMEOUT) {
    if (currentState == SILENCE || currentState == HEAVY_SHATTER || currentState == SHATTER || 
        currentState == BIG_CRACK || currentState == CRACK || currentState == SMALL_CRACK) {
      currentState = REBUILD;
      playStateSound(REBUILD);
      lastStateChangeTime = millis();
      wasInDestructiveState = false;
    }
  }
  
  if (currentState == REBUILD) {
    destructionLevel -= RECOVERY_SPEED;
    if (destructionLevel <= 0.5f) {
      currentState = RECOVERY;
      playStateSound(RECOVERY);
      lastStateChangeTime = millis();
    }
  }
  
  if (currentState == RECOVERY) {
    destructionLevel -= RECOVERY_SPEED;
    if (destructionLevel <= 0.0f) {
      destructionLevel = 0.0f;
      currentState = NORMAL;
      cracks.clear();
      particles.clear();
    }
  }
  
  // 状態更新
  updateState();
  
  // 描画
  renderGlass();
  
  delay(16); // 約60FPS
}

// ========== 状態更新 ==========
void updateState() {
  GlassState newState = currentState;
  
  // より細かい状態遷移
  if (destructionLevel < 0.05f) {
    newState = NORMAL;
  } else if (destructionLevel < 0.15f) {
    newState = TINY_CRACK;
  } else if (destructionLevel < 0.30f) {
    newState = SMALL_CRACK;
  } else if (destructionLevel < 0.50f) {
    newState = CRACK;
  } else if (destructionLevel < 0.65f) {
    newState = BIG_CRACK;
  } else if (destructionLevel < 0.75f) {
    newState = SHATTER;
  } else if (destructionLevel < 0.85f) {
    newState = HEAVY_SHATTER;
  } else if (destructionLevel >= 0.85f) {
    newState = SILENCE;
  }
  
  // 状態遷移処理
  if (newState != currentState && currentState != REBUILD && currentState != RECOVERY) {
    GlassState oldState = currentState;
    currentState = newState;
    
    // 全画面にランダムな位置からひび割れを追加!
    if (currentState >= TINY_CRACK && currentState <= BIG_CRACK) {
      // ランダムな位置から複数のひび割れを生成
      int crackCount = (int)currentState + 1; // 状態が進むほど多く
      for (int i = 0; i < crackCount; i++) {
        float startX = random(0, SCREEN_WIDTH);
        float startY = random(0, SCREEN_HEIGHT);
        float angle = random(0, 628) / 100.0f;
        addCrack(startX, startY, angle, 0);
      }
      playStateSound(CRACK);
    }
    else if (currentState == SHATTER) {
      // 全画面からパーティクル生成
      createShatterParticles(80);
      playStateSound(SHATTER);
    }
    else if (currentState == HEAVY_SHATTER) {
      // さらに大量のパーティクル
      createShatterParticles(100);
      playStateSound(SHATTER);
    }
    else if (currentState == SILENCE) {
      playStateSound(SILENCE);
    }
    
    lastStateChangeTime = millis();
  }
  
  // ひび割れ状態では常にランダムにピシッピシッと追加
  if (currentState >= TINY_CRACK && currentState <= BIG_CRACK && cracks.size() < MAX_CRACKS) {
    // 回転するたびにランダムな位置にひび追加
    if (random(0, 100) > 60) { // 40%の確率でピシッ
      float randX = random(0, SCREEN_WIDTH);
      float randY = random(0, SCREEN_HEIGHT);
      float randAngle = random(0, 628) / 100.0f;
      addCrack(randX, randY, randAngle, 0);
    }
  }
  
  // 既存のひびから分岐
  if (currentState >= CRACK && currentState <= BIG_CRACK && cracks.size() < MAX_CRACKS) {
    if (cracks.size() > 0 && random(0, 100) > 70) {
      int idx = random(0, cracks.size());
      Crack& c = cracks[idx];
      if (c.generation < 4) { // 4世代まで
        float midX = (c.x1 + c.x2) / 2.0f;
        float midY = (c.y1 + c.y2) / 2.0f;
        float angle = atan2(c.y2 - c.y1, c.x2 - c.x1) + random(-157, 157) / 100.0f;
        addCrack(midX, midY, angle, c.generation + 1);
      }
    }
  }
}

// ========== ひび割れ追加 ==========
void addCrack(float x, float y, float angle, int generation) {
  if (cracks.size() >= MAX_CRACKS) return;
  
  // ランダムな長さでより自然に
  float length = random(20, 60) / (generation + 1.0f);
  float x2 = x + cos(angle) * length;
  float y2 = y + sin(angle) * length;
  
  cracks.push_back(Crack(x, y, x2, y2, generation));
  
  // より複雑な分岐パターン
  if (generation < 4 && random(0, 100) > 40) {
    float branchAngle1 = angle + random(20, 120) / 100.0f;
    float branchAngle2 = angle - random(20, 120) / 100.0f;
    addCrack(x2, y2, branchAngle1, generation + 1);
    if (random(0, 100) > 50) { // 2本目は50%の確率
      addCrack(x2, y2, branchAngle2, generation + 1);
    }
  }
}

// ========== パーティクル生成 ==========
void createShatterParticles(int count) {
  for (int i = 0; i < count && particles.size() < MAX_PARTICLES; i++) {
    float angle = random(0, 628) / 100.0f;
    float speed = random(10, 50) / 10.0f;
    // 全画面からランダムに生成
    float x = random(0, SCREEN_WIDTH);
    float y = random(0, SCREEN_HEIGHT);
    particles.push_back(Particle(x, y, angle, speed));
  }
}

// ========== 音再生 ==========
void playStateSound(GlassState state) {
  int freq = 0;
  int duration = 100;
  
  switch (state) {
    case CRACK: freq = SOUND_CRACK; break;
    case SHATTER: freq = SOUND_SHATTER; duration = 200; break;
    case SILENCE: freq = SOUND_SILENCE; duration = 300; break;
    case REBUILD: freq = SOUND_REBUILD; duration = 150; break;
    case RECOVERY: freq = SOUND_RECOVERY; duration = 150; break;
    default: return;
  }
  
  M5Dial.Speaker.tone(freq, duration);
}

// ========== 描画 ==========
void renderGlass() {
  M5Dial.Display.fillScreen(TFT_BLACK);
  
  // 全画面に状態による背景効果
  uint16_t bgColor = getStateColor();
  if (bgColor != TFT_BLACK) {
    int alpha = (int)(destructionLevel * 100);
    // 全画面を薄く塗る
    M5Dial.Display.fillScreen(bgColor);
    // 暗めに戻す
    for (int y = 0; y < SCREEN_HEIGHT; y += 4) {
      for (int x = 0; x < SCREEN_WIDTH; x += 4) {
        if (random(0, 100) > alpha) {
          M5Dial.Display.fillRect(x, y, 4, 4, TFT_BLACK);
        }
      }
    }
  }
  
  // ひび割れ描画 - 全画面
  for (auto& crack : cracks) {
    uint16_t color = TFT_WHITE;
    if (currentState == SILENCE) color = TFT_DARKGREY;
    else if (currentState == HEAVY_SHATTER) color = TFT_RED;
    M5Dial.Display.drawLine((int)crack.x1, (int)crack.y1, (int)crack.x2, (int)crack.y2, color);
  }
  
  // パーティクル更新と描画 - 全画面
  for (int i = particles.size() - 1; i >= 0; i--) {
    particles[i].update();
    if (!particles[i].isAlive()) {
      particles.erase(particles.begin() + i);
    } else {
      int x = (int)particles[i].x;
      int y = (int)particles[i].y;
      if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        M5Dial.Display.fillCircle(x, y, 2, TFT_WHITE); // 少し大きく
      }
    }
  }
  
  // 破壊レベル表示
  M5Dial.Display.setTextDatum(top_center);
  M5Dial.Display.setTextSize(1);
  M5Dial.Display.setTextColor(TFT_WHITE);
  M5Dial.Display.drawString(String(destructionLevel, 2), 120, 10);
  
  // 状態名表示
  const char* stateName = nullptr;
  const char* s_normal = "NORMAL";
  const char* s_tiny = "TINY_CRACK";
  const char* s_small = "SMALL_CRACK";
  const char* s_crack = "CRACK";
  const char* s_big = "BIG_CRACK";
  const char* s_shatter = "SHATTER";
  const char* s_heavy = "HEAVY_SHATTER";
  const char* s_silence = "SILENCE";
  const char* s_rebuild = "REBUILD";
  const char* s_recovery = "RECOVERY";
  
  switch (currentState) {
    case NORMAL: stateName = s_normal; break;
    case TINY_CRACK: stateName = s_tiny; break;
    case SMALL_CRACK: stateName = s_small; break;
    case CRACK: stateName = s_crack; break;
    case BIG_CRACK: stateName = s_big; break;
    case SHATTER: stateName = s_shatter; break;
    case HEAVY_SHATTER: stateName = s_heavy; break;
    case SILENCE: stateName = s_silence; break;
    case REBUILD: stateName = s_rebuild; break;
    case RECOVERY: stateName = s_recovery; break;
  }
  
  if (stateName) {
    M5Dial.Display.setTextDatum(bottom_center);
    M5Dial.Display.drawString(stateName, 120, 230);
  }
}

// ========== 状態色取得 ==========
uint16_t getStateColor() {
  switch (currentState) {
    case TINY_CRACK: return M5Dial.Display.color565(20, 20, 40);
    case SMALL_CRACK: return M5Dial.Display.color565(30, 30, 50);
    case CRACK: return M5Dial.Display.color565(40, 40, 60);
    case BIG_CRACK: return M5Dial.Display.color565(60, 30, 30);
    case SHATTER: return M5Dial.Display.color565(80, 20, 20);
    case HEAVY_SHATTER: return M5Dial.Display.color565(120, 10, 10);
    case SILENCE: return M5Dial.Display.color565(20, 20, 20);
    case REBUILD: return M5Dial.Display.color565(20, 60, 40);
    case RECOVERY: return M5Dial.Display.color565(40, 80, 60);
    default: return TFT_BLACK;
  }
}
