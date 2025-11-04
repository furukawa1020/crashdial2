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
  SILENCE      // 沈黙 (0.85~1.0)
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
std::vector<Particle> particles; // 使わないが構造体は残す
long lastEncoderValue = 0;

// ========== 定数定義 ==========
const float DESTRUCTION_INCREMENT = 0.015f; // 割れやすく!
const float RECOVERY_SPEED = 0.005f; // 逆回転で復元
const int MAX_CRACKS = 500; // 500本!超細かく
const int MAX_PARTICLES = 0; // パーティクル無効化
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 240;

// ========== 音響定義 ==========
const int SOUND_TINY_CRACK = 1800;
const int SOUND_SMALL_CRACK = 2200;
const int SOUND_CRACK = 2600;
const int SOUND_BIG_CRACK = 3000;
const int SOUND_SHATTER = 3500;
const int SOUND_HEAVY_SHATTER = 4000;
const int SOUND_SILENCE = 600;
const int SOUND_RECOVERY = 1000;

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
  lastEncoderValue = M5Dial.Encoder.read();
}

// ========== メインループ ==========
void loop() {
  M5Dial.update();
  
  // エンコーダ読み取り
  long currentEncoder = M5Dial.Encoder.read();
  long delta = currentEncoder - lastEncoderValue;
  
  if (delta > 0) {
    // 正回転:破壊
    destructionLevel += delta * DESTRUCTION_INCREMENT;
    if (destructionLevel > 1.0f) destructionLevel = 1.0f;
  } else if (delta < 0) {
    // 逆回転:復元
    destructionLevel += delta * RECOVERY_SPEED; // deltaは負なので減る
    if (destructionLevel < 0.0f) {
      destructionLevel = 0.0f;
      cracks.clear();
    }
  }
  
  lastEncoderValue = currentEncoder;
  
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
  
  // 状態遷移時の音
  if (newState != currentState) {
    currentState = newState;
    playStateSound(currentState);
  }
  
  // ひび割れ状態では常にランダムにピシッピシッと追加
  if (currentState >= TINY_CRACK && currentState <= BIG_CRACK && cracks.size() < MAX_CRACKS) {
    // 回転するたびにランダムな位置にひび追加
    if (random(0, 100) > 50) { // 50%の確率でピシッ
      float randX = random(0, SCREEN_WIDTH);
      float randY = random(0, SCREEN_HEIGHT);
      float randAngle = random(0, 628) / 100.0f;
      addCrack(randX, randY, randAngle, 0);
    }
  }
  
  // 既存のひびから分岐
  if (currentState >= CRACK && currentState <= HEAVY_SHATTER && cracks.size() < MAX_CRACKS) {
    if (cracks.size() > 0 && random(0, 100) > 70) {
      int idx = random(0, cracks.size());
      Crack& c = cracks[idx];
      if (c.generation < 5) { // 5世代まで
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
  
  // より細かい線に
  float length = random(15, 40) / (generation + 1.0f);
  float x2 = x + cos(angle) * length;
  float y2 = y + sin(angle) * length;
  
  cracks.push_back(Crack(x, y, x2, y2, generation));
  
  // より複雑な分岐パターン
  if (generation < 5 && random(0, 100) > 30) {
    float branchAngle1 = angle + random(15, 90) / 100.0f;
    float branchAngle2 = angle - random(15, 90) / 100.0f;
    addCrack(x2, y2, branchAngle1, generation + 1);
    if (random(0, 100) > 40) { // 2本目は60%の確率
      addCrack(x2, y2, branchAngle2, generation + 1);
    }
  }
}

// ========== パーティクル生成 ==========
void createShatterParticles(int count) {
  // パーティクル無効化
  return;
}

// ========== 音再生 ==========
void playStateSound(GlassState state) {
  int freq = 0;
  int duration = 50; // 短く鋭く
  
  switch (state) {
    case TINY_CRACK: freq = SOUND_TINY_CRACK; break;
    case SMALL_CRACK: freq = SOUND_SMALL_CRACK; break;
    case CRACK: freq = SOUND_CRACK; break;
    case BIG_CRACK: freq = SOUND_BIG_CRACK; break;
    case SHATTER: freq = SOUND_SHATTER; duration = 100; break;
    case HEAVY_SHATTER: freq = SOUND_HEAVY_SHATTER; duration = 150; break;
    case SILENCE: freq = SOUND_SILENCE; duration = 200; break;
    default: return;
  }
  
  M5Dial.Speaker.tone(freq, duration);
}

// ========== 描画 ==========
void renderGlass() {
  // 黒背景のみ
  M5Dial.Display.fillScreen(TFT_BLACK);
  
  // ひび割れ描画 - 白い細い線
  for (auto& crack : cracks) {
    M5Dial.Display.drawLine((int)crack.x1, (int)crack.y1, (int)crack.x2, (int)crack.y2, TFT_WHITE);
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
  
  switch (currentState) {
    case NORMAL: stateName = s_normal; break;
    case TINY_CRACK: stateName = s_tiny; break;
    case SMALL_CRACK: stateName = s_small; break;
    case CRACK: stateName = s_crack; break;
    case BIG_CRACK: stateName = s_big; break;
    case SHATTER: stateName = s_shatter; break;
    case HEAVY_SHATTER: stateName = s_heavy; break;
    case SILENCE: stateName = s_silence; break;
  }
  
  if (stateName) {
    M5Dial.Display.setTextDatum(bottom_center);
    M5Dial.Display.drawString(stateName, 120, 230);
  }
}

// ========== 状態色取得 ==========
uint16_t getStateColor() {
  // 使わない - 常に黒背景
  return TFT_BLACK;
}
