#define REMOTEXY_MODE__ESP32CORE_BLE

#include <BLEDevice.h>
#include <RemoteXY.h>

// ==========================================
// 1. RemoteXY 連線設定
// ==========================================
#define REMOTEXY_BLUETOOTH_NAME "車車" 

#pragma pack(push, 1)  
uint8_t const PROGMEM RemoteXY_CONF_PROGMEM[] =   
  { 255,2,0,0,0,22,0,19,0,0,0,0,31,1,106,200,1,1,1,0,
  5,35,87,38,38,0,2,26,31 };
  
struct {
  int8_t joystick_01_x; // 搖桿 X 軸 (-100 到 100)
  int8_t joystick_01_y; // 搖桿 Y 軸 (-100 到 100)
  uint8_t connect_flag;  
} RemoteXY;   
#pragma pack(pop)

// ==========================================
// 2. 腳位定義 (依據你最新的需求)
// ==========================================
// 馬達驅動腳位 (假設這是控制右側馬達)
#define ENB  14  // 記得拔掉 L298N ENB 上的跳線帽！
#define IN3_PIN  26 
#define IN4_PIN  27 // 必須是 IN4 才能與 ENB 搭配

// ==========================================
// setup() 初始設定
// ==========================================
void setup() {
  Serial.begin(115200);
  RemoteXY_Init();

  // 馬達方向腳位設定
  pinMode(IN3_PIN, OUTPUT); 
  pinMode(IN4_PIN, OUTPUT);
  
  // 馬達 PWM 腳位設定
  pinMode(ENB, OUTPUT);
}

// ==========================================
// loop() 主程式迴圈
// ==========================================
void loop() {
  RemoteXY_Handler(); 

  int x = RemoteXY.joystick_01_x;
  int y = RemoteXY.joystick_01_y;

  // 差速混控公式 (此處以右輪為例：Y - X)
  int rawRight = y - x; 

  // 將數值轉換為指定的 142~170 區間
  int rightMotorSpeed = convertToPWM(rawRight);

  // 輸出給馬達
  setMotorSpeed(rightMotorSpeed);
}

// ==========================================
// 🌟 專屬的 PWM 轉換函式 (限制 142 ~ 170)
// ==========================================
int convertToPWM(int rawSpeed) {
  // 死區防抖
  if (abs(rawSpeed) < 10) {
    return 0; 
  }
  
  // 將搖桿有效推力 (10~200) 映射到馬達有效轉速 (142~170)
  int pwm = map(abs(rawSpeed), 10, 200, 142, 170);
  
  // 確保數值絕對不會超出安全範圍
  pwm = constrain(pwm, 142, 170);

  // 補回正負號
  if (rawSpeed > 0) {
    return pwm;
  } else {
    return -pwm; 
  }
}

// ==========================================
// 驅動馬達副程式 (單側版本)
// ==========================================
void setMotorSpeed(int rightSpeed) {
  
  // --- 控制右側動力 ---
  if (rightSpeed > 0) {
    digitalWrite(IN3_PIN, HIGH); digitalWrite(IN4_PIN, LOW); // 前進
    analogWrite(ENB, rightSpeed);
  } else if (rightSpeed < 0) {
    digitalWrite(IN3_PIN, LOW); digitalWrite(IN4_PIN, HIGH); // 後退
    analogWrite(ENB, abs(rightSpeed)); // analogWrite 必須寫入正數
  } else {
    digitalWrite(IN3_PIN, LOW); digitalWrite(IN4_PIN, LOW);  // 煞車
    analogWrite(ENB, 0);
  }
}