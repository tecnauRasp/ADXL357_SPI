#include "adxl357.h"


//unsigned char spi_bits = SPI_BITS_ADXL;
//unsigned int spi_speed = SPI_SPEED_ADXL; 
//unsigned short spi_mode = SPI_MODE_0;

unsigned int spibits = 0;
unsigned int spispeed = 0;

int Adxl357_Init(int *spi, unsigned char ce, unsigned char bits, unsigned short mode, unsigned int speed) {
  unsigned char spidev[30];

  spibits = bits;
  spispeed = speed;
  
  if (ce == 1) {
    strcpy(spidev, "/dev/spidev0.1" );//spidev0.1 --> CE1
  } else{ 
    strcpy(spidev, "/dev/spidev0.0" );//spidev0.0 --> CE0
  }
  
  // *spi = open(spidev, O_RDONLY);
  *spi = open(spidev, O_RDWR);
  if (*spi < 0) {
    printf("SPI device ADXL355 error 1\n");
    close(*spi);
    return(-1);
  }

  if (ioctl(*spi, SPI_IOC_WR_MODE, &mode) < 0) {
    printf("SPI device ADXL355 error 2\n");
    close(*spi);
    return(-1);
  }

  if (ioctl(*spi, SPI_IOC_RD_MODE, &mode) < 0) {
    printf("SPI device ADXL355 error 3\n");
    close(*spi);
    return(-1);
  }
  
  if (ioctl(*spi, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
    printf("SPI device ADXL355 error 4\n");
    close(*spi);
    return(-1);
  }
  
  if (ioctl(*spi, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) {
    printf("SPI device ADXL355 error 5\n");
    close(*spi);
    return(-1);
  }
  
  if (ioctl(*spi, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
    printf("SPI device ADXL355 error 6\n");
    close(*spi);
    return(-1);
  }

  if (ioctl(*spi, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
    printf("SPI device ADXL355 error 7\n");
    close(*spi);
    return(-1);
  }

  return(0);
}

void Adxl357_Read(int spi, unsigned char addr, unsigned char len, unsigned char *rec) {
  unsigned char send[1];
  struct spi_ioc_transfer tr = {0};
  int i;
  
  send[0] = (addr << 1) | 0x01;

  tr.tx_buf = (unsigned long) send;
  tr.rx_buf = (unsigned long) rec;
  tr.len = len+1;
  tr.speed_hz = spispeed;
  tr.delay_usecs = 20/*0*/;  // 20us - delay from ChipSelect and data transmission
  tr.bits_per_word = spibits;
  tr.tx_nbits = 8;
  tr.rx_nbits = 8;
  tr.cs_change = 0;
  
  ioctl(spi, SPI_IOC_MESSAGE(1) , &tr);

  i = 0;
  while (i < len) {
    rec[i] = rec[i+1];
    i++;
  }
}

void Adxl357_Write(int spi, unsigned char addr, unsigned char len, unsigned char *rec) {
  unsigned char send[2];
  struct spi_ioc_transfer tr = {0};
  
  send[0] = (addr << 1) | 0x00;
  send[1] = *(unsigned char *)rec;
  
  tr.tx_buf = (unsigned long) send;
  tr.rx_buf = (unsigned long) NULL;
  tr.len = len+1;
  tr.speed_hz = spispeed;
  tr.delay_usecs = 20/*0*/; // 20us - delay from ChipSelect and data transmission
  tr.bits_per_word = spibits;
  tr.tx_nbits = 8;
  tr.rx_nbits = 8;
  tr.cs_change = 0;
  
  ioctl(spi, SPI_IOC_MESSAGE(1), &tr);
}

void Adxl357_Reset(int spi){
  unsigned char resetVal = 0x52;

  Adxl357_Write(spi, ADXL357_REG_RESET, 1, &resetVal);
}

unsigned char Adxl357_GetDeviceId(int spi) {
  unsigned char a[2];

  Adxl357_Read(spi, ADXL357_REG_PARTID, 1, a);

  return a[0];
}

unsigned char Adxl357_GetDeviceVer(int spi) {
  unsigned char a[2];

  Adxl357_Read(spi, ADXL357_REG_REVID, 1, a);

  return a[0];
}

unsigned char Adxl357_GetStatus(int spi) {
  unsigned char a[2];

  Adxl357_Read(spi, ADXL357_REG_STATUS, 1, a);

  return a[0];
}


unsigned char Adxl357_GetRegTemp1(int spi) {
  char a[2];

  Adxl357_Read(spi, ADXL357_REG_TEMP1, 1, a);

  return a[0];
}

unsigned char Adxl357_GetRegTemp2(int spi) {
  char a[2];

  Adxl357_Read(spi, ADXL357_REG_TEMP2, 1, a);

  return a[0];
}

// void bin(unsigned n, int bit) {
//   unsigned i;
//   for (i = 1 << bit-1; i>0; i = i/2)
//     (n & i) ? printf("1") : printf("0");
// }

float Adxl357_ConvertTempData(int temp1, int temp2) {
  // printf("\ntemp1 = %d; ", temp1);
  // bin(temp1, 8);
  // printf("\ntemp2 = %d; ", temp2);
  // bin(temp2, 4);

  int mask =  0x0F; // temp2 mask -> 0000 1111 (first 4 bits no info)
  int value = temp1 | ((temp2 & mask) << 8); //temp1 bits[7:0] temp2 bits[11:8] 
  // printf("\nvalue = %d; ", value);
  // bin(value, 12);

  return (float) (-(1/9.05) * (value - 1885)) + 25 + OFFSET_TEMPERATURE; // scale = -9.05 LSB/C° -- 1885 LSB = 25 C°
}



unsigned char Adxl357_IsDataReady(int spi) {
  unsigned char a[2];

  Adxl357_Read(spi, ADXL357_REG_STATUS, 1, a);

  return a[0] & ADXL357_REG_STATUS_BIT_DATA_RDY;
}

unsigned char Adxl357_GetFifoEntries(int spi) {
  unsigned char a[2];

  Adxl357_Read(spi, ADXL357_REG_FIFO_ENTRIES, 1, a);

  return a[0] & 0x7F;
}

// void Adxl357_GetRawAccelData(int spi, long *x, long *y, long *z) {
//   unsigned char a[10];
  
//   Adxl357_Read(spi, ADXL357_REG_XDATA3, 9, a);

//   // *x = (a[0] << 16) | (a[1] << 8) | ((a[2] & 0xF0) >> 4);
//   // *y = (a[3] << 16) | (a[4] << 8) | ((a[5] & 0xF0) >> 4);
//   // *z = (a[6] << 16) | (a[7] << 8) | ((a[8] & 0xF0) >> 4);

//   // pezzo di codice preso da un forum di Arduino
//   *x = (((long) a[0]) << 12) | (((long) a[1]) << 4) | (((long) a[2]) >> 4);
//   *y = (((long) a[3]) << 12) | (((long) a[4]) << 4) | (((long) a[5]) >> 4);
//   *z = (((long) a[6]) << 12) | (((long) a[7]) << 4) | (((long) a[8]) >> 4);
// }

float Adxl357_ConvertAccelData(long value, unsigned char range) {
  if (range == ADXL357_RANGE_10G) {
    return (float)value * 10.0 / 524288.0;     // 2^19 = 524288
  } else if (range == ADXL357_RANGE_20G) {
    return (float)value * 20.0 / 524288.0;
  } else if (range == ADXL357_RANGE_40G) {
    return (float)value * 40.0 / 524288.0;
  }
  return value;
}


void Adxl357_GetRawAccelFromFifo(int spi, long *x, unsigned char *isEmpty, unsigned char *isX) {
  unsigned char a[4];

  Adxl357_Read(spi, ADXL357_REG_FIFO_DATA, 3, a);

  *isEmpty = ((a[2] & 0x02) > 0) ? 1 : 0;
  *isX = ((a[2] & 0x01) > 0) ? 1 : 0;

  *x = (((long) a[0]) << 12) | (((long) a[1]) << 4) | (((long) a[2]) >> 4);

  // all final measurements are signed with two's complement
  // MSB check is needed for correction
  *x = (*x & 0x80000) ? ((*x & 0x7FFFF) - 0x80000) : *x;
}


// Set power modes
void Adxl357_SetPowerCtl(int spi, unsigned char value) {
  // bit 0 - standby  -> 1 = standby mode / 0 = measurement mode
  // bit 1 - temp_off -> set 1 to disable temperature processing
  // bit 2 - drdy_off -> set 1 to force the DRDY output to 0 in modes where it is normally signal data ready
  Adxl357_Write(spi, ADXL357_REG_POWER_CTL, 1, &value);
}

// Set the register for internal high-pass and low-pass filters
void Adxl357_SetFilter(int spi, unsigned char hpf, unsigned char odr_lpf) {
  unsigned char value = (hpf << 4) | odr_lpf;   // hpf is 3-bits while lpf is 4-bits
  Adxl357_Write(spi, ADXL357_REG_FILTER, 1, &value);
}

void Adxl357_SetAccelRange(int spi, unsigned char range) {
  unsigned char a[2];

  // register contains other data we don't wanna change
  Adxl357_Read(spi, ADXL357_REG_RANGE, 1, a);

  unsigned char val = (a[0] & 0xFC) | range;   // reset previous range and set new one
  Adxl357_Write(spi, ADXL357_REG_RANGE, 1, &val);
}

// read number of events above threshold required to detect activity
unsigned char Adxl357_GetActivityCount(int spi) {
  unsigned char a[2];

  Adxl357_Read(spi, ADXL357_REG_ACT_COUNT, 1, a);

  return a[0];
}

// Set the Activity Count register
void Adxl357_SetActivityCount(int spi, unsigned char count) {
  Adxl357_Write(spi, ADXL357_REG_ACT_COUNT, 1, &count);
}

// Set the components for the activity detection algorithm
void Adxl357_SetActivityEnable(int spi, unsigned char x, unsigned char y, unsigned char z) {
  unsigned char value = 0 | (z << 2) |(y << 1) | x;

  Adxl357_Write(spi, ADXL357_REG_ACT_EN, 1, &value);
}

// Set the activity threshold that needs tp be met to count an activity
void Adxl357_SetActivityThreshold(int spi, int threshold) {
  unsigned char data1 = threshold >> 8;   // split into 2 bytes
  unsigned char data2 = threshold & 0xFF;

  Adxl357_Write(spi, ADXL357_REG_ACT_THRESH_H, 1, &data1);
  Adxl357_Write(spi, ADXL357_REG_ACT_THRESH_L, 1, &data2);
}

// Set interrupt pins map them for different events
void Adxl357_SetIntMap(int spi, unsigned char value) {
  Adxl357_Write(spi, ADXL357_REG_INT_MAP, 1, &value);
}