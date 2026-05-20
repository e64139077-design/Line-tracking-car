//按一下開燈，按一下關燈(使用 delay 處理彈跳訊號)
#define LED_PIN 5       // LED 連接到 GPIO5
#define BUTTON_PIN 15   // 按鈕 連接到 GPIO15（內建 PULLUP）

bool ledState = LOW;   // LED 初始狀態

void setup() {
  pinMode(LED_PIN, OUTPUT);           // 設定 LED 為輸出
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // 使用內建 PULLUP，按下時變 LOW
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {  // 按鈕按下
    delay(50);  // 消抖
    if (digitalRead(BUTTON_PIN) == LOW) {  // 確保按鈕仍然按下
      ledState = !ledState;   // 切換 LED 狀態
      digitalWrite(LED_PIN, ledState);  // 更新 LED
      while (digitalRead(BUTTON_PIN) == LOW);  // 等待按鈕放開
      delay(50);  // 再次消抖
    }
  }
}
//我決定用 millis() 來處理按鈕的彈跳訊號，
//這樣可以避免使用 delay() 函數，讓程式更流暢。以下是修改後的程式碼：

