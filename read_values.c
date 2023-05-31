#include "adxl357.h"

#define CE_ADXL 0


char *program_name;

int main(int argc, char *argv[]){ 
  int j;
  //long x,y,z;
  float x,y,z;
  int spi0;
  int ce0 = CE_ADXL;
  
  
  program_name = argv[0];
  while ((argc > 1) && (argv[1][0] == '-')){
    switch (argv[1][1]){
	  
    case 'h':
    default:
      printf("usage: %s \n", program_name );
      exit(1);
    }
    argv=argv + 2;
    argc=argc - 2;
  }

  Adxl357_Init(&spi0, ce0);
  Adxl357_Reset(spi0);
  Adxl357_SetFilter(spi0, 0x00, 0x00);
  Adxl357_SetAccelRange(spi0, ADXL357_RANGE_40G);
  Adxl357_SetPowerCtl(spi0, ADXL357_POWER_ALL_ON/*0x06*/);

  j=0;
  while(1){
    int status = Adxl357_GetStatus(spi0);
    Adxl357_GetScaledAccelData(spi0, &x, &y, &z);
    //printf("%d\t%10ld\t%10ld\t%10ld\n", j,x,y,z);
    printf("%d\t%d\t%f\t%f\t%f\n", j, status, x, y, z);
    j++;
  }
  
  return(0);
}
