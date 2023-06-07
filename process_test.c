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

#define DATA_SIZE 3
#define TIME_MAX_FILE 5

#define CE_ADXL 0
#define CSV_FILE_PATH "./CsvRecords/"

float OutputDataRate = 4000;        // 4KHz
float SamplingInterval = 0.00025;   // 1/4000 = 250ms

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

    // logFile = fopen("logFile.txt", "w+");

    Adxl357_Init(&spi0, ce0, ADXL357_SPIBITS_8, SPI_MODE_0, ADXL357_SPISPEED_10MHZ);
    Adxl357_Reset(spi0);
    Adxl357_SetFilter(spi0, ADXL357_ODR_HPF_CORNER_NONE, ADXL357_ODR_LPF_4000Hz_1000Hz);
    Adxl357_SetAccelRange(spi0, ADXL357_RANGE_40G);
    Adxl357_SetPowerCtl(spi0, ADXL357_POWER_ALL_ON/*0x06*/);

    unsigned char deviceId = Adxl357_GetDeviceId(spi0);
    printf("Device ID = %d\n", deviceId);
    

    int ciclo = 0;
    
    while (1) {

        // get status
        int status = Adxl357_GetStatus(spi0);
        // int dataReady = (status & ADXL357_REG_STATUS_BIT_DATA_RDY) ? 1 : 0;
        // int fifoFull = (status & ADXL357_REG_STATUS_BIT_FIFO_FULL) ? 1 : 0;
        int fifoOvr = (status & ADXL357_REG_STATUS_BIT_FIFO_OVR) ? 1 : 0;
        unsigned char fifoEntries = Adxl357_GetFifoEntries(spi0);

        // fprintf(logFile, "Status=%d - DataReady=%d - FifoFull=%d - FifoOvr=%d.- FifoEntries=%d\n", status, dataReady, fifoFull, fifoOvr, fifoEntries);
        // printf("Status=%d - DataReady=%d - FifoFull=%d - FifoOvr=%d- FifoEntries=%d\n", status, dataReady, fifoFull, fifoOvr, fifoEntries);
        // printf("Ciclo=%d - Entries=%d\n", ciclo, fifoEntries);

        // clock_gettime(CLOCK_MONOTONIC, &starttime);

        if (fifoOvr) {
            // fprintf(logFile, "FIFO has overrun.\n");
            printf("FIFO has overrun.\n");
            exit(0);
            // isFifoAligned = 0;
            
        }

        while (fifoEntries > 0) {
        
            if (!isFifoAligned) {
                // read data
                Adxl357_GetRawAccelFromFifo(spi0, &rawX, &isEmptyX, &isX);

                if (!isEmptyX && isX) {
                    Adxl357_GetRawAccelFromFifo(spi0, &rawY, &isEmptyY, &isNotY);
                    Adxl357_GetRawAccelFromFifo(spi0, &rawZ, &isEmptyZ, &isNotZ);
                    write_flag = 1;
                    
                    // fprintf(logFile, "X detected (FifoEntries=%d).\n", (fifoEntries + 1));
                    // printf("X detected (FifoEntries=%d).\n", (fifoEntries + 1));
                    // printf("X=%d(%d)", rawX, (fifoEntries + 1));
                }
                
            } else {

                // read data
                Adxl357_GetRawAccelFromFifo(spi0, &rawX, &isEmptyX, &isX);
                Adxl357_GetRawAccelFromFifo(spi0, &rawY, &isEmptyY, &isNotY);
                Adxl357_GetRawAccelFromFifo(spi0, &rawZ, &isEmptyZ, &isNotZ);
                write_flag = 1;

                 

                // check data
                if (isEmptyX || !isX || isEmptyY || isNotY || isEmptyZ || isNotZ) {
                    // fprintf(logFile, "FIFO X data incorrect (isEmpty=%d / isX=%d).\n", isEmptyX, isX);
                    printf("FIFO data incorrect (isEmptyX=%d / isX=%d ) (isEmptyY=%d / isNotY=%d ) (isEmptyZ=%d / isNotZ=%d ).\n", isEmptyX, isX);
                    close(pipeWriteEnd);
                    kill(getppid(), SIGTERM);
                    exit(0);
                }

            }

            // Write the sensor data to the pipe
            if (write_flag) {                // Write the sensor data to the pipe
                data[0] = rawX;
                data[1] = rawY;
                data[2] = rawZ;

                // printf("X=%d(%d-%d-%d)", rawX, fifoEntries, isEmptyX, isX);
                // printf("Y=%d(%d-%d-%d)", rawY, fifoEntries, isEmptyY, isX);
                // printf("Z=%d(%d-%d-%d)", rawY, fifoEntries, isEmptyY, isX);   

                write(pipeWriteEnd, data, sizeof(data));
                write_flag = 0;

            }

            unsigned char fifoEntries = Adxl357_GetFifoEntries(spi0);

        }    
    }

    close(pipeWriteEnd);
    kill(getppid(), SIGTERM);
    exit(0);
}

void createNewFile(struct tm tm, struct timeval micro){
    char fileName[255];
    sprintf(fileName, "%s%s", CSV_FILE_PATH, "OutData");

    char name[255];
    sprintf(name, "%s_%d-%02d-%02d_%02d-%02d-%02d-%06d.csv", fileName, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, micro.tv_usec);
    outFile = fopen(name, "w+");
    fprintf(outFile, "time; x; y; z;\n");
}

void childWriteInFile(int pipeReadEnd) {
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

        gettimeofday(&endtime, NULL);
		float sample_time = endtime.tv_sec - starttime.tv_sec + (endtime.tv_usec - starttime.tv_usec) / 1e6;

        accX = Adxl357_ConvertAccelData(data[0], ADXL357_RANGE_40G);
        accY = Adxl357_ConvertAccelData(data[1], ADXL357_RANGE_40G);
        accZ = Adxl357_ConvertAccelData(data[2], ADXL357_RANGE_40G);
        fprintf(outFile, "%.5f; %f; %f; %f;\n", sample_time, accX, accY, accZ);
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
        childWriteInFile(pipefd[0]);
    }

    return 0;
}
