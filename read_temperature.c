#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include "adxl357.h"

#define DATA_SIZE 2
#define TIME_MAX_FILE 60

#define CE_ADXL 0
#define CSV_FILE_PATH "./RecordsTemp/"
#define OUTPUT_FILE_NAME "TempData"


FILE *outFile, *logFile;



void createNewFile(struct tm tm, struct timeval micro){
    char fileName[255];
    sprintf(fileName, "%s%s", CSV_FILE_PATH, OUTPUT_FILE_NAME);

    char name[255];
    sprintf(name, "%s_%d-%02d-%02d_%02d-%02d-%02d.csv", fileName, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    outFile = fopen(name, "w+");
    fprintf(outFile, "time; temp;\n");
}



void parentWriteInFile() {
    long data1, data2;
    int sample = 0;
    float sample_time = 0;
    float temp = 0;   

    int spi0;
    int ce0 = CE_ADXL;

    Adxl357_Init(&spi0, ce0, ADXL357_SPIBITS_8, SPI_MODE_0, ADXL357_SPISPEED_10MHZ);
    Adxl357_Reset(spi0);
    Adxl357_SetFilter(spi0, ADXL357_ODR_HPF_CORNER_NONE, ADXL357_ODR_LPF_2000Hz_500Hz);
    Adxl357_SetAccelRange(spi0, ADXL357_RANGE_40G);
    Adxl357_SetPowerCtl(spi0, ADXL357_POWER_ALL_ON/*0x06*/);

    unsigned char deviceId = Adxl357_GetDeviceId(spi0);
    printf("Device ID = %d\n", deviceId);


    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    struct timeval starttime, endtime;
    gettimeofday(&starttime, NULL);

    createNewFile(tm, starttime);

    while(1) {
        data1 = Adxl357_GetRegTemp1(spi0);
        data2 = Adxl357_GetRegTemp2(spi0);                
        gettimeofday(&endtime, NULL);
		sample_time = endtime.tv_sec - starttime.tv_sec + (endtime.tv_usec - starttime.tv_usec) / 1e6;


        temp = Adxl357_ConvertTempData(data1, data2);
        fprintf(outFile, "%d; %f;\n", (int)sample_time, temp);
        // printf("Data converted: X=%f / Y=%f / Z=%f.\n", accX, accY, accZ);
        // printf("time = %lf\n", sample_time);
        

        //Open another file after TIME_MAX_FILE seconds
        if (sample_time >= TIME_MAX_FILE) {
            // printf("time passed = %f\n", sample_time);
            sample = 0;
            gettimeofday(&starttime, NULL);

            fclose(outFile);
            t = time(NULL);
            tm = *localtime(&t);
            
            createNewFile(tm, starttime);
        }

        sleep(1);
    }
}


int main() {
    parentWriteInFile();

    return 0;
}
