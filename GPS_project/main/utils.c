    #include "utils.h"
    #include "globals.h"
    #include "esp_spiffs.h"
    #include "esp_sntp.h"

    #define EXAMPLE_ESP_WIFI_SSID "iPhone de João"
    #define EXAMPLE_ESP_WIFI_PASS "batman123"
    #define EXAMPLE_ESP_MAXIMUM_RETRY 5

    #define CONFIG_FILE_PATH "/spiffs/config.txt"
    #define FILE_PATH "/spiffs/data.txt"

    #define WIFI_CONNECTED_BIT BIT0
    #define WIFI_FAIL_BIT      BIT1

    #define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
    #define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH

    #define BOUNDING_AREA_SIZE 10

    static int s_retry_num = 0;
    bool newGpsDataAvailable = false;
    /* FreeRTOS event group to signal when we are connected*/
    static EventGroupHandle_t s_wifi_event_group;

    static  esp_http_client_config_t http_config = {
    .url =  "http://172.20.10.6:8000/api",
    .method = HTTP_METHOD_POST,
  };

  static esp_http_client_handle_t client;

    static void event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
    {   
        static const char *TAG = "wifi station";

        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
        } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
            if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
                if(low_power_mode == false){
                    esp_wifi_connect();
                    s_retry_num++;
                    ESP_LOGI(TAG, "retry to connect to the AP");
                }
            } else {
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                connected_state = 0;
            }
            ESP_LOGI(TAG,"connect to the AP fail");
        } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            s_retry_num = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            connected_state = 1;
        }
    }

void configure_UART(){

    const uart_port_t uart_num = CONFIG_ESP_CONSOLE_UART_NUM;
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 122,
};
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);
}


    void change_uart_gps(int baud_rate) {
        printf("Changing UART baud rate to %d\n", baud_rate);
        uart_config_t uart_config = {
            .baud_rate = baud_rate,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
            .rx_flow_ctrl_thresh = 122,
            .source_clk = UART_SCLK_DEFAULT,
        };
        ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
        ESP_ERROR_CHECK(uart_set_baudrate(UART_NUM_1, baud_rate));
    }


    void uart_gps_init(int baudrate) {
        const uart_config_t uart_config = {
            .baud_rate = baudrate,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
        };
        // We won't use a buffer for sending data.
        uart_driver_install(UART_NUM_1, 800 * 2, 0, 0, NULL, 0);
        uart_param_config(UART_NUM_1, &uart_config);
        uart_set_pin(UART_NUM_1, 4, 5, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }


void on_wifi_disconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // TAG
    connected_state = 0;
    static const char *TAG = "wifi station";

    ESP_LOGI(TAG, "Wi-Fi disconnected. Reconnecting...");
    if (!low_power_mode) {
        esp_wifi_connect();
    }
}
// Global flag variable to track SNTP synchronization status
bool sntp_synced = false;

void on_got_ip(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // TAG
    connected_state = 1;
    client = esp_http_client_init(&http_config);
    static const char *TAG = "wifi station";

    printf("Wi-Fi Connected...\n");
}

void wifi_init_sta(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &(wifi_config_t){
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
        }
    });

    esp_wifi_start();
    esp_wifi_connect();

    // Register the IP_EVENT_STA_GOT_IP event handler
    esp_event_handler_instance_t instance;
    esp_event_handler_instance_t instance_disconnect;
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_got_ip,NULL, &instance);

    // Register the WIFI_EVENT_STA_DISCONNECTED event handler
    esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, on_wifi_disconnect,NULL, &instance_disconnect);
}

    void init_filesystem() {
        // Configure and mount SPIFFS/LittleFS
        
        esp_err_t ret;
        esp_vfs_spiffs_conf_t conf = {
            .base_path = "/spiffs",
            .partition_label = "storage",
            .max_files = 5,
            .format_if_mount_failed = true
        };
        esp_vfs_spiffs_register(&conf);

    }

