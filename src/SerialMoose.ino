#include "driver/uart.h"
#include "Bluetooth.h"
#include "SimpleSerialShell.h"

static const int RX_BUF_SIZE = 1024;

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

// Modified from esp32/Print.cpp.  
static int debug_vprintf(const char *format, va_list ap) {

    char loc_buf[64];
    char * temp = loc_buf;
    int len = vsnprintf(temp, sizeof(loc_buf), format, ap);

    if(len < 0) {
        return 0;
    };
    if(len >= (int)sizeof(loc_buf)){  
        temp = (char*) malloc(len+1);
        if(temp == NULL) {
            return 0;
        }
        len = vsnprintf(temp, len+1, format, ap);
    }
    len = DebugConsole.write((uint8_t*)temp, len);
    if(temp != loc_buf){
        free(temp);
    }
    return len;
}

static void rx_task(void *arg) {

  static const char *RX_TASK_TAG = "RX";
  esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
  uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
  while (1) {
      const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
      if (rxBytes > 0) {
          data[rxBytes] = 0;
          ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_ERROR);
      }
  }
  free(data);
}

static void tx_task(void *arg) {

  static const char *TX_TASK_TAG = "TX";
  esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
  uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
  while (1) {
      const int txBytes = uart_read_bytes(UART_NUM_2, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
      if (txBytes > 0) {
          data[txBytes] = 0;
          ESP_LOG_BUFFER_HEXDUMP(TX_TASK_TAG, data, txBytes, ESP_LOG_ERROR);
      }
  }
  free(data);
}


// TODO: Automatically confirming pairing request for now.
//       Replace with code to read "BOOT" pin button press.
static void confirm_pin(void *arg) {

  while (1) {
    if (confirmRequestPending)
    {
        DebugConsole.confirmReply(true);
        DebugConsole.println("Connected");
        confirmRequestPending = false;
    }
    vTaskDelay(10);
  }
}

int echo(int argc, char **argv)
{
    auto lastArg = argc - 1;
    for ( int i = 1; i < argc; i++) {

        shell.print(argv[i]);

        if (i < lastArg)
            shell.print(F(" "));
    }
    shell.println();

    return EXIT_SUCCESS;
}


void setup() {

  pinMode(0, INPUT);

  setup_debug_console();

  shell.attach(DebugConsole);
  shell.addCommand(F("echo"), echo);

  // Configure ESP-IDF logger 
  esp_log_set_vprintf(debug_vprintf);

  SniffRX.begin(9600, SERIAL_8N1, 35, 33, false, 11000UL);

  DebugConsole.println("Initialized RX Line.");

  unsigned long detectedBaudRate = SniffRX.baudRate();
  if (detectedBaudRate) {
    DebugConsole.printf("Detected baudrate is %lu\n", detectedBaudRate);
  } else {
    DebugConsole.println("No baudrate detected, Serial1 will not work!");
  }

  DebugConsole.println("Initialized TX Line.");
  SniffTX.begin(detectedBaudRate, SERIAL_8N1, 34, 32, false, 11000UL);

  // Arduino runs on core 1 by default
  xTaskCreatePinnedToCore(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL, 0);
  xTaskCreatePinnedToCore(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL, 0);
  // TODO: Check what the stack depth actually needs to be...
  xTaskCreatePinnedToCore(confirm_pin, "confirm_pin", 1024*2, NULL, configMAX_PRIORITIES-1, NULL, 0);
}

void loop() {
  shell.executeIfInput();

}
