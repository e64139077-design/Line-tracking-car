//透過序列控制 LED 發光
#define LED_BUILTIN 5 //腳位
char val;

void setup(){
  pinMode(LED_BUILTIN, OUTPUT); //設定腳位為輸出
  Serial.begin(115200); //注意要相同的傳輸速率
  Serial.print("Welcome to Arduino!");
}

void loop(){
  if (Serial.available()){
    val = Serial.read();
    if (val == '1'){
      digitalWrite(LED_BUILTIN, HIGH); //LED 發光
      Serial.println("LED ON");
    }
    else if (val == '0'){
      digitalWrite(LED_BUILTIN, LOW); //LED 熄滅
      Serial.println("LED OFF");
    }
  }
}