void read_config_file() {
    FILE* file = fopen(CONFIG_FILE_PATH, "r");
    if (file == NULL) {
        printf("Failed to open file\n");
        return;
    }

    char* lines[60]; // Declare the array to store the lines
    int line_count = 0;

    // Allocate memory for each line and read the lines into the array
    while (1) {
        lines[line_count] = (char*)malloc(60 * sizeof(char));
        if (fgets(lines[line_count], 60, file) == NULL) {
            break;
        }
        line_count++;
    }

    // Print the file contents
    printf("###File###\n");
    for (int i = 0; i < line_count; i++) {
        printf("%s", lines[i]);
    }

    fclose(file);

    // Assign each line to a variable
    baudrate = atoi(lines[0]);
    last_registered_time = atoi(lines[1]);
    position_fix_interval = atoi(lines[2]);

    // Print the configuration variables
    printf("###Configuration Variables###\n");
    printf("Baudrate: %d\n", baudrate);
    printf("Last Registered Time: %d\n", last_registered_time);
    printf("Position Fix Interval: %d\n", position_fix_interval);

    // Free the allocated memory for lines
    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }
}



void read_data_file() {
    FILE* file = fopen(FILE_PATH, "r");
    if (file == NULL) {
        printf("Failed to open file\n");
        return;
    }
    char json_data[1000];
    char* lines[60]; // Declare the array to store the lines
    int line_count = 0;

    esp_http_client_config_t http_config_upload_stored_Data = {
        .url =  "http://172.20.10.6:8000/stored_data",
        .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t  client_stored_data = esp_http_client_init(&http_config_upload_stored_Data);

    // Allocate memory for each line and read the lines into the array
    while (1) {
        lines[line_count] = (char*)malloc(60 * sizeof(char));
        if (fgets(lines[line_count], 60, file) == NULL) {
            break;
        }
        line_count++;
    }

    fclose(file);

    double Speed = 0;
    double Course = 0;
    double Longitude = 0;
    double Latitude = 0;
    int Time_h = 0;
    int Time_m = 0;
    int Time_s = 0;
    int Status = 0;

    // Print the lines from the array and assign values
    for (int i = 0; i < line_count; i++) {
        int last = 0;
        printf("%s", lines[i]);

        int result = sscanf(lines[i], "%lf, %lf, %lf, %lf, %d, %d, %d, %d\n",
                        &Speed, &Course, &Longitude, &Latitude,
                        &Time_h, &Time_m, &Time_s, &Status);


        if (result != 8) {
            printf("Invalid line format: %s", lines[i]);
            continue;
        }
        if(i == line_count -1){
            last = 1;
        }
        //append variables to json_data
        sprintf(json_data, "{\"Speed\":%lf,\"Course\":%lf,\"Longitude\":%lf,\"Latitude\":%lf,\"Time_H\":%d,\"Time_M\":%d,\"Time_S\":%d,\"Status\":%d,\"Last\": %d}", Speed, Course, Longitude, Latitude, Time_h, Time_m, Time_s, Status,last);
        esp_http_client_set_post_field(client_stored_data, json_data, strlen(json_data));
        esp_http_client_set_header(client_stored_data, "Content-Type", "application/json");
        // Perform the HTTP request
        esp_err_t err = esp_http_client_perform(client_stored_data);
        if (err == ESP_OK) {
            printf("HTTP POST Status = %d, content_length = %lld\n",
                esp_http_client_get_status_code(client_stored_data),
                esp_http_client_get_content_length(client_stored_data));
            // Signal that HTTP POST is complete

        } else {
            printf("HTTP POST request failed: %s\n", esp_err_to_name(err));
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }

      esp_http_client_cleanup(client_stored_data);

    // Free the allocated memory for lines
    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }

    //delete file content after reading
    file = fopen(FILE_PATH, "w");
    fclose(file);

}




// Create a file and write configurations
void update_config_file() {
        FILE* file = fopen(CONFIG_FILE_PATH, "w");
        if (file == NULL) {
            printf("Failed to create file\n");
            return;
        }

        last_registered_time = GPS.Time_H;
        position_fix_interval = position_fix_interval;
        // Write your configuration data to the file
        fprintf(file, "%d\n",9600);
        fprintf(file, "%d\n",last_registered_time);
        fprintf(file, "%d\n",position_fix_interval);
        
        

        fclose(file);
}


void write_data_file(double speed, double course, double lon, double lat, int time_h, int time_m, int time_s, int status) {
    FILE* file = fopen(FILE_PATH, "a");
    if (file == NULL) {
        printf("Failed to create file\n");
        return;
    }
    fclose(file);

    file = fopen(FILE_PATH, "r");
    if (file == NULL) {
        printf("Failed to open file\n");
        return;
    }

    // Move the file pointer to the beginning of the file
    fseek(file, 0, SEEK_SET);
    printf("Storing in the data file...\n");
    // Check if the file has more than 50 lines
    int line_count = 0;
    int c;
    for (c = getc(file); c != EOF; c = getc(file)) {
        if (c == '\n') {
            line_count++;
        }
    }
    fclose(file);

    if (line_count >= 50) {
        // Delete the file
        if (remove(FILE_PATH) != 0) {
            printf("Failed to delete file\n");
            return;
        }

        // Create a new file
        file = fopen(FILE_PATH, "w");
        if (file == NULL) {
            printf("Failed to create file\n");
            return;
        }
    }

    // Open the file in append mode
    file = fopen(FILE_PATH, "a");
    if (file == NULL) {
        printf("Failed to open file\n");
        return;
    }

    /*if(status != 86){
        // Write the data to the file

        fprintf(file, "%lf, %lf, %lf, %lf, %d, %d, %d, %d\n",
        speed, course, lon, lat, time_h, time_m, time_s, status);
    }*/
    fprintf(file, "%lf, %lf, %lf, %lf, %d, %d, %d, %d\n",
    speed, course, lon, lat, time_h, time_m, time_s, status);


    fclose(file);
}






void createBoundingArea(double lat, double lon, double radius_m) {
        // Convert the radius from meters to degrees

        printf("lat, lon, radius_m: %lf %lf %lf\r\n", lat, lon, radius_m);
        double radius_deg = radius_m / 100000.0;

        // Calculate the delimiting area's inner bounds
        double minLat = lat - radius_deg;
        double maxLat = lat + radius_deg;
        double minLon = lon - radius_deg;
        double maxLon = lon + radius_deg;


        boundingAreaN = maxLat;
        boundingAreaS = minLat;
        boundingAreaE = maxLon;
        boundingAreaW = minLon;

        printf("Bounding area: %lf %lf %lf %lf\r\n", boundingAreaN, boundingAreaS, boundingAreaE, boundingAreaW);   
}

bool isInsideBoundingArea(double lat, double lon) {

    if (lat <= boundingAreaN && lat >= boundingAreaS && lon <= boundingAreaE && lon >= boundingAreaW) {
        gpio_set_level(GPIO_NUM_16, 1);
        return true;
    }
    else {
        gpio_set_level(GPIO_NUM_16, 0);
        return false;
    }
}

void terminalTask(void *parameters) {
        int operation = 0;

        while (!operation) {
            int choice = 0;
            
            printf("\n\n");
            printf("Choose an operation:\n");
            printf("0 - Set NMEA Output\n ");
            printf("1 - Set Start mode 1\n");
            printf("2 - Set Position Fix Interval\n");
            printf("3 - Get Current Position \n");
            printf("4 - Get Current Configs\n");
            if(connected_state == 1){
                printf("5 - Upload Stored Data to Server\n");
            }
            if(low_power_mode == false){
                printf("6 - Set Low Power Mode");
            }else{
                printf("6 - Set Normal Mode");
            }
            
            scanf("%d", &choice); // Move scanf inside the loop

            switch (choice) {
                case 0:
                    pMod_sendCommand(SET_NMEA_OUTPUT);
                    printf("NMEA OUTPUT SET\n");
                    break;
                case 1:
                    int start_mode = 0;
                    printf("1 - HOT START\n");
                    printf("2 - WARM START\n");
                    printf("3 - COLD START\n");
                    printf("4 - FACTORY RESET\n");
                    scanf("%d", &start_mode);
                    getchar(); // Consume the newline character from the buffer
                    switch (start_mode) {
                        case 1:
                            pMod_sendCommand(HOT_START);
                            
                            printf("HOT STARTING\n");
                            break;

                        case 2:
                            pMod_sendCommand(WARM_START);
                            printf("WARM STARTING\n");
                            break;

                        case 3:
                            pMod_sendCommand(COLD_START);
                            printf("COLD STARTING\n");
                            break;

                        case 4:
                            pMod_sendCommand(FULL_COLD_START);
                            printf("FACTORY RESET\n");
                            break;

                        default:
                            printf("Invalid option. Try again.\n");
                            break;
                    }

                    break;
                case 2:
                    //SET POSITION FIX INTERVAL
                    int interval = 0;
                    printf("1 - 200ms\n");
                    printf("2 - 400ms\n");
                    printf("3 - 800ms\n");
                    printf("4 - 1s\n");
                    printf("5 - 2s\n"); 
                    scanf("%d",&interval);
                    getchar(); // Consume the newline character from the buffer
                    printf("Setting position fix interval...\n\n");
                    printf("Interval: %d\n", interval);
                    switch (interval)
                    {
                    case 1:
                        pMod_sendCommand(SET_POS_FIX_200MS);
                        position_fix_interval = 200;
                        update_config_file();
                        break;
                    case 2:
                        pMod_sendCommand(SET_POS_FIX_400MS);
                        position_fix_interval = 400;
                        update_config_file();
                        break;
                    case 3:
                        pMod_sendCommand(SET_POS_FIX_800MS);
                        position_fix_interval = 800;

                        update_config_file();
                        break;
                    case 4:
                        pMod_sendCommand(SET_POS_FIX_1S);
                        position_fix_interval = 1000;
                        update_config_file();
                        break;
                    case 5:
                        pMod_sendCommand(SET_POS_FIX_2S);
                        position_fix_interval = 2000;
                        update_config_file();
                        break;
                    
                    default:
                        printf("Invalid option. Try again.\n");
                        break;
                    }
                    break;
                case 3:
                    //PRINT CURRENT POSITION from gps variable
                    printf("Latitude: %lf\n", GPS.Lat);
                    printf("Longitude: %lf\n", GPS.Lon);
                    printf("Speed: %lf\n", GPS.Speed);
                    printf("Course: %lf\n", GPS.Course);
                    printf("Time: %d:%d:%d\n", GPS.Time_H, GPS.Time_M, GPS.Time_S);
                    printf("Status: %d\n", GPS.Status);
                    break;
                case 4:
                    read_config_file();
                    break;
                case 5:
                    read_data_file();
                    break;
                case 6:
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
                    break;
                default:
                    printf("Invalid operation. Try again.\n");
                    break;
        }
    }


}


void gpsTask(void *parameters) {
  uint8_t* data = (uint8_t*) malloc(BUFFSIZE+1);


  while(1) {
    const int rxBytes = uart_read_bytes(UART_NUM_1, data, BUFFSIZE, 100 / portTICK_PERIOD_MS);
    if (rxBytes > 0) {
        //print data
      data[rxBytes] = 0;
        //printf("%s", (char*)data);
      // check for $PMTK001,314
        if(strstr((char*)data, "$PMTK001,220")) {
            printf("Position Fix Interval Acknowledged\n");
        }

      if(xSemaphoreTake(semaphore_gps, portMAX_DELAY) == pdTRUE) {
        printf("GPS task running with position fix interval of %d...\n\n", position_fix_interval);
        GPS = pMod_getGNRMC(data);

        xSemaphoreGive(semaphore_http_complete); // Release the semaphore
        }
    }
    vTaskDelay(position_fix_interval / portTICK_PERIOD_MS);
  }
  free(data);
}


void http_post_task(void *pvParameters) {
   static const char *TAG = "http post task";
  // Create an HTTP client handle

  //esp_http_client_handle_t client = esp_http_client_init(&http_config);
  // Create a JSON string with the GPS data
  char json_data[256];
    uint8_t first_run = 1;
    uint8_t run = 0;
  while (1) {
    // Check if new GPS data is available

    if (xSemaphoreTake(semaphore_http_complete, portMAX_DELAY) == pdTRUE) {
      // Wait for the GPS data mutex to be available
        run++;
        printf("HTTP POST task running...\n\n");
        double lat = GPS.Lat;
        double lon = GPS.Lon;
        double speed = GPS.Speed;
        double course = GPS.Course;
        uint8_t status = GPS.Status;
        uint8_t time_h = GPS.Time_H;
        uint8_t time_m = GPS.Time_M;
        uint8_t time_s = GPS.Time_S;
        // Create a JSON string with the GPS data
        snprintf(json_data, sizeof(json_data),
          "{\"Time_H\": %d, \"Time_M\": %d, \"Time_S\": %d, "
          "\"Status\": %d, "
          "\"Latitude\": %lf, "
          "\"Longitude\": %lf, "
          "\"Speed\": %lf, "
          "\"Course\": %lf}",
          time_h, time_m, time_s, status, lat, lon, speed, course);

          
        if (first_run == 1) {
            if (status == 'A'){
                createBoundingArea(lat, lon, BOUNDING_AREA_SIZE);
                first_run = 0;
            }
        }else{
           bool isInside = isInsideBoundingArea(lat, lon);
           if (isInside)
           {
                //set Led to off
                printf("Inside bounding area\n");
           }else{
                //set Led to on
                printf("Outside bounding area\n");
           }
           
        }
        
        update_config_file();
        // Release the mutex
       if(connected_state == true){
            esp_http_client_set_post_field(client, json_data, strlen(json_data));
            esp_http_client_set_header(client, "Content-Type", "application/json");
            // Perform the HTTP request
            esp_err_t err = esp_http_client_perform(client);
            if (err == ESP_OK) {
            printf("HTTP POST Status = %d, content_length = %lld\n",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
            // Signal that HTTP POST is complete

            } else {

            //printf("%lf %lf %lf %lf %d %d %d %d\n", lat, lon, speed, course, time_h, time_m, time_s, status);
            write_data_file(speed, course, lon, lat, time_h, time_m, time_s, status);
            
            }
        }else{

            write_data_file(speed, course, lon, lat, time_h, time_m, time_s, status);
        }
        
        if(low_power_mode == true){
            printf("Sleep task running...\n");
            // Perform sleep operations
            
            esp_sleep_enable_timer_wakeup(position_fix_interval - 50); // 200 milliseconds sleep
            esp_light_sleep_start();

             printf("Sleep task Woke...\n");   
        }
        xSemaphoreGive(semaphore_gps); // Signal sleep task 
    }

  }

  // Clean up
  esp_http_client_cleanup(client);
}


// Callback function to wait for SNTP synchronization completion
void sntp_sync_notification(struct timeval *tv) {
    //tag
    static const char *TAG = "sntp_sync_notification";
    ESP_LOGI(TAG, "SNTP Synchronization complete");
}

// Function to initialize SNTP synchronization
void initialize_sntp() {
    sntp_init();
    sntp_set_time_sync_notification_cb(sntp_sync_notification);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

// Function to wait for SNTP synchronization to complete
void wait_for_sntp_sync() {

    //tag
    static const char *TAG = "wait_for_sntp_sync";


    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}


// Function to get the actual hour
void get_actual_hour(struct tm *tm_struct) {
    time_t now;
    time(&now);
    localtime_r(&now, tm_struct);
}
