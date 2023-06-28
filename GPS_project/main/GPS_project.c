/* UART asynchronous example, that uses separate RX and TX tasks

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "esp_vfs_fat.h"
#include "string.h"
#include "freertos/semphr.h"
#include "globals.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "utils.h"
#include "pMod.h"
#include "Math.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "time.h"


#define UART_PORT UART_NUM_0
#define TXD_PIN (GPIO_NUM_4)
#define RXD_PIN (GPIO_NUM_5)
#define GPIO_NUM_16 16





static void configure_GPIO_out(int n){
    gpio_reset_pin(n);
    gpio_set_direction(n, GPIO_MODE_OUTPUT);
}



void State_Init(void)
{   

    esp_err_t ret = nvs_flash_init();
    wifi_init_sta();
    
    initialize_sntp();
    wait_for_sntp_sync();

    init_filesystem();

    configure_UART();
    configure_GPIO_out(GPIO_NUM_16);



    //READ CONFIG FILE
    read_config_file();
    uart_gps_init(baudrate);
    struct tm tm_struct;
    get_actual_hour(&tm_struct);
    uint8_t hour = tm_struct.tm_hour;
    //if difference between last registered time and current time is bigger than 3 hours, send COLD_START
    printf("hour: %d\n", hour);
    if(hour - last_registered_time > 3 || hour - last_registered_time < -3){
        pMod_sendCommand(COLD_START);
        printf("COLD STARTING\n");
    }
    else{
        //REGISTER DEVICE
        pMod_sendCommand(HOT_START);
        printf("HOT STARTING\n");
    }

    //evaluate position fix interval
    printf("position fix interval: %d\n", position_fix_interval);
    switch (position_fix_interval)
    {
    case 200:
        printf("200ms\n");
        pMod_sendCommand(SET_POS_FIX_200MS);
        break;
    case 400:
        printf("400ms\n");
        pMod_sendCommand(SET_POS_FIX_400MS);
        break;
    case 800:
        printf("800ms\n");
        pMod_sendCommand(SET_POS_FIX_800MS);
        break;
    case 1000:
        printf("1s\n");
        pMod_sendCommand(SET_POS_FIX_1S);
        break;
    case 2000:
        printf("2s\n");
        pMod_sendCommand(SET_POS_FIX_2S);
        break;
    
    default:
        break;
    }
    //CONFIGURE DRIVERS
    semaphore_gps = xSemaphoreCreateBinary();
    //set it free
    xSemaphoreGive(semaphore_gps);
    semaphore_http_complete = xSemaphoreCreateBinary();
    pMod_sendCommand(SET_NMEA_OUTPUT);
    vTaskDelay(100 / portTICK_PERIOD_MS);

}




void app_main(void)
{

    State_Init();

    pMod_sendCommand(SET_NMEA_OUTPUT);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    xTaskCreate(&gpsTask, "GPS_task", 4096, NULL, configMAX_PRIORITIES, NULL);

    xTaskCreate(&http_post_task, "http_post_task", 4096, NULL, 5, NULL);

    //acabar ligação pelo terminal



    xTaskCreate(&terminalTask, "terminal_Task", 4096, NULL, configMAX_PRIORITIES, NULL);
    //esp_log_level_set("*", ESP_LOG_NONE);

    
}
