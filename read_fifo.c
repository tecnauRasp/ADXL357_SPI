#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include "adxl357.h"

#define CE_ADXL 0
#define CSV_FILE_PATH "./CsvRecords/"

float OutputDataRate = 4000;        // 4KHz
float SamplingInterval = 0.00025;   // 1/4000 = 250ms

/**
 * Idea of function that can clear a little bit the code.
 * There is a major problem in doing so, code is clearer but
 * i have to put all the correct variables in the correct function.
 * 
 * If there are some variables that comunates from function to function
 * i'm is a little complicaed, but it can be fixed using a pipe
 * so you can comunicate from process to process.
*/
void write_in_file();
void reading_from_sensor();
void empty_fifo();


int main(int argc, char *argv[]) { 
  int spi0;
  int ce0 = CE_ADXL;

  FILE *outFile, *logFile;
  char fileName[255];
  int sample = 0;
  float sampleTime = 0;
  //struct timespec starttime, stoptime;
  unsigned char isX = 0, accDataRead = 0, isFifoAligned = 0;

  sprintf(fileName, "%s%s", CSV_FILE_PATH, (argc>1) ? argv[1] : "OutData");

  int number_of_out_files = 0;
  char name[255];
  sprintf(name, "%s%d.csv", fileName, number_of_out_files);
  outFile = fopen(name, "w+");
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
  clock_t begin = clock();
  
    while (1) {  
      long rawX = 0, rawY = 0, rawZ = 0;
      unsigned char isEmptyX = 0, isEmptyY = 0, isEmptyZ = 0; 
      

      // get status
      int status = Adxl357_GetStatus(spi0);
      int dataReady = (status & ADXL357_REG_STATUS_BIT_DATA_RDY) ? 1 : 0;
      int fifoOvr = (status & ADXL357_REG_STATUS_BIT_FIFO_OVR) ? 1 : 0;
      int fifoFull = (status & ADXL357_REG_STATUS_BIT_FIFO_FULL) ? 1 : 0;
      unsigned char fifoEntries = Adxl357_GetFifoEntries(spi0);

      // fprintf(logFile, "Status=%d - DataReady=%d - FifoFull=%d - FifoOvr=%d.- FifoEntries=%d\n", status, dataReady, fifoFull, fifoOvr, fifoEntries);
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
            // printf("X=%d(%d-%d-%d)", rawX, fifoEntries, isEmptyX, isX);
            fifoEntries--;
            accDataRead++;

            // check data
            if (isEmptyX == 1 || isX == 0) {
              isFifoAligned = 0;
              accDataRead = 0;
              fprintf(logFile, "FIFO X data incorrect (isEmpty=%d / isX=%d).\n", isEmptyX, isX);
              printf("FIFO X data incorrect (isEmpty=%d / isX=%d).\n", isEmptyX, isX);
              return 0;


            }

          } else if (accDataRead == 1) {
            Adxl357_GetRawAccelFromFifo(spi0, &rawY, &isEmptyY, &isX);
            // printf(" / Y=%d(%d-%d-%d)", rawY, fifoEntries, isEmptyY, isX);
            fifoEntries--;
            accDataRead++;

            // check data
            if (isEmptyY == 1 || isX == 1) {
              isFifoAligned = 0;
              accDataRead = 0;
              fprintf(logFile, "FIFO Y data incorrect (isEmpty=%d / isX=%d).\n", isEmptyY, isX);
              printf("FIFO Y data incorrect (isEmpty=%d / isX=%d).\n", isEmptyY, isX);
              return 0;

            
            }

          } else if (accDataRead == 2) {
            Adxl357_GetRawAccelFromFifo(spi0, &rawZ, &isEmptyZ, &isX);
            // printf(" / Z=%d(%d-%d-%d)\n", rawZ, fifoEntries, isEmptyZ, isX);
            fifoEntries--;
            accDataRead = 0;
            
            // check data
            if (isEmptyZ == 1 || isX == 1) {
              isFifoAligned = 0;
              // accDataRead = 0;
              fprintf(logFile, "FIFO Z data incorrect (isEmpty=%d / isX=%d).\n", isEmptyZ, isX);
              printf("FIFO Z data incorrect (isEmpty=%d / isX=%d).\n", isEmptyZ, isX);
              return 0;

            
            } else {
              float accX = 0, accY = 0, accZ = 0;
              // write in file
              accX = Adxl357_ConvertAccelData(rawX, ADXL357_RANGE_40G);
              accY = Adxl357_ConvertAccelData(rawY, ADXL357_RANGE_40G);
              accZ = Adxl357_ConvertAccelData(rawZ, ADXL357_RANGE_40G);
              fprintf(outFile, "%d; %f; %f;%f;%f;\n", sample, sampleTime, accX, accY, accZ);
              //printf("Data converted: X=%f / Y=%f / Z=%f.\n", accX, accY, accZ);
              sample++;
              sampleTime += SamplingInterval;


              //Open another file after 30 sec
              long time_passed = (clock() - begin) / CLOCKS_PER_SEC;
              if (time_passed >= 30){
                begin = clock();
                fclose(outFile);
                printf("time passed = %d\n", time_passed);
                sprintf(name, "%s%d.csv", fileName, ++number_of_out_files);
                outFile = fopen(name, "w+");
              }


            }
          }
        }

      }

    ciclo++;

    // clock_gettime(CLOCK_MONOTONIC, &stoptime);
    // double time_spent = ((stoptime.tv_sec - starttime.tv_sec)*1e9 + (stoptime.tv_nsec - starttime.tv_nsec)) / 1e6;
    // fprintf(errFile, "%d - Time spent = %d.\n", cycle, time_spent);
    }

    exit(0);
  
  




  fclose(outFile);
  fclose(logFile);
  
  return 0;
}


