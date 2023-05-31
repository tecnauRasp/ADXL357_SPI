#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include "adxl357.h"
#include <pthread.h>

#define CE_ADXL 0
#define CSV_FILE_PATH "./CsvRecords/"

float OutputDataRate = 4000;        // 4KHz
float SamplingInterval = 0.00025;   // 1/4000 = 250ms


// Define a shared buffer to hold the data
#define BUFFER_SIZE 100
struct SensorData {
    long value1;
    long value2;
    long value3;
};
struct SensorData buffer[BUFFER_SIZE];
long buffer_index = 0;

// Mutex and condition variables for synchronization
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_full_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_empty_cond = PTHREAD_COND_INITIALIZER;

// Function to read data from the sensor
void* read_data(void* arg) {
    int spi0;
    int ce0 = CE_ADXL;

    FILE *outFile, *logFile;
    char *fileName;
    int sample = 0;
    float sampleTime = 0;
    //struct timespec starttime, stoptime;
    unsigned char isX = 0, accDataRead = 0, isFifoAligned = 0;

    fileName = "OutData.csv";


    outFile = fopen(fileName, "w+");
    logFile = fopen("logFile.txt", "w+");

    Adxl357_Init(&spi0, ce0, ADXL357_SPIBITS_8, SPI_MODE_0, ADXL357_SPISPEED_10MHZ);
    Adxl357_Reset(spi0);
    Adxl357_SetFilter(spi0, ADXL357_ODR_HPF_CORNER_NONE, ADXL357_ODR_LPF_4000Hz_1000Hz);
    Adxl357_SetAccelRange(spi0, ADXL357_RANGE_40G);
    Adxl357_SetPowerCtl(spi0, ADXL357_POWER_ALL_ON/*0x06*/);

    sample = 0;
    sampleTime = 0;

    isFifoAligned = 0;

    unsigned char deviceId = Adxl357_GetDeviceId(spi0);
    printf("Device ID = %d\n", deviceId);
    

    int ciclo = 0;
    
    while (1) {
        // Simulate reading data from the sensor
        long rawX = 0, rawY = 0, rawZ = 0;
        unsigned char isEmptyX = 0, isEmptyY = 0, isEmptyZ = 0; 
    

        // get status
        int status = Adxl357_GetStatus(spi0);
        int dataReady = (status & ADXL357_REG_STATUS_BIT_DATA_RDY) ? 1 : 0;
        int fifoOvr = (status & ADXL357_REG_STATUS_BIT_FIFO_OVR) ? 1 : 0;
        int fifoFull = (status & ADXL357_REG_STATUS_BIT_FIFO_FULL) ? 1 : 0;
        unsigned char fifoEntries = Adxl357_GetFifoEntries(spi0);

        fprintf(logFile, "Status=%d - DataReady=%d - FifoFull=%d - FifoOvr=%d.- FifoEntries=%d\n", status, dataReady, fifoFull, fifoOvr, fifoEntries);
        // printf("Status=%d - DataReady=%d - FifoFull=%d - FifoOvr=%d- FifoEntries=%d\n", status, dataReady, fifoFull, fifoOvr, fifoEntries);
        // printf("Ciclo=%d - Entries=%d\n", ciclo, fifoEntries);

        // clock_gettime(CLOCK_MONOTONIC, &starttime);

        if (fifoOvr) {
        fprintf(logFile, "FIFO has overrun.\n");
        printf("FIFO has overrun.\n");
        return 0;
        }

        while (fifoEntries > 0) {
        
            if (isFifoAligned == 0) {
                // read data
                Adxl357_GetRawAccelFromFifo(spi0, &rawX, &isEmptyX, &isX);
                fifoEntries--;

                if (isEmptyX == 0 && isX == 1) {
                    isFifoAligned = 1;
                    accDataRead = 1;
                    fprintf(logFile, "X detected (FifoEntries=%d).\n", (fifoEntries + 1));
                    // printf("X detected (FifoEntries=%d).\n", (fifoEntries + 1));

                    //printf("X=%d(%d)", rawX, (fifoEntries + 1));
                }
            } else {
                    // read data
                    if (accDataRead == 0) {
                        Adxl357_GetRawAccelFromFifo(spi0, &rawX, &isEmptyX, &isX);
                        printf("X=%d(%d-%d-%d)", rawX, fifoEntries, isEmptyX, isX);
                        fifoEntries--;
                        accDataRead++;

                        // check data
                        if (isEmptyX == 1 || isX == 0) {
                            fprintf(logFile, "FIFO X data incorrect (isEmpty=%d / isX=%d).\n", isEmptyX, isX);
                            printf("FIFO X data incorrect (isEmpty=%d / isX=%d).\n", isEmptyX, isX);
                            return 0;
                        }

                    } else if (accDataRead == 1) {
                        Adxl357_GetRawAccelFromFifo(spi0, &rawY, &isEmptyY, &isX);
                        printf(" / Y=%d(%d-%d-%d)", rawY, fifoEntries, isEmptyY, isX);
                        fifoEntries--;
                        accDataRead++;

                        // check data
                        if (isEmptyY == 1 || isX == 1) {
                            fprintf(logFile, "FIFO Y data incorrect (isEmpty=%d / isX=%d).\n", isEmptyY, isX);
                            printf("FIFO Y data incorrect (isEmpty=%d / isX=%d).\n", isEmptyY, isX);
                            return 0;
                        }

                    } else if (accDataRead == 2) {
                        Adxl357_GetRawAccelFromFifo(spi0, &rawZ, &isEmptyZ, &isX);
                        printf(" / Z=%d(%d-%d-%d)\n", rawZ, fifoEntries, isEmptyZ, isX);
                        fifoEntries--;
                        accDataRead = 0;
                        
                        // check data
                        if (isEmptyZ == 1 || isX == 1) {
                            fprintf(logFile, "FIFO Z data incorrect (isEmpty=%d / isX=%d).\n", isEmptyZ, isX);
                            printf("FIFO Z data incorrect (isEmpty=%d / isX=%d).\n", isEmptyZ, isX);
                            return 0;
                        }
                    }
                }
            

            // Acquire the mutex before accessing the shared buffer
            pthread_mutex_lock(&buffer_mutex);

            // Wait until the buffer has space to store the data
            while (buffer_index >= BUFFER_SIZE) {
                pthread_cond_wait(&buffer_empty_cond, &buffer_mutex);
            }

            // Store the data in the buffer
            struct SensorData data;
            data.value1 = rawX;
            data.value2 = rawY;
            data.value3 = rawZ;
            buffer[buffer_index++] = data;

            // Signal that the buffer is no longer empty
            pthread_cond_signal(&buffer_full_cond);

            // Release the mutex
            pthread_mutex_unlock(&buffer_mutex);

        }
    }

    return NULL;
}

// Function to write data to the file
void* write_data(void* arg) {
    while (1) {
        // Acquire the mutex before accessing the shared buffer
        pthread_mutex_lock(&buffer_mutex);

        // Wait until the buffer has data to be written
        while (buffer_index <= 0) {
            pthread_cond_wait(&buffer_full_cond, &buffer_mutex);
        }

        // Get the data from the buffer
        struct SensorData data = buffer[--buffer_index];

        // Signal that the buffer is no longer full
        pthread_cond_signal(&buffer_empty_cond);

        // Release the mutex
        pthread_mutex_unlock(&buffer_mutex);

        // Write the data to the file
        write_to_file(data.value1, data.value2, data.value3);
    }

    return NULL;
}

int main() {
    pthread_t reader_thread, writer_thread;

    // Create the reader and writer threads
    pthread_create(&reader_thread, NULL, read_data, NULL);
    pthread_create(&writer_thread, NULL, write_data, NULL);

    // Wait for the threads to finish (which will never happen in this example)
    pthread_join(reader_thread, NULL);
    pthread_join(writer_thread, NULL);

    return 0;
}
