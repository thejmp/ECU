void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(22, OUTPUT);
  digitalWrite(22, HIGH);
  delay(1000);
  digitalWrite(22, LOW);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
}
