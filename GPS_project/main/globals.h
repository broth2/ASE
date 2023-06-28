#ifndef GLOBALS_H
#define GLOBALS_H
#include <pMod.h>
#include "freertos/semphr.h"

extern GNRMC GPS;
extern SemaphoreHandle_t semaphore_gps, semaphore_http_complete;
extern double boundingAreaW, boundingAreaE, boundingAreaN, boundingAreaS;
extern int position_fix_interval, baudrate, last_registered_time, connected_state;
extern bool low_power_mode, received_ack;
#endif


/*case 6:
            if (low_power_mode == 0)
            {
                esp_wifi_stop();
                low_power_mode = 1;
                printf("Low power mode set\n");
            }
            else
            {
                low_power_mode = 0;
                esp_wifi_start();
                esp_wifi_connect();
                if (connected_state == 0) {
                    esp_wifi_connect();
                }
            }
            break;*/