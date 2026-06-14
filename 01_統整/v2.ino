/*
   ========================================================
     新增:防呆測試 循跡數值輸出(3路) + 小車動作監控 
   ========================================================
    燈號腳位：紅 12, 綠 13, 藍 14
    馬達腳位：IN1~4 -> 16, 17, 18, 19
    循跡腳位：左 33, 中 32, 右 35
    改 : 未加轉速控制 之後用 PWM 調整速度 ,joystick 來控制速度 
    循跡演算法測試
*/

#define REMOTEXY_MODE__ESP32CORE_BLE
#include <BLEDevice.h>
#define REMOTEXY_BLUETOOTH_NAME "車車" 
#include <RemoteXY.h>

// 1.介面配置碼
#pragma pack(push, 1)  
uint8_t const PROGMEM RemoteXY_CONF_PROGMEM[] =   
  { 255,5,0,0,0,177,0,19,0,0,0,232,187,138,232,187,138,0,31,1,
  106,200,1,1,5,0,1,41,66,20,20,0,2,31,226,172,134,0,1,41,
  112,21,21,0,2,31,226,172,135,0,1,15,89,21,21,0,2,31,226,134,
  182,0,1,66,89,21,21,0,2,31,226,134,183,0,12,17,42,74,11,195,
  30,26,229,174,137,229,133,168,231,139,128,230,133,139,32,40,231,182,160,231,
  135,136,41,0,229,176,177,231,183,146,231,139,128,230,133,139,32,40,231,180,
  133,231,135,136,41,0,232,135,170,228,184,187,229,176,142,232,136,170,32,40,
  233,150,131,231,136,141,41,0,32,233,129,153,230,142,167,230,184,172,232,169,
  166,32,40,232,151,141,231,135,136,41,0,229,133,168,230,187,133,231,139,128,
  230,133,139,0 };
  
struct {
  uint8_t button_01; 
  uint8_t button_02; 
  uint8_t button_03; 
  uint8_t button_04; 
  uint8_t optionSelector_01; 
  uint8_t connect_flag; 
} RemoteXY;   
#pragma pack(pop)

// ==========================================
// 2. 腳位設定
// ==========================================
#define RED_PIN   12
#define GREEN_PIN 13
#define BLUE_PIN  14

#define IN1_PIN 16 
#define IN2_PIN 17 
#define IN3_PIN 18 
#define IN4_PIN 19 
//簡化3路
#define TRACK_LEFT_PIN  33  
#define TRACK_MID_PIN   32  
#define TRACK_RIGHT_PIN 35  

int currentState = 1;
int lastState = -1;

unsigned long previousMillis = 0;
unsigned long lastTrackPrint = 0; 
bool redBlinkState = LOW;

// 2Hz 閃爍間隔
const long interval = 250; 

// 用來記憶小車「上一個動作」的變數 (防洗版用)
int lastAction = -1; 

