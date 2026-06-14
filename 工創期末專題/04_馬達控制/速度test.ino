// === 馬達 A (左輪) 腳位設定 ===
int ena = 13; // 接 L298N 的 ENA (負責馬達 A 速度)
int IN1 = 12; // 接 L298N 的 IN1 (負責馬達 A 方向)
int IN2 = 25; // 接 L298N 的 IN2 (負責馬達 A 方向)

// === 馬達 B (右輪) 腳位設定 === (先不接)
int enb = 14; // 接 L298N 的 ENB (負責馬達 B 速度)
int IN3 = 27; // 接 L298N 的 IN3 (負責馬達 B 方向)
int IN4 = 26; // 接 L298N 的 IN4 (負責馬達 B 方向)

void setup() {
  Serial.begin(115200);

  // 1. 設定方向控制腳位為 OUTPUT
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // 2. 將 ena 和 enb 都設定為 PWM 腳位 (500Hz, 8-bit 解析度)
  ledcAttach(ena, 500, 8); 
  ledcAttach(enb, 500, 8); 

  // 3. 確保一開始兩顆馬達都是停止的
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  ledcWrite(ena, 0);
  ledcWrite(enb, 0);

  Serial.println("=== 雙馬達 ENA  遙控測試已啟動 ===");
  Serial.println("請輸入轉速 (-255 到 255) 來同時控制兩顆馬達：");
}

void loop() {
  // 檢查是否有從序列埠監控視窗傳來的資料
  if (Serial.available() > 0) {
    
    int inputSpeed = Serial.parseInt(); // 讀取輸入的數字

    // 清空緩衝區，避免讀到 Enter 換行符號
    while(Serial.available() > 0) {
      Serial.read();
    }

    // 限制數值範圍
    inputSpeed = constrain(inputSpeed, -255, 255);

    Serial.print("➡️ 收到指令，雙輪轉速設定為: ");
    Serial.println(inputSpeed);

    if (inputSpeed > 0) {
      // --- 雙輪前進 ---
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      
      // 輸出速度
      ledcWrite(ena, inputSpeed);
      ledcWrite(enb, inputSpeed);
    } 
    else if (inputSpeed < 0) {
      // --- 雙輪後退 ---
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
      
      // 輸出速度 (使用 abs 轉成正數)
      ledcWrite(ena, abs(inputSpeed));
      ledcWrite(enb, abs(inputSpeed));
    } 
    else {
      // --- 雙輪停止 ---
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
      
      ledcWrite(ena, 0);
      ledcWrite(enb, 0);
    }
  }
}
// 轉速: 141~170 會變化 的比較明顯，超過 170 不會便更快