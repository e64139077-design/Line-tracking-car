/*
   --自走車 --
    修正版：循跡感測器 0 與 1 訊號互換 (反相讀取) 
   
   RGB 燈號腳位：紅 21, 綠 22, 藍 23 
   左馬達腳位：ENA=12, IN1=16, IN2=17
   右馬達腳位：ENB=14, IN3=26, IN4=27
   循跡腳位(左至右)：34, 33, 32, 35, 36
*/

#define REMOTEXY_MODE__ESP32CORE_BLE
#include <BLEDevice.h>
#define REMOTEXY_BLUETOOTH_NAME "RemoteXY"
#include <RemoteXY.h>

#pragma pack(push, 1)  
uint8_t const PROGMEM RemoteXY_CONF_PROGMEM[] =   // 162 bytes V19 
  { 255,3,0,0,0,155,0,19,0,0,0,232,187,138,232,187,138,0,31,1,
  106,200,1,1,3,0,12,17,42,74,11,195,30,26,229,174,137,229,133,168,
  231,139,128,230,133,139,32,40,231,182,160,231,135,136,41,0,229,176,177,231,
  183,146,231,139,128,230,133,139,32,40,231,180,133,231,135,136,41,0,232,135,
  170,228,184,187,229,176,142,232,136,170,32,40,233,150,131,231,136,141,41,0,
  32,233,129,153,230,142,167,230,184,172,232,169,166,32,40,232,151,141,231,135,
  136,41,0,229,133,168,230,187,133,231,139,128,230,133,139,0,5,36,94,33,
  33,0,2,26,31,129,35,68,36,12,64,17,230,150,185,229,144,145,231,155,
  164,0 };
  
struct {
  uint8_t optionSelector_01; // from 0 to 5
  int8_t joystick_01_x;      // from -100 to 100
  int8_t joystick_01_y;      // from -100 to 100
  uint8_t connect_flag;  
} RemoteXY;   
#pragma pack(pop)


// 硬體腳位與全域變數定義
// ==========================================
#define RED_PIN   21
#define GREEN_PIN 22
#define BLUE_PIN  23

#define ENA      12  
#define IN1_PIN  16 
#define IN2_PIN  17 

#define ENB      14  
#define IN3_PIN  26 
#define IN4_PIN  27 

#define TRACK_1_PIN 34 
#define TRACK_2_PIN 33 
#define TRACK_3_PIN 32 
#define TRACK_4_PIN 35 
#define TRACK_5_PIN 5 

uint8_t currentState = 1;
uint8_t lastState = 0;
unsigned long previousMillis = 0;
unsigned long lastTrackPrint = 0; 
bool redBlinkState = LOW;
const uint16_t interval = 250; 
uint8_t lastSensorState = 255; 

// 自走車自動循跡基礎速度
const int TRACK_SPEED = 120; 

// ==========================================
// setup() 初始設定
// ==========================================
void setup() 
{
  Serial.begin(115200);
  RemoteXY_Init ();  
  
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(TRACK_1_PIN, INPUT);
  pinMode(TRACK_2_PIN, INPUT);
  pinMode(TRACK_3_PIN, INPUT);
  pinMode(TRACK_4_PIN, INPUT);
  pinMode(TRACK_5_PIN, INPUT);

  setMotorSpeed(0, 0); 
  Serial.println(F("\n===== 系統啟動 ====="));
}

