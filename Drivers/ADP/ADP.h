
#ifndef GASSENSOR_ADP_H
#define GASSENSOR_ADP_H

#include "stm32l4xx_hal.h"

//extern  SPI_HandleTypeDef hspi;
typedef enum {
    STANDBY = 0,
    PROGRAM,
    NORMAL
} DeviceStates;

struct RegisterAddress {
    uint8_t writeReadBit: 1;
    uint8_t addressValue: 7;
};

struct SpiMessage {
    struct RegisterAddress registerAddress;
    uint16_t data;
};

struct SpiMessageFifo {
    struct RegisterAddress registerAddress;
    uint8_t data[128];
};


struct CsPin {
    GPIO_TypeDef *gpio;
    uint16_t gpioPinMask;
};


union DeviceIdRegister {
    struct Bytes {
        uint8_t deviceID;
        uint8_t revision;
    } bytes;
    uint16_t raw;
};


union SampleClockRegister {
    struct Bits {
        uint8_t clock32Adjust: 6;
        uint8_t reserved: 1;
        uint8_t clockEnable: 1;
        uint8_t clock32Bypass: 1;
        uint8_t reserved1: 7;
    } bits;
    uint16_t raw;
};

union StatusRegister {
    struct StatusBits
    {
        uint8_t reserved1:5;
        uint8_t slotAInt:1;
        uint8_t slotBInt:1;
        uint8_t reserved2:1;
        uint8_t fifoSamples;
    }bits;
    uint16_t raw;
};

union SlotEnableRegister{
    struct SlotBits
    {
       uint8_t slotAEn:1;
       uint8_t reserved1:1;
       uint8_t slotAFifoMode:3;
       uint8_t slotBEn:1;
       uint8_t slotBFifoMode:3;
       uint8_t reserved2:3;
       uint8_t fifoOverRunPrevent:1;
       uint8_t rDoutMode:1;
       uint8_t reserved3:2;
    }bits;
    uint16_t raw;
};


union DataAccessCtrl
{
  struct DataAccessCtrlBits
  {
      uint8_t digitalClockEnable:1;
      uint8_t slotADataHold:1;
      uint8_t slotBDataHold:1;
      uint16_t reserved:13;
  }bits;
  uint16_t  raw;
};


union PdLedSelect
{
    struct PdLedSelectBits
    {
        uint8_t slotALedSel:2;
        uint8_t slotBLedSel:2;
        uint8_t slotAPdSel:4;
        uint8_t slotBPdSel:4;
        uint8_t reserved:4;
    }bits;
    uint16_t  raw;
};


union NumAvgRegister
{
    struct NumAvgRegisterBits
    {
        uint8_t reserved:4;
        uint8_t slotANumAvg:3;
        uint8_t reserved1:1;
        uint8_t slotBNumAvg:3;
        uint8_t reserved2:5;
    }bits;
    uint16_t raw;
};
//----------------------------------------------------------------------------------------------------------------------

static const uint8_t MODE_REGISTER = 0x10;
static const uint8_t DEVICE_ID_REGISTER = 0x08;
static const uint8_t DEFAULT_DEVICE_ID = 0x16;
static const uint8_t SAMPLE_CLK_REGISTER = 0x4B;
static const uint8_t STATUS_REGISTER=0x00;
static const uint8_t SLOT_ENABLE_REGISTER=0x11;
static const uint8_t FIFO_ACCESS_REGISTER=0x60;
static const uint8_t DATA_ACCESS_CONTROL_REGISTER=0x5F;
static const uint8_t PD_LED_SEL_REGISTER=0x14;
//----------------------------------------------------------------------------------------------------------------------
void setDeviceMode(SPI_HandleTypeDef hspi, DeviceStates state, struct CsPin csPin);

HAL_StatusTypeDef writeToRegister(const uint8_t registerAddress, const uint16_t data, SPI_HandleTypeDef hspi, struct CsPin csPin);

uint16_t readRegisterData(const uint8_t registerAddress, SPI_HandleTypeDef hspi, struct CsPin csPin);

HAL_StatusTypeDef smokeSensorADPD188Detect(SPI_HandleTypeDef hspi, struct CsPin csPin);

HAL_StatusTypeDef smokeSensorADPD188Init(SPI_HandleTypeDef hspi, struct CsPin csPin);

HAL_StatusTypeDef terminateNormalOperation(SPI_HandleTypeDef hspi, struct CsPin csPin);

uint32_t readData(SPI_HandleTypeDef hspi, struct CsPin csPin);
void config(SPI_HandleTypeDef hspi, struct CsPin csPin);
void  readFifo( SPI_HandleTypeDef hspi, struct CsPin csPin );
#endif //GASSENSOR_ADP_H

