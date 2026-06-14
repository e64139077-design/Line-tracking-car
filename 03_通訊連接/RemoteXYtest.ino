void setup()
{
  RemoteXY_Init (); // initialization by macros
  pinMode(LED_PIN, OUTPUT);
  
  // TODO your setup code
}

void loop()
{
  RemoteXYEngine.handler ();
  
  if (RemoteXY.switch_01 == 1) {
    digitalWrite(LED_PIN, HIGH);
  }
  else if (RemoteXY.switch_01 == 0) {
    digitalWrite(LED_PIN, LOW);
  }
}