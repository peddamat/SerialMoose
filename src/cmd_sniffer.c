#include "cmd_sniffer.h"

#include <stdio.h>
#include <string.h>

#include "argtable3/argtable3.h"
#include "driver/uart.h"
#include "esp_console.h"
#include "esp_log.h"
#include "esp_rom_gpio.h"
#include "esp_vfs_fat.h"
#include "main.h"
#include "sdkconfig.h"
#include "soc/uart_periph.h"

static const char *TAG = "cmd_sniffer";
static void configure_sniffer_line(int channel, int rx_pin, int tx_pin, int baud_rate);

// static esp_err_t i2c_get_port(int port, i2c_port_t *i2c_port) {
//     if (port >= I2C_NUM_MAX) {
//         ESP_LOGE(TAG, "Wrong port number: %d", port);
//         return ESP_FAIL;
//     }
//     *i2c_port = port;
//     return ESP_OK;
// }

#define READ_BUF_SIZE (1024)

TaskHandle_t Probe1RxTaskHandle;
TaskHandle_t Probe2RxTaskHandle;
TaskHandle_t Probe1TxTaskHandle;
TaskHandle_t Probe2TxTaskHandle;

/******************************************************************************/
// UART: Sniffer Async Tasks
/******************************************************************************/

static void probe1_rxpin_task(void *arg) {
    static const char *RX_TASK_TAG = "RX";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(READ_BUF_SIZE + 1);
    while (1) {
        const int rxBytes = uart_read_bytes(PROBE1_UART_CHANNEL, data, READ_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_ERROR);
        }
    }
    free(data);
}

static void probe2_rxpin_task(void *arg) {
    static const char *TX_TASK_TAG = "TX";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(READ_BUF_SIZE + 1);
    while (1) {
        const int txBytes = uart_read_bytes(PROBE2_UART_CHANNEL, data, READ_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (txBytes > 0) {
            data[txBytes] = 0;
            ESP_LOG_BUFFER_HEXDUMP(TX_TASK_TAG, data, txBytes, ESP_LOG_ERROR);
        }
    }
    free(data);
}

/******************************************************************************/
// Command: TX Passthrough
/******************************************************************************/

/******************************************************************************/
// Command: Generate Serial Data
/******************************************************************************/

// static void connect_uarts_old(void) {
//     // connect the default uart RX pin to the console uart TX pin signal
//     esp_rom_gpio_connect_out_signal(DEFAULT_UART_RX_PIN, UART_PERIPH_SIGNAL(1, SOC_UART_TX_PIN_IDX), false, false);
//     // connect the default uart TX pin to the console uart RX pin signal
//     esp_rom_gpio_connect_in_signal(DEFAULT_UART_TX_PIN, UART_PERIPH_SIGNAL(1, SOC_UART_RX_PIN_IDX), false);

//     // connect the default uart TX pin to the default uart TX pin signal
//     esp_rom_gpio_connect_out_signal(DEFAULT_UART_TX_PIN, UART_PERIPH_SIGNAL(0, SOC_UART_TX_PIN_IDX), false, false);
//     // connect the default uart RX pin to the default uart RX pin signal
//     esp_rom_gpio_connect_in_signal(DEFAULT_UART_RX_PIN, UART_PERIPH_SIGNAL(0, SOC_UART_RX_PIN_IDX), false);
// }

// static void disconnect_uarts(void) {
//     // connect the console uart TX pin to the console uart TX pin signal
//     esp_rom_gpio_connect_out_signal(CONSOLE_UART_TX_PIN, UART_PERIPH_SIGNAL(1, SOC_UART_TX_PIN_IDX), false, false);
//     // connect the console uart RX pin to the console uart RX pin signal
//     esp_rom_gpio_connect_in_signal(CONSOLE_UART_RX_PIN, UART_PERIPH_SIGNAL(1, SOC_UART_RX_PIN_IDX), false);
// }

static void probe1_txpin_task(void *arg) {
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        uart_write_bytes(PROBE1_UART_CHANNEL, "helloworld", strlen("helloworld"));
        // sendData(TX_TASK_TAG, "Hello world");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static struct {
    struct arg_int *pin;
    struct arg_end *end;
} gen_traffic_args;

static int do_generate_traffic_cmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&gen_traffic_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, gen_traffic_args.end, argv[0]);
        return 0;
    }

    int len = gen_traffic_args.pin->count;

    if (len == 1) {
        int pin = gen_traffic_args.pin->ival[0];

        // Configure Probe 1 TX pin
        // configure_sniffer_line(PROBE1_UART_CHANNEL, PROBE1_UART_RX_PIN, pin, UARTS_BAUD_RATE);
        ESP_ERROR_CHECK(uart_set_pin(PROBE1_UART_CHANNEL, pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

        // Start Probe 1 TX task
        xTaskCreate(probe1_txpin_task, "probe1_txpin_task", 1024 * 4, NULL, configMAX_PRIORITIES - 1, &Probe1TxTaskHandle);
    }
    // connect_uarts();
    return 0;
}

static void register_gen_traffic_cmd(void) {
    gen_traffic_args.pin = arg_intn("p", "pin", "<pin>", 1, 2, "Specify the pin(s) to generate traffic on");
    gen_traffic_args.end = arg_end(3);
    const esp_console_cmd_t gen_traffic_cmd = {
        .command = "gen",
        .help = "Generate fake traffic",
        .hint = NULL,
        .func = &do_generate_traffic_cmd,
        .argtable = &gen_traffic_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&gen_traffic_cmd));
}

