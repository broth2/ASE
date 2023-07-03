#include "driver/uart.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <esp_wifi_types.h>
#include "esp_http_client.h"
#include "pMod.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include <sys/time.h>
#include "esp_sntp.h"
#include "esp_sleep.h"


void uart_gps_init(int baudrate);

void wifi_init_sta();
void change_uart_gps(int baudrate);
void get_actual_hour(struct tm *tm_struct);
void uart_terminal_init(void);
void gpsTask(void *parameters);
void wifiCheckTask(void *parameters);
void terminalTask(void *parameters);
void http_post_task(void *pvParameters);
void upT(void *parameters);
void updateTask();
void initialize_sntp();
void wait_for_sntp_sync();
// Create a file and write configurations
void update_config_file(); 
void read_config_file();
void read_data_file();
void init_filesystem();
void wifiTask(void *parameters);
void initialize_sntp();
void wait_for_sntp_sync();
void sntp_sync_notification(struct timeval *tv);
void on_got_ip(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void on_wifi_disconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void configure_UART();