// loop() 主程式迴圈
// ==========================================
void loop() 
{ 
  RemoteXYEngine.handler ();   
  unsigned long currentMillis = millis();

  currentState = (RemoteXY.connect_flag == 0) ? 1 : (RemoteXY.optionSelector_01 + 1);

  if (currentState != lastState) {
    lastState = currentState;
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(RED_PIN, LOW);
    digitalWrite(BLUE_PIN, LOW);
    if (currentState == 1) { digitalWrite(GREEN_PIN, HIGH); } 
    else if (currentState == 2) { digitalWrite(RED_PIN, HIGH); } 
    else if (currentState == 4) { digitalWrite(BLUE_PIN, HIGH); } 
    Serial.print(F("\n模式切換至: ")); Serial.println(currentState);
  }

  switch (currentState) {
    
    case 1: // 安全模式
      setMotorSpeed(0, 0); 
      break;

    case 2: // 就緒模式 (加上 ! 反相輸出，讓你看到轉換後的真實邏輯)
      setMotorSpeed(0, 0); 
      if (currentMillis - lastTrackPrint > 300) {
        Serial.printf("[感測器(已反相)] 左至右 -> 1:%d 2:%d 3:%d 4:%d 5:%d\n", 
          !digitalRead(TRACK_1_PIN), !digitalRead(TRACK_2_PIN), 
          !digitalRead(TRACK_3_PIN), !digitalRead(TRACK_4_PIN), !digitalRead(TRACK_5_PIN));
        lastTrackPrint = currentMillis;
      }
      break;

    case 3: // 自動導航
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        redBlinkState = !redBlinkState;
        digitalWrite(RED_PIN, redBlinkState);
      }

      {
        // 改版:在所有的 digitalRead 前面加上 符號，把 0 跟 1 直接顛倒 
        uint8_t sensorState = (!digitalRead(TRACK_1_PIN) << 4) | 
                              (!digitalRead(TRACK_2_PIN) << 3) | 
                              (!digitalRead(TRACK_3_PIN) << 2) | 
                              (!digitalRead(TRACK_4_PIN) << 1) | 
                              !digitalRead(TRACK_5_PIN);
        
        const __FlashStringHelper* statusMsg; 

        // 底下邏輯完全不需修改，0b01110 依然代表中間有線
        if (sensorState == 0b01110) {
          setMotorSpeed(TRACK_SPEED, TRACK_SPEED); 
          statusMsg = F("完美置中");
        } 
        else if (sensorState == 0b00111 || sensorState == 0b00110 || sensorState == 0b00011 || sensorState == 0b00010 || sensorState == 0b00001) {
          setMotorSpeed(TRACK_SPEED, -TRACK_SPEED); 
          statusMsg = F("偏左 (需向右轉)");
        } 
        else if (sensorState == 0b11100 || sensorState == 0b01100 || sensorState == 0b11000 || sensorState == 0b01000 || sensorState == 0b10000) {
          setMotorSpeed(-TRACK_SPEED, TRACK_SPEED); 
          statusMsg = F("偏右 (需向左轉)");
        } 
        else if (sensorState == 0b11111 || sensorState == 0b01111 || sensorState == 0b11110) {
          setMotorSpeed(0, 0); 
          statusMsg = F("遇到十字路口/停止線");
        } 
        else if (sensorState == 0b00000) {
          setMotorSpeed(0, 0); 
          statusMsg = F("完全脫軌");
        } 
        else {
          setMotorSpeed(0, 0);
          statusMsg = F("未知雜訊");
        }

        if (sensorState != lastSensorState) {
          Serial.print(F("狀態: [ ")); 
          for(int i=4; i>=0; i--) Serial.print(bitRead(sensorState, i));
          Serial.print(F(" ] ==> ")); 
          Serial.println(statusMsg);
          lastSensorState = sensorState; 
        }
      }
      break;

    case 4: // 手動遙控
      {
        int x = RemoteXY.joystick_01_x;
        int y = RemoteXY.joystick_01_y;

        int rawLeft  = y + x; 
        int rawRight = y - x; 

        int leftMotorSpeed  = convertToPWM(rawLeft); 
        int rightMotorSpeed = convertToPWM(rawRight);

        setMotorSpeed(leftMotorSpeed, rightMotorSpeed);
      }
      break;

    case 5: // 【模式 5】休眠模式
      setMotorSpeed(0, 0); 
      break;
  }
}

// PWM 安全轉換與壓縮函式 
// ==========================================
int convertToPWM(int rawSpeed) {
  if (abs(rawSpeed) < 10) return 0; 
  int pwm = constrain(map(abs(rawSpeed), 10, 200, 142, 170), 142, 170); // 將 10~200 映射到 142~170 的 PWM 範圍，並限制在 142~170 之內
  return (rawSpeed > 0) ? pwm : -pwm; 
}

// 底層馬達驅動函式
// ==========================================
void setMotorSpeed(int leftSpeed, int rightSpeed) {
  if (leftSpeed == 0) {
    digitalWrite(IN1_PIN, LOW); digitalWrite(IN2_PIN, LOW);  
    analogWrite(ENA, 0);
  } else {
    digitalWrite(IN1_PIN, leftSpeed > 0); 
    digitalWrite(IN2_PIN, leftSpeed < 0); 
    analogWrite(ENA, abs(leftSpeed));     
  }

  if (rightSpeed == 0) {
    digitalWrite(IN3_PIN, LOW); digitalWrite(IN4_PIN, LOW);  
    analogWrite(ENB, 0);
  } else {
    digitalWrite(IN3_PIN, rightSpeed > 0); 
    digitalWrite(IN4_PIN, rightSpeed < 0); 
    analogWrite(ENB, abs(rightSpeed));    
  }
}