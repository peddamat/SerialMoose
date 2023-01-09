#include <stdio.h>

#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_log.h"

static const char *TAG = "cmd_sniffer";

// static esp_err_t i2c_get_port(int port, i2c_port_t *i2c_port) {
//     if (port >= I2C_NUM_MAX) {
//         ESP_LOGE(TAG, "Wrong port number: %d", port);
//         return ESP_FAIL;
//     }
//     *i2c_port = port;
//     return ESP_OK;
// }

/******************************************************************************/
/* Command: TX Passthrough                                                    */
/******************************************************************************/

/******************************************************************************/
/* Command: Generate Serial Data                                              */
/******************************************************************************/

/******************************************************************************/
/* Command: Determine Baud Rate                                               */
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
/* Command: Setup                                                             */
/******************************************************************************/

static struct {
    struct arg_int *rx_pin;
    struct arg_int *tx_pin;
    struct arg_int *baud;
    struct arg_end *end;
} setup_args;

static int do_setup_cmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&setup_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, setup_args.end, argv[0]);
        return 0;
    }

    /* Check chip address: "-c" option */
    // int chip_addr = i2cset_args.chip_address->ival[0];
    /* Check register address: "-r" option */
    // int data_addr = 0;

    return 0;
}

static void register_setup(void) {
    setup_args.rx_pin = arg_int1("r", "rx", "<rx pin>", "Specify the 'RX' pin");
    setup_args.tx_pin = arg_int1("t", "rx", "<tx pin>", "Specify the 'TX' pin");
    setup_args.baud = arg_int1("b", "baud", "<baud rate>", "Specify the baud rate");
    setup_args.end = arg_end(3);
    const esp_console_cmd_t setup_cmd = {
        .command = "set",
        .help = "Setup sniffer lines",
        .hint = NULL,
        .func = &do_setup_cmd,
        .argtable = &setup_args};
    ESP_ERROR_CHECK(esp_console_cmd_register(&setup_cmd));
}

/******************************************************************************/
/* Register Commands                                                          */
/******************************************************************************/

void register_sniffertools(void) {
    register_setup();
    register_determine_baud();
}
