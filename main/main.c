#include <stdio.h>
#include <string.h>

#include "cmd_sniffer.h"
#include "cmd_system.h"
#include "esp_console.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdkconfig.h"

static const char *TAG = "main";

#if CONFIG_EXAMPLE_STORE_HISTORY

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

static void initialize_filesystem(void) {
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 4,
        .format_if_mount_failed = true};
    esp_err_t err = esp_vfs_fat_spiflash_mount(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }
}
#endif  // CONFIG_EXAMPLE_STORE_HISTORY

void app_main(void) {
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();

#if CONFIG_EXAMPLE_STORE_HISTORY
    initialize_filesystem();
    repl_config.history_save_path = HISTORY_PATH;
#endif
    repl_config.prompt = "moose>";

    // install console REPL environment
#if CONFIG_ESP_CONSOLE_UART
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
#elif CONFIG_ESP_CONSOLE_USB_CDC
    esp_console_dev_usb_cdc_config_t cdc_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&cdc_config, &repl_config, &repl));
#elif CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t usbjtag_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&usbjtag_config, &repl_config, &repl));
#endif

    register_sniffertools();
    // register_system();

    printf("\n ==============================================================\n");
    printf(" |    Welcome to                                              |\n");
    printf(" |     SerialMoose!     _       _                             |\n");
    printf(" |                     ) \\     /_(                            |\n");
    printf(" |                       )_`-)-_(_                            |\n");
    printf(" |                        `,' `__,)                           |\n");
    printf(" |                       _/   ((                              |\n");
    printf(" |              ______,-'    )                                |\n");
    printf(" |             (            ,                                 |\n");
    printf(" |              \\  )    (   |                                 |\n");
    printf(" |             /| /`-----` /|                                 |\n");
    printf(" |             \\ \\        / |                                 |\n");
    printf(" |             |\\|\\      /| |\\                                |\n");
    // printf(" |     _ __ ___   ___   ___  ___  ___                         |\n");
    // printf(" |    | '_ ` _ \ / _ \ / _ \/ __|/ _ \                        |\n");
    // printf(" |    | | | | | | (_) | (_) \__ \  __/                        |\n");
    // printf(" |    |_| |_| |_|\___/ \___/|___/\___|                        |\n");
    printf(" |                                                            |\n");
    printf(" |  1. Try 'help', check all supported commands               |\n");
    printf(" |                                                            |\n");
    printf(" ==============================================================\n\n");

    // start console REPL
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