// ==========================================
// 🛠️ 3. Setup 初始化
// ==========================================
void setup() {
  Serial.begin(115200);
  
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);

  pinMode(TRACK_LEFT_PIN, INPUT);
  pinMode(TRACK_MID_PIN, INPUT);
  pinMode(TRACK_RIGHT_PIN, INPUT);

  motorStop(); 
  RemoteXY_Init();  
  Serial.println("\n===== 🚀 小車系統啟動完畢 =====");
}
// 4. Loop 主程式
// ==========================================
void loop() {
  RemoteXYEngine.handler(); 
  unsigned long currentMillis = millis();

  // 失控保護機制(安全模式)：如果藍牙斷線，就強制切回安全模式
  if (RemoteXY.connect_flag == 0) {
    currentState = 1; 
  } else {
    currentState = RemoteXY.optionSelector_01 + 1; 
  }

  // 燈號更新邏輯 (防止閃爍)
  if (currentState != lastState) {
    lastState = currentState;
    
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(RED_PIN, LOW);
    digitalWrite(BLUE_PIN, LOW);

    if (currentState == 1) { digitalWrite(GREEN_PIN, HIGH); }
    else if (currentState == 2) { digitalWrite(RED_PIN, HIGH); }
    else if (currentState == 4) { digitalWrite(BLUE_PIN, HIGH); }
    
    Serial.print("\n📱 模式切換至: "); Serial.println(currentState);
  }

  // 將感測器變數與當下動作宣告在 switch 外面
  int trackL = 0;
  int trackM = 0;
  int trackR = 0;
  int currentAction = 0; // 用來記錄這一瞬間決定要做什麼動作

  // [小車狀態]
  switch (currentState) {
    
    case 1: // 🟢 安全模式
      motorStop(); 
      break;

    case 2: // 🔴 就緒模式 + 循跡數值校正測試
      motorStop(); 
      if (currentMillis - lastTrackPrint > 300) {
        trackL = digitalRead(TRACK_LEFT_PIN);
        trackM = digitalRead(TRACK_MID_PIN);
        trackR = digitalRead(TRACK_RIGHT_PIN);
        
        Serial.print("📡 [感測器數值] 左:"); Serial.print(trackL);
        Serial.print(" 中:"); Serial.print(trackM);
        Serial.print(" 右:"); Serial.println(trackR);
        lastTrackPrint = currentMillis;
      }
      break;

    case 3: // ⚡ 自主導航模式 (紅燈 2Hz 閃爍 + 循跡自動駕駛)
      
      // 1. 紅燈 2Hz 閃爍
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        redBlinkState = !redBlinkState;
        digitalWrite(RED_PIN, redBlinkState);
      }

      // 2. 讀取循跡感測器訊號
      trackL = digitalRead(TRACK_LEFT_PIN);
      trackM = digitalRead(TRACK_MID_PIN);
      trackR = digitalRead(TRACK_RIGHT_PIN);

      // 3. 決定馬達動作與記錄
      if (trackM == 1) {
        motorForward();   
        currentAction = 1; // 狀態 1：直走
      } 
      else if (trackL == 1) {
        motorLeft();      
        currentAction = 2; // 狀態 2：向左修正
      } 
      else if (trackR == 1) {
        motorRight();     
        currentAction = 3; // 狀態 3：向右修正
      } 
      else {
        motorStop();      
        currentAction = 0; // 狀態 0：煞車停止
      }

      // 4. 監控輸出：只有當「決定做的動作」跟「上一次」不一樣時，才印出文字
      if (currentAction != lastAction) {
        if (currentAction == 1)      { Serial.println("🛣️ 中間壓線 ➡️ 直走！"); }
        else if (currentAction == 2) { Serial.println("👈 左邊壓線 ➡️ 向左甩尾修正！"); }
        else if (currentAction == 3) { Serial.println("👉 右邊壓線 ➡️ 向右甩尾修正！"); }
        else if (currentAction == 0) { Serial.println("🛑 完全沒壓線 ➡️ 煞車尋軌..."); } 
        
        lastAction = currentAction; // 更新記憶
      }
      break;

    case 4: // 🔵 遙控模式
      if (RemoteXY.button_01 == 1) { motorForward(); }
      else if (RemoteXY.button_02 == 1) { motorBackward(); }
      else if (RemoteXY.button_03 == 1) { motorLeft(); }
      else if (RemoteXY.button_04 == 1) { motorRight(); }
      else { motorStop(); }
      break;

    case 5: // 休眠模式 (燈全滅 + 馬達停止)
      digitalWrite(GREEN_PIN, LOW);
      digitalWrite(RED_PIN, LOW);
      digitalWrite(BLUE_PIN, LOW);
      motorStop(); 
      break;
  }
}
// 5. 馬達控制
// ==========================================
void motorForward() {
  digitalWrite(IN1_PIN, HIGH); digitalWrite(IN2_PIN, LOW);  
  digitalWrite(IN3_PIN, HIGH); digitalWrite(IN4_PIN, LOW);  
}

void motorBackward() {
  digitalWrite(IN1_PIN, LOW);  digitalWrite(IN2_PIN, HIGH); 
  digitalWrite(IN3_PIN, LOW);  digitalWrite(IN4_PIN, HIGH); 
}

void motorLeft() {
  digitalWrite(IN1_PIN, LOW);  digitalWrite(IN2_PIN, HIGH); 
  digitalWrite(IN3_PIN, HIGH); digitalWrite(IN4_PIN, LOW);  
}

void motorRight() {
  digitalWrite(IN1_PIN, HIGH); digitalWrite(IN2_PIN, LOW);  
  digitalWrite(IN3_PIN, LOW);  digitalWrite(IN4_PIN, HIGH); 
}

void motorStop() {
  digitalWrite(IN1_PIN, LOW); digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, LOW); digitalWrite(IN4_PIN, LOW);
}