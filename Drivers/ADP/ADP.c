#include "ADP.h"

//----------------------------------------------------------------------------------------------------------------------

void setDeviceMode(SPI_HandleTypeDef hspi,DeviceStates state,struct CsPin csPin)
{
    uint16_t registerValue=0;
    switch (state) {
        case STANDBY:
            registerValue=0x00;
            break;
        case PROGRAM:
            registerValue=0x01;
            break;
        case NORMAL:
            registerValue=0x02;
            break;
    }
    writeToRegister(MODE_REGISTER,registerValue,hspi,csPin);
}

//----------------------------------------------------------------------------------------------------------------------

void writeToRegister(const uint8_t registerAddress,const uint16_t data, SPI_HandleTypeDef hspi, struct CsPin csPin)
{
    struct SpiMessage spiMessage={};
    // address 7bits ,1bit W/R =1
    spiMessage.registerAddress.addressValue=registerAddress;
    spiMessage.registerAddress.writeReadBit=0x01;
    spiMessage.data=data;
    HAL_GPIO_WritePin(csPin.gpio, csPin.gpioPinMask, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi, (uint8_t *)(&spiMessage), 3, 100);
    HAL_GPIO_WritePin(csPin.gpio,csPin.gpioPinMask,GPIO_PIN_SET);
}

//----------------------------------------------------------------------------------------------------------------------

uint16_t readRegisterData(const uint8_t registerAddress, SPI_HandleTypeDef hspi, struct CsPin csPin)
{

    uint8_t rxData[3]={0,0,0};
    struct SpiMessage spiMessage={};
    // address 7bits ,1bit W/R =0
    spiMessage.registerAddress.addressValue=registerAddress;
    spiMessage.registerAddress.writeReadBit=0x00;
    HAL_GPIO_WritePin(csPin.gpio, csPin.gpioPinMask, GPIO_PIN_RESET);
    // register size is always 2 bytes + 1 Byte for Address
    HAL_SPI_TransmitReceive(&hspi,(uint8_t *)(&spiMessage),rxData,3,100);
    HAL_GPIO_WritePin(csPin.gpio,csPin.gpioPinMask,GPIO_PIN_SET);

    return  ((rxData[1]<<8) +rxData[2]);
}

//----------------------------------------------------------------------------------------------------------------------

HAL_StatusTypeDef  smokeSensorADPD188Detect(SPI_HandleTypeDef hspi,struct CsPin csPin)
{
    // check for ID
    union DeviceIdRegister deviceIdRegister={0,0};
    deviceIdRegister.raw= readRegisterData(DEVICE_ID_REGISTER,hspi,csPin);
    if(deviceIdRegister.bytes.deviceID==DEFAULT_DEVICE_ID)
    {
        HAL_Delay(1);
        return HAL_OK;
    }
    return HAL_ERROR;
}

//----------------------------------------------------------------------------------------------------------------------

uint16_t mode=0;
void smokeSensorADPD188Init(SPI_HandleTypeDef hspi,struct CsPin csPin)
{
     union SampleClockRegister sampleClockRegister={0x12,0,1,0,0x26};
    writeToRegister(SAMPLE_CLK_REGISTER,sampleClockRegister.raw,hspi,csPin);
    setDeviceMode(hspi,PROGRAM,csPin);
    mode= readRegisterData(MODE_REGISTER,hspi,csPin);
    HAL_Delay(1);
}