/******************************************************************************/
// Command: Determine Baud Rate
/******************************************************************************/

static struct {
    struct arg_int *pin;
    struct arg_end *end;
} det_baud_args;

static int do_determine_baud_cmd(int argc, char **argv) {
    return 0;
}

static void register_determine_baud(void) {
    det_baud_args.pin = arg_int1("p", "pin", "<pin>", "Specify the pin to detect baud on");
    det_baud_args.end = arg_end(3);
    const esp_console_cmd_t det_baud_cmd = {
        .command = "det",
        .help = "Detect baud rate",
        .hint = NULL,
        .func = &do_determine_baud_cmd,
        .argtable = &det_baud_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&det_baud_cmd));
}

/******************************************************************************/
// Command: Stop Sniffing
/******************************************************************************/

static struct {
    struct arg_end *end;
} stop_args;

static int do_stop_cmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&stop_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, stop_args.end, argv[0]);
        return 0;
    }

    // TODO: Ensure sniffer has been started...

    printf("Stopping sniff!\n\n");
    vTaskDelete(Probe1RxTaskHandle);
    vTaskDelete(Probe2RxTaskHandle);
    vTaskDelete(Probe1TxTaskHandle);

    return 0;
}

static void register_stop(void) {
    stop_args.end = arg_end(3);
    const esp_console_cmd_t stop_cmd = {
        .command = "stop",
        .help = "Stop sniffer lines",
        .hint = NULL,
        .func = &do_stop_cmd,
        .argtable = &stop_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&stop_cmd));
}

/******************************************************************************/
// Command: Start Sniffing
/******************************************************************************/

static struct {
    struct arg_end *end;
} start_args;

static int do_start_cmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&start_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, start_args.end, argv[0]);
        return 0;
    }

    // TODO: Ensure sniffer lines are configured...

    printf("Starting sniff!\n\n");
    xTaskCreate(probe1_rxpin_task, "uart_rx_task", 1024 * 4, NULL, configMAX_PRIORITIES - 1, &Probe1RxTaskHandle);
    xTaskCreate(probe2_rxpin_task, "uart_tx_task", 1024 * 4, NULL, configMAX_PRIORITIES - 1, &Probe2RxTaskHandle);

    return 0;
}

static void register_start(void) {
    start_args.end = arg_end(1);
    const esp_console_cmd_t start_cmd = {
        .command = "start",
        .help = "Start sniffer lines",
        .hint = NULL,
        .func = &do_start_cmd,
        .argtable = &start_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&start_cmd));
}

/******************************************************************************/
// UART: Setup
/******************************************************************************/

static void configure_sniffer_line(int channel, int rx_pin, int tx_pin, int baud_rate) {
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    ESP_ERROR_CHECK(uart_driver_install(channel, READ_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(channel, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(channel, tx_pin, rx_pin,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    printf("Line configured on pin %i @ %i\n", rx_pin, baud_rate);
}

/******************************************************************************/
// Command: Setup Sniffer Lines
/******************************************************************************/

static struct {
    struct arg_int *p1_pin;
    struct arg_int *p2_pin;
    struct arg_int *baud;
    struct arg_end *end;
} setup_args;

static int do_setup_cmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&setup_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, setup_args.end, argv[0]);
        return 0;
    }

    int p1_pin = setup_args.p1_pin->ival[0];
    int p2_pin = setup_args.p2_pin->ival[0];
    int baud = setup_args.baud->ival[0];

    configure_sniffer_line(PROBE1_UART_CHANNEL, p1_pin, UART_PIN_NO_CHANGE, baud);
    configure_sniffer_line(PROBE2_UART_CHANNEL, p2_pin, UART_PIN_NO_CHANGE, baud);

    return 0;
}

static void register_setup(void) {
    setup_args.p1_pin = arg_int1("r", "p1", "<p1 pin>", "Specify the Probe 1 pin");
    setup_args.p2_pin = arg_int1("t", "p2", "<p2 pin>", "Specify the Probe 2 pin");
    setup_args.baud = arg_int1("b", "baud", "<baud rate>", "Specify the baud rate");
    setup_args.end = arg_end(3);
    const esp_console_cmd_t setup_cmd = {
        .command = "setup",
        .help = "Setup sniffer lines",
        .hint = NULL,
        .func = &do_setup_cmd,
        .argtable = &setup_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&setup_cmd));
}

/******************************************************************************/
// Register Commands
/******************************************************************************/

void register_sniffertools(void) {
    register_setup();
    register_start();
    register_stop();
    // Generate serial data
    register_gen_traffic_cmd();
    // TX passthrough
    register_determine_baud();
}
