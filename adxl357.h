#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <unistd.h>


#define ADXL357_SPISPEED_500KHZ     500000
#define ADXL357_SPISPEED_1MHZ       1000000
#define ADXL357_SPISPEED_10MHZ      10000000
#define ADXL357_SPIBITS_8           8


// Device addresses
#define ADXL357_DEF_ADD             0x1D    // when MISO is seto to 0
#define ADXL357_ALT_ADD             0x53    // when MISO is seto to 1

// Ranges
#define ADXL357_RANGE_10G           0x81
#define ADXL357_RANGE_20G           0x82
#define ADXL357_RANGE_40G           0x83

// Power modes
#define ADXL357_POWER_DRDY_OFF      0x04
#define ADXL357_POWER_TEMP_OFF      0x02
#define ADXL357_POWER_STANDBY       0x01
#define ADXL357_POWER_ALL_ON        0x00

// Register addresses
#define ADXL357_REG_DEID_AD         0x00
#define ADXL357_REG_DEID_MST        0x01
#define ADXL357_REG_PARTID          0x02
#define ADXL357_REG_REVID           0x03
#define ADXL357_REG_STATUS          0x04
#define ADXL357_REG_FIFO_ENTRIES    0x05
#define ADXL357_REG_TEMP2           0x06
#define ADXL357_REG_TEMP1           0x07
#define ADXL357_REG_XDATA3          0x08
#define ADXL357_REG_XDATA2          0x09
#define ADXL357_REG_XDATA1          0x0A
#define ADXL357_REG_YDATA3          0x0B
#define ADXL357_REG_YDATA2          0x0C
#define ADXL357_REG_YDATA1          0x0D
#define ADXL357_REG_ZDATA3          0x0E
#define ADXL357_REG_ZDATA2          0x0F
#define ADXL357_REG_ZDATA1          0x10
#define ADXL357_REG_FIFO_DATA       0x11
#define ADXL357_REG_OFFSET_X_H      0x1E
#define ADXL357_REG_OFFSET_X_L      0x1F
#define ADXL357_REG_OFFSET_Y_H      0x20
#define ADXL357_REG_OFFSET_Y_L      0x21
#define ADXL357_REG_OFFSET_Z_H      0x22
#define ADXL357_REG_OFFSET_Z_L      0x23
#define ADXL357_REG_ACT_EN          0x24
#define ADXL357_REG_ACT_THRESH_H    0x25
#define ADXL357_REG_ACT_THRESH_L    0x26
#define ADXL357_REG_ACT_COUNT       0x27
#define ADXL357_REG_FILTER          0x28
#define ADXL357_REG_FIFO_SAMPLES    0x29
#define ADXL357_REG_INT_MAP         0x2A
#define ADXL357_REG_SYNC            0x2B
#define ADXL357_REG_RANGE           0x2C
#define ADXL357_REG_POWER_CTL       0x2D
#define ADXL357_REG_SELF_TEST       0x2E
#define ADXL357_REG_RESET           0x2F


// bit [6:4] register REGISTER_FILTER
//-3dB filter corner for the first-order, high-pass filter relative to the ODR
#define ADXL357_ODR_HPF_CORNER_NONE         0x00
#define ADXL357_ODR_HPF_CORNER_24_7         0x01
#define ADXL357_ODR_HPF_CORNER_6_2084       0x02
#define ADXL357_ODR_HPF_CORNER_1_5545       0x03
#define ADXL357_ODR_HPF_CORNER_0_3862       0x04
#define ADXL357_ODR_HPF_CORNER_0_0954       0x05
#define ADXL357_ODR_HPF_CORNER_0_0238       0x06
// bit [3:0] register REGISTER_FILTER
// ODR and low-pass filter corner
#define ADXL357_ODR_LPF_4000Hz_1000Hz       0x00
#define ADXL357_ODR_LPF_2000Hz_500Hz        0x01
#define ADXL357_ODR_LPF_1000Hz_250Hz        0x02
#define ADXL357_ODR_LPF_500Hz_125Hz         0x03
#define ADXL357_ODR_LPF_250Hz_62_25Hz       0x04
#define ADXL357_ODR_LPF_125Hz_31_25Hz       0x05
#define ADXL357_ODR_LPF_62_5Hz_15_625Hz     0x06
#define ADXL357_ODR_LPF_31_25Hz_7_813Hz     0x07
#define ADXL357_ODR_LPF_15_625Hz_3_906Hz    0x08
#define ADXL357_ODR_LPF_7_813Hz_1_953Hz     0x09
#define ADXL357_ODR_LPF_3_906Hz_0_977Hz     0x0A


// Status bits
#define ADXL357_REG_STATUS_BIT_NVMBUSY      0x10	// NVM cointroller is busy with a refresh, programmin, or a built in self test (BIST)
#define ADXL357_REG_STATUS_BIT_ACTIVITY     0x08	// Activity, as defined in the ACT_THRESH_x and ACT_COUNT registers, is detected
#define ADXL357_REG_STATUS_BIT_FIFO_OVR     0x04	// FIFO has overrun, and the oldest data is lost
#define ADXL357_REG_STATUS_BIT_FIFO_FULL    0x02	// FIFO watermark is reached
#define ADXL357_REG_STATUS_BIT_DATA_RDY	    0x01	// a compete x-axis, y-axis and z-xis measurement was made and results can be read


#define OFFSET_TEMPERATURE -3                       // in the experiment qith the NI we have a 3 °C differences at 30 °C


int Adxl357_Init(int *spi, unsigned char ce, unsigned char bits, unsigned short mode, unsigned int speed);

void Adxl357_Read(int spi, unsigned char regist, unsigned char len, unsigned char * rec);
void Adxl357_Write(int spi, unsigned char addr, unsigned char len, unsigned char *rec);

void Adxl357_Reset(int spi);
unsigned char Adxl357_GetDeviceId(int spi);
unsigned char Adxl357_GetDeviceVer(int spi);
unsigned char Adxl357_GetStatus(int spi);
unsigned char Adxl357_IsDataReady(int spi);
unsigned char Adxl357_GetFifoEntries(int spi);



unsigned char Adxl357_GetRegTemp1(int spi);
unsigned char Adxl357_GetRegTemp2(int spi);
float Adxl357_ConvertTempData(int temp1, int temp2);

float Adxl357_ConvertAccelData(long value, unsigned char range);
void Adxl357_GetRawAccelFromFifo(int spi, long *x, unsigned char *isValid, unsigned char *isX);
void Adxl357_SetPowerCtl(int spi, unsigned char value);
void Adxl357_SetFilter(int spi, unsigned char hpf, unsigned char odr_lpf);
void Adxl357_SetAccelRange(int spi, unsigned char range);
unsigned char Adxl357_GetActivityCount(int spi);
void Adxl357_SetActivityCount(int spi, unsigned char count);
void Adxl357_SetActivityEnable(int spi, unsigned char x, unsigned char y, unsigned char z);
void Adxl357_SetActivityThreshold(int spi, int threshold);
void Adxl357_SetIntMap(int spi, unsigned char value);
