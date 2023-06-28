#include "globals.h"

//inicialize GNRMC GPS var
GNRMC GPS;
SemaphoreHandle_t semaphore_gps, semaphore_http_complete;
double boundingAreaW = 0;
double boundingAreaE = 0;
double boundingAreaN = 0;
double boundingAreaS = 0;
int position_fix_interval = 400;
int baudrate = 9600;
int last_registered_time = 16;
int connected_state = 0;
bool low_power_mode = false;

