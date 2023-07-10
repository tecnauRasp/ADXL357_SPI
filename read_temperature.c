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

float OutputDataRate = 4000;        // 4KHz
float SamplingInterval = 0.00025;   // 1/4000 = 250us

FILE *outFile, *logFile;

pid_t pid1, pid2;
int pipefd[2];


void childReadFromSensor(int pipeWriteEnd) {

    long data[DATA_SIZE];

    int spi0;
    int ce0 = CE_ADXL;

    // FILE *logFile;
    char *fileName;
    //struct timespec starttime, stoptime;
    unsigned char isX = 0, isNotY = 0, isNotZ = 0;
    unsigned char write_flag = 0, isFifoAligned = 0;
    long rawX = 0, rawY = 0, rawZ = 0;
    unsigned char isEmptyX = 0, isEmptyY = 0, isEmptyZ = 0;

    // struct timespec ts;
    // ts.tv_sec = 0;
    // ts.tv_nsec = 250000;

    // logFile = fopen("logFile.txt", "w+");

    Adxl357_Init(&spi0, ce0, ADXL357_SPIBITS_8, SPI_MODE_0, ADXL357_SPISPEED_10MHZ);
    Adxl357_Reset(spi0);
    Adxl357_SetFilter(spi0, ADXL357_ODR_HPF_CORNER_NONE, ADXL357_ODR_LPF_4000Hz_1000Hz);
    Adxl357_SetAccelRange(spi0, ADXL357_RANGE_40G);
    Adxl357_SetPowerCtl(spi0, ADXL357_POWER_ALL_ON/*0x06*/);

    unsigned char deviceId = Adxl357_GetDeviceId(spi0);
    printf("Device ID = %d\n", deviceId);
    
    
    while (1) {

        usleep(100);

        data[0] = Adxl357_GetRegTemp1(spi0);
        data[1] = Adxl357_GetRegTemp2(spi0);
        write(pipeWriteEnd, data, sizeof(data));
    }

    close(pipeWriteEnd);
    kill(getppid(), SIGTERM);
    exit(0);
}


void createNewFile(struct tm tm, struct timeval micro){
    char fileName[255];
    sprintf(fileName, "%s%s", CSV_FILE_PATH, OUTPUT_FILE_NAME);

    char name[255];
    sprintf(name, "%s_%d-%02d-%02d_%02d-%02d-%02d.csv", fileName, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    outFile = fopen(name, "w+");
    fprintf(outFile, "time; x; y; z; temp;\n");
}

void parentWriteInFile(int pipeReadEnd) {
    long data[DATA_SIZE];
    int sample = 0;
    float sample_time = 0;   
    
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    struct timeval starttime, endtime;
    gettimeofday(&starttime, NULL);

    createNewFile(tm, starttime);

    while(1) {
        
        // Read the sensor data from the pipe
        read(pipeReadEnd, data, sizeof(data));
        
        // Write the data to the file
        float accX = 0, accY = 0, accZ = 0;
        float temp = 0;

        gettimeofday(&endtime, NULL);
		float sample_time = endtime.tv_sec - starttime.tv_sec + (endtime.tv_usec - starttime.tv_usec) / 1e6;

        temp = Adxl357_ConvertTempData(data[0], data[1]);
        fprintf(outFile, "%.5f; %f;\n", sample_time, temp);
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
    }

    close(pipeReadEnd);
    exit(0);
}


int main() {
    int status;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    pid1 = fork();

    if (pid1 == -1) {
        perror("fork1");
        exit(1);
    } else if (pid1 == 0) {
        close(pipefd[0]); // Close the unused read end of the pipe in the child process
        childReadFromSensor(pipefd[1]);

    } else {
        close(pipefd[1]); // Close the unused write end of the pipe in the child process
        parentWriteInFile(pipefd[0]);
    }

    return 0;
}
