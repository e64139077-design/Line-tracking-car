// 25組循跡自走車 可以動作版
// 可以作用 問題: 做第2個直角灣後會直走暴衝
   //可能須新增 偵測到2秒後  暫停倒車  第4秒停止 

#define REMOTEXY_MODE__ESP32CORE_BLE
#include <BLEDevice.h>
#define REMOTEXY_BLUETOOTH_NAME "車車"
#include <RemoteXY.h>
#pragma pack(push, 1)  
uint8_t const PROGMEM RemoteXY_CONF_PROGMEM[] =   
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
  uint8_t optionSelector_01; 
  int8_t joystick_01_x;      
  int8_t joystick_01_y;      
  uint8_t connect_flag;  
} RemoteXY;   
#pragma pack(pop)
// 硬體腳位定義
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
unsigned long lastTrackPrint = 0; 
uint8_t lastSensorState = 255; 
unsigned long lastLedToggleMillis = 0; 
bool ledState = LOW;                  
const int TRACK_SPEEDL = 115;     // 左直線基準速度  
const int TRACK_SPEEDR = 115;     // 右直線基準速度
float Kp = 2.4;  // 比例常數
float Kd =0.98;  // 微分常數

int error = 0;       
int lastError = 0;   

void setup() 
{
  Serial.begin(115200);
  RemoteXY_Init ();  
  pinMode(RED_PIN, OUTPUT); pinMode(GREEN_PIN, OUTPUT); pinMode(BLUE_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT); pinMode(IN2_PIN, OUTPUT); pinMode(ENA, OUTPUT);
  pinMode(IN3_PIN, OUTPUT); pinMode(IN4_PIN, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(TRACK_1_PIN, INPUT); pinMode(TRACK_2_PIN, INPUT); pinMode(TRACK_3_PIN, INPUT);
  pinMode(TRACK_4_PIN, INPUT); pinMode(TRACK_5_PIN, INPUT);

  setMotorSpeed(0, 0); 
  digitalWrite(GREEN_PIN, HIGH); digitalWrite(RED_PIN, LOW); digitalWrite(BLUE_PIN, LOW);
  Serial.println(F("\n系統啟動"));
}

void loop() 
{ 
  // 控制燈號
  RemoteXYEngine.handler ();   
  unsigned long currentMillis = millis();
  currentState = (RemoteXY.connect_flag == 0) ? 1 : (RemoteXY.optionSelector_01 + 1);
  if (currentState != lastState) {
    lastState = currentState;
    if (currentState == 1) { 
      digitalWrite(GREEN_PIN, HIGH); digitalWrite(RED_PIN, LOW); digitalWrite(BLUE_PIN, LOW);
    } else if (currentState == 2) {
      digitalWrite(GREEN_PIN, LOW); digitalWrite(RED_PIN, HIGH); digitalWrite(BLUE_PIN, LOW);
    } else if (currentState == 3) {
      digitalWrite(GREEN_PIN, LOW); digitalWrite(BLUE_PIN, LOW);
      ledState = HIGH; digitalWrite(RED_PIN, ledState); lastLedToggleMillis = currentMillis;
    } else if (currentState == 4) {
      digitalWrite(GREEN_PIN, LOW); digitalWrite(RED_PIN, LOW); digitalWrite(BLUE_PIN, HIGH);
    } else if (currentState == 5) {
      digitalWrite(GREEN_PIN, LOW); digitalWrite(RED_PIN, LOW); digitalWrite(BLUE_PIN, LOW);
    }
    Serial.print(F("\n操作模式切換至: ")); Serial.println(currentState);
  }

  // 模式操控（1: 安全；2: 就緒; 3: 自主; 4: 遙控; 5: 不亮燈）
  switch (currentState) {
    case 1: // 安全: 不動作
      setMotorSpeed(0, 0);
      break;
    case 2: // 就緒: 開啟感測燈，輸出在terminal
      setMotorSpeed(0, 0); 
      if (currentMillis - lastTrackPrint > 300) {
        Serial.printf("[感測器] %d %d %d %d %d\n", 
          !digitalRead(TRACK_1_PIN), !digitalRead(TRACK_2_PIN), !digitalRead(TRACK_3_PIN), !digitalRead(TRACK_4_PIN), !digitalRead(TRACK_5_PIN));
        lastTrackPrint = currentMillis;
      }
      break;
   case 3:
  {
    // 燈號閃爍
    if (currentMillis - lastLedToggleMillis >= 250) {
      ledState = !ledState;
      digitalWrite(RED_PIN, ledState);
      lastLedToggleMillis = currentMillis;
    }

    static unsigned long lostStartTime = 0;
    static bool isLost = false;

    int s1 = !digitalRead(TRACK_1_PIN);
    int s2 = !digitalRead(TRACK_2_PIN);
    int s3 = !digitalRead(TRACK_3_PIN);
    int s4 = !digitalRead(TRACK_4_PIN);
    int s5 = !digitalRead(TRACK_5_PIN);
    uint8_t sensorState = (s1 << 4) | (s2 << 3) | (s3 << 2) | (s4 << 1) | s5;

    long sum = 0;
    int active = 0;
    if (s1 == 1) { sum += -39; active++; }
    if (s2 == 1) { sum += -19; active++; }
    if (s3 == 1) { sum +=   0; active++; }
    if (s4 == 1) { sum +=  19; active++; }
    if (s5 == 1) { sum +=  39; active++; }

    // ★ 修正1: 全黑路口 - 保持方向小推力，並同步更新 lastError
    if (sensorState == 0b11111) {
      error = (lastError > 0) ? 8 : (lastError < 0) ? -8 : 0;
      isLost = false; // 全黑不算脫軌
    }
    // 有感測到線
    else if (active > 0) {
      error = sum / active;
      isLost = false; // 找回線，重設脫軌狀態
    }
    // 全白：脫軌
    else {
      if (!isLost) {
        isLost = true;
        lostStartTime = currentMillis;
      }
      error = (lastError < 0) ? -38 : (lastError > 0) ? 38 : 0;
    }

    // PID 計算
    int P = error;
    int D = error - lastError;
    int correction = (int)(Kp * P + Kd * D);
    lastError = error;

    int leftSpd  = TRACK_SPEEDL + correction;
    int rightSpd = TRACK_SPEEDR - correction;

    // 大角度時降速
    if (abs(error) >= 45) {
      leftSpd  -= 40;
      rightSpd -= 40;
    }

    // ★ 修正2: 脫軌計時覆蓋速度，在 setMotorSpeed 之前處理
    if (isLost) {
      unsigned long lostDuration = currentMillis - lostStartTime;
      if (lostDuration > 3000) {
        leftSpd  = 0;    // 超過3秒：全停
        rightSpd = 0;
      } else if (lostDuration > 1500) {
        leftSpd  = -140; // 1.5~3秒：倒車
        rightSpd = -140;
      }
      // 0~1.5秒：維持PID轉向繼續找線
    }

    // ★ 修正3: 只在這裡呼叫一次 setMotorSpeed
    setMotorSpeed(leftSpd, rightSpd);

    if (sensorState != lastSensorState) {
      Serial.print(F("感測: [ "));
      for (int i = 4; i >= 0; i--) Serial.print(bitRead(sensorState, i));
      Serial.printf(" ] | Error: %3d | L:%4d R:%4d | Lost:%d\n",
                    error, leftSpd, rightSpd, isLost ? (int)(currentMillis - lostStartTime) : 0);
      lastSensorState = sensorState;
    }
  }
  break;
    case 4: // 遙控 & 不亮
    case 5: 
      { 
        int x = RemoteXY.joystick_01_x;
        int y = RemoteXY.joystick_01_y;
        int rawLeft  = y + x; 
        int rawRight = y - x; 
        setMotorSpeed(convertToPWM(rawLeft), convertToPWM(rawRight));
      }
      break;
  }
} 
// 遙控模式PWM轉換
int convertToPWM(int rawSpeed) {
  if (abs(rawSpeed) < 10) return 0; 
  int pwm = constrain(map(abs(rawSpeed), 10, 200, 142, 170), 142, 170);
  return (rawSpeed > 0) ? pwm : -pwm; 
}
// 馬達速度設置
void setMotorSpeed(int leftSpeed, int rightSpeed) {
  // 限制範圍
  leftSpeed = constrain(leftSpeed, -250, 250);
  rightSpeed = constrain(rightSpeed, -250, 250);
  // 摩擦力補償
  /*
  int MIN_PWM_L = 110;  // 左輪起步動力
  int MIN_PWM_R = 104;  // 右輪起步動力
  if (leftSpeed > 0) {
    leftSpeed = map(leftSpeed, 1, 255, MIN_PWM_L, 255);
  } else if (leftSpeed < 0) {
    leftSpeed = -map(abs(leftSpeed), 1, 255, MIN_PWM_L, 255);
  }
  if (rightSpeed > 0) {
    rightSpeed = map(rightSpeed, 1, 255, MIN_PWM_R, 255);
  } else if (rightSpeed < 0) {
    rightSpeed = -map(abs(rightSpeed), 1, 255, MIN_PWM_R, 255);
  }/
  *
  // 馬達速度設置（左）
  if (leftSpeed == 0) {
    digitalWrite(IN1_PIN, LOW); 
    digitalWrite(IN2_PIN, LOW); 
    analogWrite(ENA, 0);
  } else if (leftSpeed > 0) {
    digitalWrite(IN1_PIN, HIGH); // 正轉
    digitalWrite(IN2_PIN, LOW); 
    analogWrite(ENA, leftSpeed);    
  } else {
    digitalWrite(IN1_PIN, LOW);  // 反轉
    digitalWrite(IN2_PIN, HIGH); 
    analogWrite(ENA, abs(leftSpeed)); 
  }
  // 馬達速度設置（右）
  if (rightSpeed == 0) {
    digitalWrite(IN3_PIN, LOW); 
    digitalWrite(IN4_PIN, LOW); 
    analogWrite(ENB, 0);
  } else if (rightSpeed > 0) {
    digitalWrite(IN3_PIN, HIGH); // 正轉
    digitalWrite(IN4_PIN, LOW); 
    analogWrite(ENB, rightSpeed);    
  } else {
    digitalWrite(IN3_PIN, LOW);  // 反轉
    digitalWrite(IN4_PIN, HIGH); 
    analogWrite(ENB, abs(rightSpeed)); 
  }
}