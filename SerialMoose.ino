#include "Bluetooth.h"

// Define sniffer lines
HardwareSerial &SniffRX = Serial1;
HardwareSerial &SniffTX = Serial2;

// Define debug console output
#ifdef DEBUG_OVER_BLUETOOTH
BluetoothSerial DebugConsole;
#elif
HardwareSerial &DebugConsole = Serial;
#endif

  
#ifdef DEBUG_OVER_BLUETOOTH
void setup_debug_console() {
  DebugConsole.enableSSP();
  DebugConsole.onConfirmRequest(BTConfirmRequestCallback);
  DebugConsole.onAuthComplete(BTAuthCompleteCallback);
  DebugConsole.begin("SerialMoose"); 

  // TODO: Remove this after debugging...
  Serial.begin(9600);
}
#elif
void setup_debug_console() {
  DebugConsole.begin(115200);
}
#endif

void setup() {

  setup_debug_console();

  SniffRX.begin(9600, SERIAL_8N1, 34, 32, false, 11000UL);

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

int counter = 0;

void loop() {

  // TODO: Automatically confirming pairing request for now.
  //       Replace with code to read "BOOT" pin button press.
  if (confirmRequestPending)
  {
      DebugConsole.confirmReply(true);
      DebugConsole.println("Connected");
      confirmRequestPending = false;
  }

  if (Serial.available())
  {
    DebugConsole.write(Serial.read());
  }
  if (DebugConsole.available())
  {
    Serial.write(DebugConsole.read());
  }

  if (SniffRX.peek() != -1) {      
    // DebugConsole.write("-> ");
    DebugConsole.printf("%09d -> ", counter);
    while (SniffRX.available()) {      
      // DebugConsole.print(SniffRX.read(), HEX);
      DebugConsole.printf("%02x ", SniffRX.read());
    }
    DebugConsole.write('\n');
  }

  if (SniffTX.peek() != -1) {    
    // DebugConsole.write("<- ");
    DebugConsole.printf("%09d <- ", counter);
    while (SniffTX.available()) {      
      // DebugConsole.print(SniffTX.read(), HEX);
      DebugConsole.printf("%02x ", SniffTX.read());
    }
    DebugConsole.write('\n');
  }

  delay(100);
  counter++;
}
