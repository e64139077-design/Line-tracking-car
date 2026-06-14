// ASL 自主狀態指示燈 第一版
// 這個程式會根據電腦指令改變燈號 
#define RED_PIN   12
#define GREEN_PIN 13
#define BLUE_PIN  14

int currentState = 1; 
int lastState = -1;    // 🆕 用來記錄「上一次」的狀態，預設設為 -1

// ⏳ 計時器 A：用於狀態 3 紅燈閃爍
unsigned long previousMillis = 0;
bool redBlinkState = LOW;
const long interval = 250; 

void setup() {
  Serial.begin(115200);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  Serial.println("\n===== 狀態系統（改變時才輸出版） =====");
}

void loop() {
  // 1. 監聽電腦指令
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == '1')      { currentState = 1; } 
    else if (cmd == '2') { currentState = 2; } 
    else if (cmd == '3') { currentState = 3; } 
    else if (cmd == '4') { currentState = 4; }
    else if (cmd == '5') { currentState = 5; }
  }

  // 2. 燈號控制邏輯（與Lab相同）
  unsigned long currentMillis = millis();
  switch (currentState) {
    case 1: digitalWrite(GREEN_PIN, HIGH); digitalWrite(RED_PIN, LOW);  digitalWrite(BLUE_PIN, LOW);  break;
    case 2: digitalWrite(GREEN_PIN, LOW);  digitalWrite(RED_PIN, HIGH); digitalWrite(BLUE_PIN, LOW);  break;
    case 3: 
      digitalWrite(GREEN_PIN, LOW); digitalWrite(BLUE_PIN, LOW);
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        redBlinkState = !redBlinkState;
        digitalWrite(RED_PIN, redBlinkState);
      }
      break;
    case 4: digitalWrite(GREEN_PIN, LOW);  digitalWrite(RED_PIN, LOW);  digitalWrite(BLUE_PIN, HIGH); break;
    case 5: digitalWrite(GREEN_PIN, LOW);  digitalWrite(RED_PIN, LOW);  digitalWrite(BLUE_PIN, LOW);  break;
  }

  // 3. 只有在 currentState 改變時，才印出一次紀錄
  if (currentState != lastState) {
    lastState = currentState; // 立刻更新歷史紀錄，防止重複列印
    
    Serial.print("📊 [狀態更新] 目前車輛模式: ");
    Serial.print(currentState);
    switch (currentState) {
      case 1: Serial.println(" -> 安全狀態 (綠燈)"); break;
      case 2: Serial.println(" -> 就緒狀態 (紅燈)"); break;
      case 3: Serial.println(" -> 自主導航 (紅燈閃爍中...)"); break;
      case 4: Serial.println(" -> 其他測試 (藍燈)"); break;
      case 5: Serial.println(" -> 全滅狀態"); break;
    }
  }
}
