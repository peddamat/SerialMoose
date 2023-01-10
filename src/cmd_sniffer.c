#include "cmd_sniffer.h"

#include <stdio.h>
#include <string.h>

#include "argtable3/argtable3.h"
#include "driver/uart.h"
#include "esp_console.h"
#include "esp_log.h"
#include "esp_rom_gpio.h"
#include "esp_vfs_fat.h"
#include "hal/uart_ll.h"
#include "main.h"
#include "sdkconfig.h"
#include "soc/rtc.h"
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
// Command: Connect
/******************************************************************************/

static struct {
    struct arg_int *in;
    struct arg_int *out;
    struct arg_end *end;
} connect_args;

static int do_connect_cmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&connect_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, connect_args.end, argv[0]);
        return 0;
    }

    // // connect the default uart RX pin to the console uart TX pin signal
    // esp_rom_gpio_connect_out_signal(DEFAULT_UART_RX_PIN, UART_PERIPH_SIGNAL(1, SOC_UART_TX_PIN_IDX), false, false);
    // // connect the default uart TX pin to the console uart RX pin signal
    // esp_rom_gpio_connect_in_signal(DEFAULT_UART_TX_PIN, UART_PERIPH_SIGNAL(1, SOC_UART_RX_PIN_IDX), false);

    // // connect the default uart TX pin to the default uart TX pin signal
    // esp_rom_gpio_connect_out_signal(DEFAULT_UART_TX_PIN, UART_PERIPH_SIGNAL(0, SOC_UART_TX_PIN_IDX), false, false);
    // // connect the default uart RX pin to the default uart RX pin signal
    // esp_rom_gpio_connect_in_signal(DEFAULT_UART_RX_PIN, UART_PERIPH_SIGNAL(0, SOC_UART_RX_PIN_IDX), false);
    return 0;
}

static void register_connect_cmd(void) {
    connect_args.in = arg_int1("i", "in", "<pin>", "Specify in pin");
    connect_args.out = arg_int1("o", "out", "<pin>", "Specify out pin");
    connect_args.end = arg_end(3);
    const esp_console_cmd_t connect_cmd = {
        .command = "con",
        .help = "Connect generators to sniffers",
        .hint = NULL,
        .func = &do_connect_cmd,
        .argtable = &connect_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&connect_cmd));
}

/******************************************************************************/
// Command: Disconnect
/******************************************************************************/

static struct {
    struct arg_int *in;
    struct arg_int *out;
    struct arg_end *end;
} disconnect_args;

static int do_disconnect_cmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&disconnect_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, disconnect_args.end, argv[0]);
        return 0;
    }

    // // connect the console uart TX pin to the console uart TX pin signal
    // esp_rom_gpio_connect_out_signal(CONSOLE_UART_TX_PIN, UART_PERIPH_SIGNAL(1, SOC_UART_TX_PIN_IDX), false, false);
    // // connect the console uart RX pin to the console uart RX pin signal
    // esp_rom_gpio_connect_in_signal(CONSOLE_UART_RX_PIN, UART_PERIPH_SIGNAL(1, SOC_UART_RX_PIN_IDX), false);
    return 0;
}

static void register_disconnect_cmd(void) {
    disconnect_args.in = arg_int1("i", "in", "<pin>", "Specify in pin");
    disconnect_args.out = arg_int1("o", "out", "<pin>", "Specify out pin");
    disconnect_args.end = arg_end(3);
    const esp_console_cmd_t disconnect_cmd = {
        .command = "dis",
        .help = "Disconnect generators from sniffers",
        .hint = NULL,
        .func = &do_disconnect_cmd,
        .argtable = &disconnect_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&disconnect_cmd));
}

/******************************************************************************/
// Command: Generate Serial Data
/******************************************************************************/

