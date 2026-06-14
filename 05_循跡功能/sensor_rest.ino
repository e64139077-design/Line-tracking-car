//循跡感測器測試
void setup() {
  Serial.begin(115200); 
  
  for (int i = 0; i < 5; i++) {
    pinMode(sensorPins[i], INPUT);
  }
  Serial.println("\n===== 5路 TCRT5000 感測器系統已啟動  =====");
}

void loop() {
  int sensorValues[5];
  
  // 讀取 5 個感測器的數位數值
  for (int i = 0; i < 5; i++) {
    sensorValues[i] = digitalRead(sensorPins[i]);
  }

  // 打印感測器狀態（0=黑線/沒反射，1=白線/有反射）
  Serial.print("📊 感測器狀態 [S1~S5]: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(sensorValues[i]);
    Serial.print(" ");
  }
  Serial.println();
  
  delay(500); // 每 0.5 秒刷新一次資料
}
