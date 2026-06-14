/*
   -- 車輛狀態控制系統 - 藍牙 BLE 整合版 --
   引腳配置：紅燈 GPIO 23, 綠燈 GPIO 22, 藍燈 GPIO 21
*/

//////////////////////////////////////////////
//        RemoteXY include library          //
//////////////////////////////////////////////

// RemoteXY select connection mode and include library 
#define REMOTEXY_MODE__ESP32CORE_BLE

#include <BLEDevice.h>

// 📡 這是你的藍牙裝置名稱，待會手機就找這個名字
#define REMOTEXY_BLUETOOTH_NAME "RemoteXY"

#include <RemoteXY.h>

// RemoteXY GUI configuration  
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
  
// RemoteXY 變數結構（此處已自動更新為你的新變數 optionSelector_01）
struct {
  uint8_t optionSelector_01; // from 0 to 5
  uint8_t connect_flag;  
} RemoteXY;   
#pragma pack(pop)
 
/////////////////////////////////////////////
//           END RemoteXY include          //
/////////////////////////////////////////////

// ✅ 宣告你的 LED 引腳 
#define RED_PIN   23
#define GREEN_PIN 22
#define BLUE_PIN  21

int currentState = 1;
int lastState = -1;

// ⏳ 狀態 3 紅燈閃爍的非阻塞計時變數 (2Hz)
unsigned long previousMillis = 0;
bool redBlinkState = LOW;
const long interval = 250; 

void setup() 
{
  Serial.begin(115200);
  
  // 初始化 LED 引腳
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  RemoteXY_Init();  // 初始化藍牙 BLE 伺服器
  
  Serial.println("\n🟢 ESP32 藍牙 BLE 伺服器已啟動！");
  Serial.println("藍牙名稱為: RemoteXY");
  Serial.println("等待手機 App 連線中...");
}

void loop() 
{ 
  RemoteXYEngine.handler();   
  
  // 1. 核心邏輯轉換：將藍牙下拉選單的索引值（0, 1, 2, 3...）轉為你的 1~5 號狀態
  currentState = RemoteXY.optionSelector_01 + 1; 

  // 2. 燈號有限狀態機（非阻塞式 millis()）
  unsigned long currentMillis = millis();

  switch (currentState) {
    case 1: // 綠燈恆亮
      digitalWrite(GREEN_PIN, HIGH); 
      digitalWrite(RED_PIN, LOW); 
      digitalWrite(BLUE_PIN, LOW);  
      break;

    case 2: // 紅燈恆亮
      digitalWrite(GREEN_PIN, LOW);  
      digitalWrite(RED_PIN, HIGH); 
      digitalWrite(BLUE_PIN, LOW);  
      break;

    case 3: // 紅燈閃爍 (2Hz)
      digitalWrite(GREEN_PIN, LOW);  
      digitalWrite(BLUE_PIN, LOW);
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        redBlinkState = !redBlinkState;
        digitalWrite(RED_PIN, redBlinkState);
      }
      break;

    case 4: // 藍燈恆亮
      digitalWrite(GREEN_PIN, LOW);  
      digitalWrite(RED_PIN, LOW);  
      digitalWrite(BLUE_PIN, HIGH); 
      break;

    case 5: // 全滅狀態
      digitalWrite(GREEN_PIN, LOW);  
      digitalWrite(RED_PIN, LOW);  
      digitalWrite(BLUE_PIN, LOW);  
      break;
  }
  // 3. 當狀態改變時，在電腦端印出一次做雙重確認
  if (currentState != lastState) {
    lastState = currentState;
    Serial.print("📱 [藍牙指令] 狀態切換至模式: ");
    Serial.println(currentState);
  }
}