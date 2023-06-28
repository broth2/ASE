#include "pMod.h"
#include "math.h"


char const Temp[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};


void pMod_sendCommand(char *data){
    static const char *TX_TASK_TAG = "TX_TASK";
    //calculate the checksum of data
    // assign data to a variable named command
    char command[100] = {0};
    strcpy(command, data);

    
    char Check = command[1];
    char Check_char[3] = {0};
    uint8_t i = 0;
    for(i = 2; command[i] != '\0'; i++){
        Check ^= command[i]; // checkValue 
    }

    Check_char[0] = Temp[Check/16%16];
    Check_char[1] = Temp[Check%16];
    Check_char[2] = '\0';
    
    // make data be in format of data*Check\r\n
    strcat(command, "*");
    strcat(command, Check_char);
    strcat(command, "\r\n");

    //send data
    uint8_t txBytes = uart_write_bytes(UART_NUM_1, command, strlen(command));
    //print txBytes
    ESP_LOGI(TX_TASK_TAG, "Wrote %d bytes", txBytes);
    // error handling
    if (txBytes != strlen(command)) {
        ESP_LOGI(TX_TASK_TAG, "Didn't write everything");
    }
    else{
        ESP_LOGI(TX_TASK_TAG, "Wrote everything");
    }

    //clear command
    memset(command, 0, sizeof(command));
    memset(Check_char, 0, sizeof(Check_char));
    Check = 0;

}




GNRMC pMod_getGNRMC(uint8_t* data)  {
    GNRMC GPS; // Create a local GNRMC variable
    
    
     sscanf((const char*)data, "$GPRMC,%2hhu%2hhu%2hhu.%*[^,],%c,%lf,%c,%lf,%c,%lf,%lf",
           &GPS.Time_H, &GPS.Time_M, &GPS.Time_S, &GPS.Status, &GPS.Lat, &GPS.Lat_area,
           &GPS.Lon, &GPS.Lon_area, &GPS.Speed, &GPS.Course);

    //check if the data is valid, if Gps.Status is 'V', then it is invalid
    if (GPS.Status == 'V') {
        GPS.Time_H = 0;
        GPS.Time_M = 0;
        GPS.Time_S = 0;
        GPS.Lat_area = 0;
        GPS.Lon_area = 0;
        GPS.Speed = 0;
        GPS.Course = 0;
        GPS.Lat = 0;
        GPS.Lon = 0;
        return GPS;
    }
    else if(GPS.Status == 'A'){
        
        if (GPS.Lat_area == 'S') {
            GPS.Lat = -GPS.Lat;
        }
        if (GPS.Lon_area == 'W') {
            GPS.Lon = -GPS.Lon;
        }

        double lat_deg = (int)(GPS.Lat / 100);
        double lat_min = fmod(GPS.Lat, 100.0);
        double lat_sec = fmod(lat_min, 1.0) * 60;
        lat_min = (int)lat_min;


        double lon_deg = (int)(GPS.Lon / 100);
        double lon_min = fmod(GPS.Lon, 100.0);
        double lon_sec = fmod(lon_min, 1.0) * 60;

        GPS.Lat = lat_deg + (lat_min / 60) + (lat_sec / 3600);
        GPS.Lon = lon_deg + (lon_min / 60) + (lon_sec / 3600);


        return GPS;
    }
    else{
        GPS.Time_H = 0;
        GPS.Time_M = 0;
        GPS.Time_S = 0;
        GPS.Lat_area = 0;
        GPS.Lon_area = 0;
        GPS.Speed = 0;
        GPS.Course = 0;
        GPS.Lat = 0;
        GPS.Lon = 0;
        GPS.Status = 'V';
        return GPS;
    }

}
