
#ifndef GASSENSOR_ADP_H
#define GASSENSOR_ADP_H

#include "stm32l4xx_hal.h"
//extern  SPI_HandleTypeDef hspi;
typedef enum
{
    STANDBY=0,
    PROGRAM,
    NORMAL
}DeviceStates;

struct RegisterAddress
{
    uint8_t writeReadBit:1;
    uint8_t addressValue:7;
};

struct SpiMessage
{
    struct RegisterAddress registerAddress;
    uint16_t data;
};

struct CsPin
{
    GPIO_TypeDef *gpio;
    uint16_t gpioPinMask;
};
union DeviceIdRegister
{
    struct Bytes
    {
        uint8_t deviceID;
        uint8_t revision;
    }bytes;
    uint16_t raw;
};


union SampleClockRegister
{
    struct Bits
    {
        uint8_t clock32Adjust:6;
        uint8_t reserved:1;
        uint8_t clockEnable:1;
        uint8_t clock32Bypass:1;
        uint8_t reserved1:7;
    }bits;
    uint16_t raw;
};
//----------------------------------------------------------------------------------------------------------------------

static const uint8_t MODE_REGISTER=0x10;
static const uint8_t DEVICE_ID_REGISTER=0x08;
static const uint8_t DEFAULT_DEVICE_ID=0x16;
static const uint8_t SAMPLE_CLK_REGISTER=0x4B;

//----------------------------------------------------------------------------------------------------------------------
void setDeviceMode(SPI_HandleTypeDef hspi,DeviceStates state,struct CsPin csPin);
void writeToRegister(const uint8_t registerAddress,const uint16_t data,SPI_HandleTypeDef hspi, struct CsPin csPin);
uint16_t readRegisterData(const uint8_t registerAddress, SPI_HandleTypeDef hspi, struct CsPin csPin);
HAL_StatusTypeDef  smokeSensorADPD188Detect(SPI_HandleTypeDef hspi,struct CsPin csPin);
void smokeSensorADPD188Init(SPI_HandleTypeDef hspi,struct CsPin csPin);
#endif //GASSENSOR_ADP_H

