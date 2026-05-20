//測試 RemoteXY 藍牙連線與數值接收的基本範例程式 不完整
// ==========================================
// 1. RemoteXY 連線設定 (使用 ESP32 藍牙模式)
// ==========================================
#define REMOTEXY_MODE__ESP32CORE_BLUETOOTH
#include <RemoteXY.h>

// 2. 藍牙名稱設定 (這是手機搜尋藍牙時看到的名字，可自訂)
#define REMOTEXY_BLUETOOTH_NAME "My_ESP32_Car"

// ==========================================
// 3. RemoteXY UI 配置陣列 (這是範例：1個搖桿 + 1個按鈕)
// ==========================================
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =
  { 255,3,0,0,0,27,2,106,200,1,
  5,32,17,28,30,30,2,26,31,1,
  2,0,67,23,22,22,2,31,88,0 };

// 4. RemoteXY 變數結構 (用來接收手機傳來的數值)
// 💡 這邊也要跟著網頁生成的結構一起替換
struct {
  int8_t joystick_1_x;  // 搖桿 X 軸 (數值 -100 到 100)
  int8_t joystick_1_y;  // 搖桿 Y 軸 (數值 -100 到 100)
  uint8_t button_1;     // 按鈕狀態 (按下為 1，放開為 0)
  uint8_t connect_flag; // 連線狀態指示 (連線時為 1，斷線時為 0)
} RemoteXY;
#pragma pack(pop)
// ==========================================

void setup() {
    Serial.begin(115200);
    
    // 初始化 RemoteXY 通訊 (這行非常重要，絕對不能少)
    RemoteXY_Init();
    
    Serial.println("ESP32 藍牙已啟動！");
    Serial.println("請打開手機 RemoteXY App，尋找藍牙名稱: My_ESP32_Car");
}

void loop() {
    // 每次迴圈都要呼叫這行，讓 ESP32 處理手機傳來的藍牙封包
    RemoteXY_Handler();

    // ==========================================
    // ⬇️ 控制邏輯 ⬇️
    // ==========================================
    
    // 為了避免序列監控視窗被文字洗版當機，我們設定每 500 毫秒 (0.5秒) 印一次數值
    static unsigned long lastPrintTime = 0;
    if (millis() - lastPrintTime > 500) {
        
        Serial.print("搖桿 X: ");
        Serial.print(RemoteXY.joystick_1_x);
        Serial.print(" | 搖桿 Y: ");
        Serial.print(RemoteXY.joystick_1_y);
        
        // 判斷按鈕是否被按下
        if (RemoteXY.button_1 == 1) {
            Serial.print(" | 🔴 按鈕被按下了！");
        }
        
        Serial.println();
        lastPrintTime = millis(); // 更新時間紀錄
    }
}