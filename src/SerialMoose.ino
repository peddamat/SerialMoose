#include "Bluetooth.h"
#include "SimpleSerialShell.h"
#include "driver/uart.h"

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
    char *temp = loc_buf;
    int len = vsnprintf(temp, sizeof(loc_buf), format, ap);

    if (len < 0) {
        return 0;
    };
    if (len >= (int)sizeof(loc_buf)) {
        temp = (char *)malloc(len + 1);
        if (temp == NULL) {
            return 0;
        }
        len = vsnprintf(temp, len + 1, format, ap);
    }
    len = DebugConsole.write((uint8_t *)temp, len);
    if (temp != loc_buf) {
        free(temp);
    }
    return len;
}

int badArgCount(char *cmdName) {
    shell.print(cmdName);
    shell.println(F(": bad arg count"));
    return -1;
}

/*
 * setup_sniffer(rx_pin, tx_pin, baud_rate)
 */
int setup_sniffer(int argc, char **argv) {
    if (argc == 4) {
        // Grab baud rate
        int baud = atoi(argv[3]);

        // Setup RX pin
        int pin = atoi(argv[1]);
        if (pin < 0 || pin > (int)NUM_DIGITAL_PINS) {
            shell.print(F("Invalid RX pin: "));
            shell.print(pin);
            return -1;
        }
        SniffRX.begin(baud, SERIAL_8N1, pin, 33, false, 11000UL);
        DebugConsole.println("Initialized RX Line.");

        xTaskCreatePinnedToCore(rx_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES - 1, NULL, 0);

        // Setup TX pin
        pin = atoi(argv[2]);
        if (pin < 0 || pin > (int)NUM_DIGITAL_PINS) {
            shell.print(F("Invalid TX pin: "));
            shell.print(pin);
            return -1;
        }
        SniffTX.begin(baud, SERIAL_8N1, pin, 32, false, 11000UL);
        DebugConsole.println("Initialized TX Line.");

        xTaskCreatePinnedToCore(tx_task, "uart_tx_task", 1024 * 2, NULL, configMAX_PRIORITIES - 1, NULL, 0);

        return EXIT_SUCCESS;
    }

    return badArgCount(argv[0]);
}

/*
 * detect_baud(on_pin)
 */
int detect_baud(int argc, char **argv) {
    if (argc == 2) {
        int pin = atoi(argv[1]);

        DebugConsole.printf("Detecting baud on: %i\n" + pin);
        SniffRX.begin(0, SERIAL_8N1, pin, 33, false, 11000UL);

        unsigned long detectedBaudRate = SniffRX.baudRate();
        if (detectedBaudRate) {
            DebugConsole.printf("- Detected baudrate is %lu\n", detectedBaudRate);
        } else {
            DebugConsole.println("- No baudrate detected, Serial1 will not work!");
        }

        return EXIT_SUCCESS;
    }

    return badArgCount(argv[0]);
}

void setup() {
    setup_debug_console();

    shell.attach(DebugConsole);
    // shell.addCommand(F("echo"), echo);
    shell.addCommand(F("setup rx_pin tx_pin baud_rate"), setup_sniffer);
    shell.addCommand(F("detect_baud pin"), detect_baud);

    // Configure ESP-IDF logger
    esp_log_set_vprintf(debug_vprintf);

    // TODO: Check what the stack depth actually needs to be...
    xTaskCreatePinnedToCore(confirm_pin, "confirm_pin", 1024 * 2, NULL, configMAX_PRIORITIES - 1, NULL, 0);
}

void loop() {
    shell.executeIfInput();
}

static void rx_task(void *arg) {
    static const char *RX_TASK_TAG = "RX";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);

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
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);

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
        if (confirmRequestPending) {
            DebugConsole.confirmReply(true);
            DebugConsole.println("Connected");
            confirmRequestPending = false;
        }
        vTaskDelay(10);
    }
}
