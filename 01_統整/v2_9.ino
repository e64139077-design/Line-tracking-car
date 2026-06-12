/*
   --自走車 --
    2格寬賽道專用版（142~170 轉速嚴格限制版）
    整合 PID 權重平均法 + 非阻塞式轉向持續時間 + 直角彎記憶轉向
    
   RGB 燈號腳位：紅 21, 綠 22, 藍 23 
   左馬達腳位：ENA=12, IN1=16, IN2=17
   右馬達腳位：ENB=14, IN3=26, IN4=27
   循跡腳位(左至右)：34, 33, 32, 35, 5
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

// 硬體腳位定義
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

// 模式與系統變數
uint8_t currentState = 1;
uint8_t lastState = 0;
unsigned long previousMillis = 0;
unsigned long lastTrackPrint = 0; 
bool redBlinkState = LOW;
const uint16_t interval = 250; 
uint8_t lastSensorState = 255; 

// ==========================================
// 核心演算法參數與速度邊界調校
// ==========================================
// PID 參數限制在窄轉速區間時，初值建議不宜過大
float Kp = 2.8;                  
float Ki = 0.0;                  
float Kd = 1.0;                  

float last_error = 0;
float integral = 0;

// 嚴格轉速限制常數
const int TRACK_SPEED = 156;     // 直線基礎速度 (142~170 的中間值)
const int MAX_PWM = 178;         // 馬達上限轉速
const int MIN_PWM = 150;         // 馬達最低正轉啟動轉速

// 直角彎專用速度
const int SHARP_OUTER = 170;     // 直角彎外側輪最大出力
const int SHARP_INNER = -142;    // 直角彎內側輪反轉煞車出力

// ===== 非阻塞式轉向計時器變數 =====
unsigned long turnEndTime = 0;     // 轉向結束的時間點
bool turnLocked = false;           // 是否鎖定轉向動作
int savedLeftSpeed = 0;            // 鎖定的左輪速度
int savedRightSpeed = 0;           // 鎖定的右輪速度

// ===== 歷史記憶變數 =====
char lastTurnDirection = 'C';      // 記憶上一次方向 'L' / 'R' / 'C'
unsigned long lastCenterTime = 0;  // 最後一次置中的時間
bool isSharpTurn = false;          // 是否正在處理直角彎

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
  Serial.println(F("\n===== 系統啟動 (142~170 限速優化版) ====="));
  Serial.println(F("常規循跡: PID差速限速142~170 | 直角/脫軌: 170/-142 原地自旋"));
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
      turnLocked = false;
      isSharpTurn = false;
      integral = 0;
      last_error = 0;
      break;

    case 2: // 就緒模式
      setMotorSpeed(0, 0); 
      turnLocked = false;
      isSharpTurn = false;
      integral = 0;
      last_error = 0;
      if (currentMillis - lastTrackPrint > 300) {
        Serial.printf("[感測器(1=黑,0=白)] 左至右 -> 1:%d 2:%d 3:%d 4:%d 5:%d\n", 
          !digitalRead(TRACK_1_PIN), !digitalRead(TRACK_2_PIN), 
          !digitalRead(TRACK_3_PIN), !digitalRead(TRACK_4_PIN), !digitalRead(TRACK_5_PIN));
        lastTrackPrint = currentMillis;
      }
      break;

    case 3: // 自動導航 (整合 PID 權重法 + 142~178 嚴格限速區間)
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        redBlinkState = !redBlinkState;
        digitalWrite(RED_PIN, redBlinkState);
      }

      {
        // ----- [A] 轉向時間鎖定中，直接輸出鎖定速度並跳出 -----
        if (turnLocked) {
          if (currentMillis < turnEndTime) {
            setMotorSpeed(savedLeftSpeed, savedRightSpeed);
            break;
          } else {
            turnLocked = false;
            isSharpTurn = false;
            integral = 0;  // 結束計時鎖定後清空 PID 積分，防止恢復循跡時大扭動
            last_error = 0;
            Serial.println(F("時間鎖定結束，恢復常規 PID 限速循跡"));
          }
        }

        // ----- [B] 讀取原生感測器狀態 (1代表黑線，0代表白地) -----
        int s1 = !digitalRead(TRACK_1_PIN);
        int s2 = !digitalRead(TRACK_2_PIN);
        int s3 = !digitalRead(TRACK_3_PIN);
        int s4 = !digitalRead(TRACK_4_PIN);
        int s5 = !digitalRead(TRACK_5_PIN);
        uint8_t sensorState = (s1 << 4) | (s2 << 3) | (s3 << 2) | (s4 << 1) | s5;
        
        const __FlashStringHelper* statusMsg;
        bool needLockTurn = false;
        uint16_t turnDuration = 0;
        int leftSpd = 0, rightSpd = 0;

        // ----- [C] 時間特判：偵測是否太久沒回到置中（疑似直角彎特徵） -----
        if (sensorState == 0b01110 || sensorState == 0b01100 || sensorState == 0b00110 || sensorState == 0b00100) {
          lastCenterTime = currentMillis;
        } else {
          if (currentMillis - lastCenterTime > 300 && lastTurnDirection != 'C') {
            if (!isSharpTurn) {
              isSharpTurn = true;
              Serial.println(F("!! 警告：長時間未置中，直角彎預備 !!"));
            }
          }
        }

        // ----- [D] 分流邏輯：全黑、全白特判與常規 PID 循跡 -----

        // 1. 全黑特判：直角彎橫切 (11111) 或嚴重壓線
        if (sensorState == 0b11111 || sensorState == 0b01111 || sensorState == 0b11110) {
          if (lastTurnDirection == 'L') {
            leftSpd = SHARP_OUTER; rightSpd = SHARP_INNER; // 170 / -142 強制原地左自旋
            statusMsg = F("全黑-記憶強制左轉 (170/-142)");
            needLockTurn = true;
            turnDuration = isSharpTurn ? 280 : 80;  
          } else if (lastTurnDirection == 'R') {
            leftSpd = SHARP_INNER; rightSpd = SHARP_OUTER; // -142 / 170 強制原地右自旋
            statusMsg = F("全黑-記憶強制右轉 (-142/170)");
            needLockTurn = true;
            turnDuration = isSharpTurn ? 280 : 80;
          } else {
            leftSpd = 0; rightSpd = 0;
            statusMsg = F("全黑停止線");
          }
        }
        // 2. 全白特判：脫軌找線 (00000)
        else if (sensorState == 0b00000) {
          if (lastTurnDirection == 'L') {
            leftSpd = MAX_PWM; rightSpd = -MIN_PWM; // 170 / -142 強制原地急左轉
            statusMsg = F("全白脫軌-大動作左轉找線 (170/-142)");
            needLockTurn = true;
            turnDuration = 150;
          } else if (lastTurnDirection == 'R') {
            leftSpd = -MIN_PWM; rightSpd = MAX_PWM; // -142 / 170 強制原地急右轉
            statusMsg = F("全白脫軌-大動作右轉找線 (-142/170)");
            needLockTurn = true;
            turnDuration = 150;
          } else {
            leftSpd = 0; rightSpd = 0;
            statusMsg = F("完全脫軌停止");
          }
        }
        // 3. 常規狀態：使用權重平均法 + PID 控制器（帶有 142~170 嚴格限速邏輯）
        else {
          // 計算權重分子與分母 (感測器權重：S1=-20, S2=-10, S3=0, S4=10, S5=20)
          float num = (s1 * -20) + (s2 * -10) + (s3 * 0) + (s4 * 10) + (s5 * 20);
          float den = (s1 + s2 + s3 + s4 + s5);
          float current_error = num / den;

          // 更新方向記憶
          if (current_error < -3)      lastTurnDirection = 'L'; // 線在左邊
          else if (current_error > 3)  lastTurnDirection = 'R'; // 線在右邊
          else if (sensorState == 0b01110 || sensorState == 0b00100) lastTurnDirection = 'C'; // 完美置中

          // PID 運算
          float P = current_error;
          integral += current_error;
          float D = current_error - last_error;
          float PID_output = (Kp * P) + (Ki * integral) + (Kd * D);
          last_error = current_error;

          // 透過 PID 輸出計算左右差速
          leftSpd  = TRACK_SPEED + PID_output;
          rightSpd = TRACK_SPEED - PID_output;

          //
          if (PID_output > 1) {
            // PID 輸出為正，左輪變快、右輪變慢 -> 車子往右修正
            statusMsg = F("PID 限速循跡 [→ 向右彎]");
          } else if (PID_output < -1) {
            // PID 輸出為負，左輪變慢、右輪變快 -> 車子往左修正
            statusMsg = F("PID 限速循跡 [← 向左彎]");
          } else {
            statusMsg = F("PID 限速循跡 [↑ 直行置中]");
          }

          // 【 142 ~ 170 轉速區間限制核心邏輯 】
          // 左輪限速處理
          if (leftSpd > TRACK_SPEED) {
            leftSpd = constrain(leftSpd, TRACK_SPEED, MAX_PWM); // 加速修正，最高 170
          } else {
            // 減速修正，如果低於最低啟動轉速 142，直接變為 0 (觸發軸心轉向)，否則鎖在 142 區間
            leftSpd = (leftSpd < MIN_PWM) ? 0 : constrain(leftSpd, MIN_PWM, TRACK_SPEED); 
          }

          // 右輪限速處理
          if (rightSpd > TRACK_SPEED) {
            rightSpd = constrain(rightSpd, TRACK_SPEED, MAX_PWM); // 最高 170
          } else {
            rightSpd = (rightSpd < MIN_PWM) ? 0 : constrain(rightSpd, MIN_PWM, TRACK_SPEED); // 最低 142 或 0
          }

        }

        // 設定馬達速度
        setMotorSpeed(leftSpd, rightSpd);

        // ----- [E] 如果觸發了特判（全黑/全白），啟動非阻塞計時鎖定 -----
        if (needLockTurn) {
          turnLocked = true;
          turnEndTime = currentMillis + turnDuration;
          savedLeftSpeed = leftSpd;
          savedRightSpeed = rightSpd;
          
          Serial.printf("觸發時間鎖定 [%s] L:%d R:%d 時間:%dms\n", 
                        isSharpTurn ? "直角彎" : "常規修正", savedLeftSpeed, savedRightSpeed, turnDuration);
        }

        // ----- [F] 狀態除錯顯示 (僅在感測器變更時更新) -----
        if (sensorState != lastSensorState) {
          Serial.print(F("狀態: [ ")); 
          for(int i=4; i>=0; i--) Serial.print(bitRead(sensorState, i));
          Serial.print(F(" ] ==> ")); 
          Serial.println(statusMsg);
          lastSensorState = sensorState; 
        }
      }
      break;

    case 4: // 手動遙控 (沿用原本轉換與壓縮邏輯)
      {
        turnLocked = false;
        isSharpTurn = false;
        int x = RemoteXY.joystick_01_x;
        int y = RemoteXY.joystick_01_y;
        int rawLeft  = y + x; 
        int rawRight = y - x; 
        setMotorSpeed(convertToPWM(rawLeft), convertToPWM(rawRight));
      }
      break;

    case 5: // 休眠模式
      setMotorSpeed(0, 0); 
      turnLocked = false;
      isSharpTurn = false;
      break;
  }
}

// PWM 安全轉換與壓縮函式 (遙控專用，維持 142~170)
// ==========================================
int convertToPWM(int rawSpeed) {
  if (abs(rawSpeed) < 10) return 0; 
  int pwm = constrain(map(abs(rawSpeed), 10, 200, 142, 170), 142, 170);
  return (rawSpeed > 0) ? pwm : -pwm; 
}

// 底層馬達驅動函式
// ==========================================
void setMotorSpeed(int leftSpeed, int rightSpeed) {
  // 限幅保護，防止任何極端計算衝破馬達安全上限 (ESP32 analogWrite 預設 0~255)
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

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
