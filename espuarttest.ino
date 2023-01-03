void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial2.begin(0, SERIAL_8N1, 34, 33, false, 11000UL);
  Serial.println("Wooo!");
  unsigned long detectedBaudRate = Serial2.baudRate();
  if (detectedBaudRate) {
    Serial.printf("Detected baudrate is %lu\n", detectedBaudRate);
  } else {
    Serial.println("No baudrate detected, Serial1 will not work!");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Sent command.");
  Serial2.println("Hi!");
  delay(5000);
}
