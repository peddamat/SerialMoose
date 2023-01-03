
HardwareSerial &DebugConsole = Serial;
HardwareSerial &SniffRX = Serial1;
HardwareSerial &SniffTX = Serial2;

void setup() {

  DebugConsole.begin(115200);

  SniffRX.begin(0, SERIAL_8N1, 34, 32, false, 11000UL);

  DebugConsole.println("Initialized RX Line.");

  unsigned long detectedBaudRate = SniffRX.baudRate();
  if (detectedBaudRate) {
    DebugConsole.printf("Detected baudrate is %lu\n", detectedBaudRate);
  } else {
    DebugConsole.println("No baudrate detected, Serial1 will not work!");
  }

  DebugConsole.println("Initialized TX Line.");
  SniffTX.begin(detectedBaudRate, SERIAL_8N1, 35, 33, false, 11000UL);
}

void loop() {
  if (SniffRX.peek() != -1) {      
    DebugConsole.write("-> ");
    while (SniffRX.available()) {      
      DebugConsole.write(SniffRX.read());
    }
    DebugConsole.write("\n");
  }

  if (SniffTX.peek() != -1) {    
    DebugConsole.write("<- ");
    while (SniffTX.available()) {      
      DebugConsole.write(SniffTX.read());   
    }
    DebugConsole.write("\n");
  }

  delay(1000);
}