static void probe1_txpin_task(void *arg) {
    esp_log_level_set("P1_TASK", ESP_LOG_INFO);
    while (1) {
        uart_write_bytes(PROBE1_UART_CHANNEL, "helloworld", strlen("helloworld"));
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void probe2_txpin_task(void *arg) {
    esp_log_level_set("P2_TASK", ESP_LOG_INFO);
    while (1) {
        uart_write_bytes(PROBE2_UART_CHANNEL, "yellowmellow", strlen("yellowmellow"));
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static struct {
    struct arg_int *pin;
    struct arg_int *baud;
    struct arg_end *end;
} gen_traffic_args;

static int do_generate_traffic_cmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&gen_traffic_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, gen_traffic_args.end, argv[0]);
        return 0;
    }

    int baud1 = UARTS_BAUD_RATE;
    int baud2 = UARTS_BAUD_RATE;
    if (gen_traffic_args.baud->count >= 1) {
        baud1 = gen_traffic_args.baud->ival[0];
    }
    if (gen_traffic_args.baud->count == 2) {
        baud2 = gen_traffic_args.baud->ival[1];
    }

    int len = gen_traffic_args.pin->count;

    // Configure Probe 1 TX pin and start task
    int pin1 = gen_traffic_args.pin->ival[0];
    configure_sniffer_line(PROBE1_UART_CHANNEL, pin1, UART_PIN_NO_CHANGE, baud1);
    xTaskCreate(probe1_txpin_task, "probe1_txpin_task", 1024 * 4, NULL, configMAX_PRIORITIES - 1, &Probe1TxTaskHandle);

    if (len == 2) {
        // Configure Probe 2 TX pin and start task
        int pin2 = gen_traffic_args.pin->ival[1];

        configure_sniffer_line(PROBE2_UART_CHANNEL, pin2, UART_PIN_NO_CHANGE, baud2);
        xTaskCreate(probe2_txpin_task, "probe2_txpin_task", 1024 * 4, NULL, configMAX_PRIORITIES - 1, &Probe2TxTaskHandle);
    }

    // connect_uarts();
    return 0;
}

static void register_gen_traffic_cmd(void) {
    gen_traffic_args.pin = arg_intn("p", "pin", "<pin>", 1, 2, "Specify the pin(s) to generate traffic on");
    gen_traffic_args.baud = arg_int0("b", "baud", "<baud rate>", "Specify the baud rate");
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

static uint32_t calculateApb(rtc_cpu_freq_config_t *conf) {
#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3
    return APB_CLK_FREQ;
#else
    if (conf->freq_mhz >= 80) {
        return 80 * MHZ;
    }
    return (conf->source_freq_mhz * MHZ) / conf->div;
#endif
}

uint32_t getApbFrequency() {
    rtc_cpu_freq_config_t conf;
    rtc_clk_cpu_freq_get_config(&conf);
    return calculateApb(&conf);
}

static unsigned long find_nearest_baud_rate(unsigned long baudrate) {
    static const unsigned long default_rates[] = {300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 74880, 115200, 230400, 256000, 460800, 921600, 1843200, 3686400};

    size_t i;
    for (i = 1; i < sizeof(default_rates) / sizeof(default_rates[0]) - 1; i++)  // find the nearest real baudrate
    {
        if (baudrate <= default_rates[i]) {
            if (baudrate - default_rates[i - 1] < default_rates[i] - baudrate) {
                i--;
            }
            break;
        }
    }

    return default_rates[i];
}

// Borrowed from Arduino_ESP32
static unsigned long detect_baud_rate(int channel, bool raw, int timeout) {
    TickType_t xTicksToWait = timeout;
    TimeOut_t xTimeOut;
    vTaskSetTimeOutState(&xTimeOut);

    uart_dev_t *hw = UART_LL_GET_HW(channel);
    hw->auto_baud.glitch_filt = 0x08;
    hw->auto_baud.en = 0;
    hw->auto_baud.en = 1;

#ifndef CONFIG_IDF_TARGET_ESP32S3
    while (hw->rxd_cnt.edge_cnt < 30) {  // UART_PULSE_NUM(uart_num)
        if (xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) != pdFALSE) {
            return 0;
        }
        // ets_delay_us(1000);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }

    // TODO: Fix these mutexes
    // UART_MUTEX_LOCK();
    // log_i("lowpulse_min_cnt = %d hightpulse_min_cnt = %d", hw->lowpulse.min_cnt, hw->highpulse.min_cnt);
    unsigned long divisor = ((hw->lowpulse.min_cnt + hw->highpulse.min_cnt) >> 1);
    // UART_MUTEX_UNLOCK();

    unsigned long baudrate = getApbFrequency() / divisor;

    if (raw) {
        return baudrate;
    } else {
        return find_nearest_baud_rate(baudrate);
    }
#else
    return 0;
#endif
}

static struct {
    struct arg_int *pin;
    struct arg_lit *raw;
    struct arg_end *end;
} det_baud_args;

static int do_determine_baud_cmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&det_baud_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, det_baud_args.end, argv[0]);
        return 0;
    }

    int pin = det_baud_args.pin->ival[0];
    if (!uart_is_driver_installed(PROBE1_UART_CHANNEL)) {
        configure_sniffer_line(PROBE1_UART_CHANNEL, UART_PIN_NO_CHANGE, pin, UARTS_BAUD_RATE);
    } else {
        ESP_ERROR_CHECK(uart_set_pin(PROBE1_UART_CHANNEL, UART_PIN_NO_CHANGE, pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    }

    // Default to not showing raw baud rate
    bool raw = det_baud_args.raw->count;

    unsigned long baud_rate = detect_baud_rate(PROBE1_UART_CHANNEL, raw, 2000 / portTICK_PERIOD_MS);
    if (baud_rate == 0) {
        printf("Timed out while detecting baud rate on pin %i\n", pin);
    } else {
        printf("Detected baud rate on pin %i: %lu\n", pin, baud_rate);
    }

    return 0;
}

static void register_determine_baud(void) {
    det_baud_args.pin = arg_int1("p", "pin", "<pin>", "Specify the pin to detect baud on");
    det_baud_args.raw = arg_lit0("r", "raw", "Output raw baud rate");
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

    if (Probe1RxTaskHandle != NULL) {
        vTaskDelete(Probe1RxTaskHandle);
        Probe1RxTaskHandle = NULL;
        printf("Stopped Probe 1 Generator\n");
    }
    if (Probe2RxTaskHandle != NULL) {
        vTaskDelete(Probe2RxTaskHandle);
        Probe2RxTaskHandle = NULL;
        printf("Stopped Probe 2 Generator\n");
    }
    if (Probe1TxTaskHandle != NULL) {
        vTaskDelete(Probe1TxTaskHandle);
        Probe1TxTaskHandle = NULL;
        printf("Stopped Probe 1 Sniffer\n");
    }
    if (Probe2TxTaskHandle != NULL) {
        vTaskDelete(Probe2TxTaskHandle);
        Probe2TxTaskHandle = NULL;
        printf("Stopped Probe 2 Sniffer\n");
    }

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

    if (uart_is_driver_installed(PROBE1_UART_CHANNEL)) {
        xTaskCreate(probe1_rxpin_task, "probe1_rx_task", 1024 * 4, NULL, configMAX_PRIORITIES - 1, &Probe1RxTaskHandle);
        printf("Started probe 1 sniffer!\n");
    }
    if (uart_is_driver_installed(PROBE2_UART_CHANNEL)) {
        xTaskCreate(probe2_rxpin_task, "probe2_rx_task", 1024 * 4, NULL, configMAX_PRIORITIES - 1, &Probe2RxTaskHandle);
        printf("Started probe 2 sniffer!\n");
    }

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

static void configure_sniffer_line(int channel, int tx_pin, int rx_pin, int baud_rate) {
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    if (!uart_is_driver_installed(channel)) {
        ESP_ERROR_CHECK(uart_driver_install(channel, READ_BUF_SIZE * 2, 0, 0, NULL, 0));
    }
    ESP_ERROR_CHECK(uart_param_config(channel, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(channel, tx_pin, rx_pin,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    if (tx_pin != -1) {
        printf("Configured probe %i generator on pin %i @ %i baud\n", channel, tx_pin, baud_rate);
    }
    if (rx_pin != -1) {
        printf("Configured probe %i sniffer on pin %i @ %i baud\n", channel, rx_pin, baud_rate);
    }
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
    int baud = setup_args.baud->ival[0];

    configure_sniffer_line(PROBE1_UART_CHANNEL, UART_PIN_NO_CHANGE, p1_pin, baud);

    if (setup_args.p2_pin->count == 1) {
        int p2_pin = setup_args.p2_pin->ival[0];

        // If only one baud rate was provided, reuse it
        if (setup_args.baud->count == 1) {
            configure_sniffer_line(PROBE2_UART_CHANNEL, UART_PIN_NO_CHANGE, p2_pin, baud);
        } else {
            int baud2 = setup_args.baud->ival[1];
            configure_sniffer_line(PROBE2_UART_CHANNEL, UART_PIN_NO_CHANGE, p2_pin, baud2);
        }
    }

    return 0;
}

static void register_setup(void) {
    setup_args.p1_pin = arg_int1("1", "probe1", "<p1 pin>", "Specify the Probe 1 pin");
    setup_args.p2_pin = arg_int1("2", "probe2", "<p2 pin>", "Specify the Probe 2 pin");
    setup_args.baud = arg_intn("b", "baud", "<baud rate>", 1, 2, "Specify the baud rate");
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
    register_connect_cmd();
    register_disconnect_cmd();
    // TX passthrough
    register_determine_baud();
}
