/*
   -- 車輛狀態控制系統：藍牙遙控(V1.0) + 5路循線感測器 整合版 --
   燈號引腳：紅 18, 綠 21, 藍 19
   感測引腳：S1->5, S2->17, S3->16, S4->4, S5->2
*/

//////////////////////////////////////////////
//        RemoteXY include library          //
//////////////////////////////////////////////
#define REMOTEXY_MODE__ESP32CORE_BLE
#include <BLEDevice.h>
// 📡 藍牙裝置名稱
#define REMOTEXY_BLUETOOTH_NAME "車車"
#include <RemoteXY.h>
// RemoteXY GUI 配置碼
#pragma pack(push, 1)  
uint8_t const PROGMEM RemoteXY_CONF_PROGMEM[] =   // 150 bytes V19 
  { 255,1,0,0,0,143,0,19,0,0,0,0,31,1,106,200,1,1,2,0,
  12,17,30,74,11,195,30,26,229,174,137,229,133,168,231,139,128,230,133,139,
  32,40,231,182,160,231,135,136,41,0,229,176,177,231,183,146,231,139,128,230,
  133,139,32,40,231,180,133,231,135,136,41,0,232,135,170,228,184,187,229,176,
  142,232,136,170,32,40,233,150,131,231,136,141,41,0,32,233,129,153,230,142,
  167,230,184,172,232,169,166,32,40,232,151,141,231,135,136,41,0,229,133,168,
  230,187,133,231,139,128,230,133,139,0,129,32,19,40,10,64,17,232,187,138,
  232,187,138,230,168,161,229,188,143,0 };
  
struct {
  uint8_t optionSelector_01; // 下拉選單元件
  uint8_t connect_flag;  
} RemoteXY;   
#pragma pack(pop)
// 引腳定義
#define RED_PIN   18
#define GREEN_PIN 21
#define BLUE_PIN  19
//  5路感測器引腳與數值變數
const int sensorPins[5] = {5, 17, 16, 4, 2}; 
int sensorValues[5] = {0, 0, 0, 0, 0}; // 狀態變數
//
int currentState = 1;
int lastState = -1;
//  計時器 A：紅燈 2Hz 閃爍 (250ms 翻轉一次)
unsigned long previousMillis = 0;
bool redBlinkState = LOW;
const long interval = 250; 
//  計時器 B：🆕 自主導航模式下，每 500ms 讀取/印出一次感測器數值
unsigned long previousSensorMillis = 0;
const long sensorInterval = 500; 
void setup() {
  Serial.begin(115200);
  
  // 初始化燈號與感測器腳位
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  for (int i = 0; i < 5; i++) {
    pinMode(sensorPins[i], INPUT);
  }
  
  RemoteXY_Init();  // 啟動藍牙
  Serial.println("\n===== 系統初始化完畢，藍牙 BLE 已就緒 =====");
}

void loop() {
  RemoteXYEngine.handler();   
  
  // 讀取手機藍牙端傳來的狀態 (0~4 轉為 1~5)
  currentState = RemoteXY.optionSelector_01 + 1; 

  unsigned long currentMillis = millis();

  switch (currentState) {
    case 1: // 綠燈恆亮：安全狀態
      digitalWrite(GREEN_PIN, HIGH); digitalWrite(RED_PIN, LOW);  digitalWrite(BLUE_PIN, LOW);  
      break;

    case 2: // 紅燈恆亮：就緒狀態
      digitalWrite(GREEN_PIN, LOW);  digitalWrite(RED_PIN, HIGH); digitalWrite(BLUE_PIN, LOW);  
      break;

    case 3: // 🔴 紅燈閃爍 +  啟動循線自主導航
      digitalWrite(GREEN_PIN, LOW);  digitalWrite(BLUE_PIN, LOW);
      
      // 處理紅燈 2Hz 閃爍
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        redBlinkState = !redBlinkState;
        digitalWrite(RED_PIN, redBlinkState);
      }

      //  每 0.5 秒醒來讀取並印出感測器數值
      if (currentMillis - previousSensorMillis >= sensorInterval) {
        previousSensorMillis = currentMillis;
        
        Serial.print("🤖 [自主導航中] 感測器 S1~S5: ");
        for (int i = 0; i < 5; i++) {
          sensorValues[i] = digitalRead(sensorPins[i]); // 實時讀取 0 或 1
          Serial.print(sensorValues[i]);
          Serial.print(" ");
        }
        Serial.println(" | ⚙️ 車子前進中！");
        
        /* 
        馬達控制邏輯就可以直接寫在這裡，根據 sensorValues[] 的狀態來決定車子的行進方向。
        */
      }
      break;

    case 4: // 藍燈恆亮：遙控或測試狀態
      digitalWrite(GREEN_PIN, LOW);  digitalWrite(RED_PIN, LOW);  digitalWrite(BLUE_PIN, HIGH); 
      break;
      Serial.println(" | ⚙️ 使用中！");
        
        /* 
        直接寫在這裡，根據 sensorValues[] 的狀態來決定車子的行進方向。
        */
      }
    case 5: // 全滅狀態
      digitalWrite(GREEN_PIN, LOW);  digitalWrite(RED_PIN, LOW);  digitalWrite(BLUE_PIN, LOW);  
      break;
  }

  // 狀態切換提示
  if (currentState != lastState) {
    lastState = currentState;
    Serial.print("\n📱 [狀態切換] 目前模式: ");
    Serial.println(currentState);
  }
}
