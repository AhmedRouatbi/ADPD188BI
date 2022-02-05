#include <stdbool.h>
#include <string.h>

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
        default: 0x00;break;
    }
    writeToRegister(MODE_REGISTER,registerValue,hspi,csPin);
}

//----------------------------------------------------------------------------------------------------------------------
uint16_t dataRead=0;
HAL_StatusTypeDef writeToRegister(const uint8_t registerAddress,const uint16_t data, SPI_HandleTypeDef hspi, struct CsPin csPin)
{
    struct SpiMessage spiMessage={};
    // address 7bits ,1bit W/R =1
    spiMessage.registerAddress.addressValue=registerAddress;
    spiMessage.registerAddress.writeReadBit=0x01;
    spiMessage.data=data;
    uint8_t txData[3];
    txData[0]=spiMessage.registerAddress.addressValue<<1 |spiMessage.registerAddress.writeReadBit;
    txData[1]=(data>>8)&0xFF;
    txData[2]=data&0xFF;
    HAL_GPIO_WritePin(csPin.gpio, csPin.gpioPinMask, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi, txData, 3, 100);
    HAL_GPIO_WritePin(csPin.gpio,csPin.gpioPinMask,GPIO_PIN_SET);
    dataRead= readRegisterData(registerAddress,hspi,csPin);
    if(dataRead==data)
    {
        return HAL_OK;
    }
    return HAL_ERROR;
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
        return HAL_OK;
    }
    return HAL_ERROR;
}

//----------------------------------------------------------------------------------------------------------------------
uint16_t mode=0;
HAL_StatusTypeDef smokeSensorADPD188Init(SPI_HandleTypeDef hspi,struct CsPin csPin)
{
    // run  recommended start-up sequence defined in the DS P 24
//    writeToRegister(SAMPLE_CLK_REGISTER,0x2610,hspi,csPin);
//    mode=readRegisterData(SAMPLE_CLK_REGISTER,hspi,csPin);

    union SampleClockRegister sampleClockRegister={0x12,0,1,0,0x26};
    // 1- enable 32K clock
    writeToRegister(SAMPLE_CLK_REGISTER,sampleClockRegister.raw,hspi,csPin);


    // 2- go to Program mode
    setDeviceMode(hspi,PROGRAM,csPin);
    config(hspi,csPin);
    HAL_Delay(1);
//    // enalbe slot A  ,slot B , and set to 16 bit for each  slot channel , read mode is set to average
//    union SlotEnableRegister slotEnableRegister={1,0,2,1,2,0,1,0};
//    writeToRegister(SLOT_ENABLE_REGISTER,slotEnableRegister.raw,hspi,csPin);
//
//    union DataAccessCtrl dataAccessCtrl={1,1,1,0};
//    writeToRegister(DATA_ACCESS_CONTROL_REGISTER,dataAccessCtrl.raw,hspi,csPin);
//    // 3-   additional configurations --> default config
//    //  clear all interrupts and clear fifo
//    union StatusRegister statusRegister={0,0,0,0,0};
//    statusRegister.raw=0x80FF;
//    config(hspi,csPin);
    // 4-go to Normal Mode
    setDeviceMode(hspi,NORMAL,csPin);
    mode= readRegisterData(MODE_REGISTER,hspi,csPin);
    if(mode==2)
    {
        return HAL_OK;
    }
    return HAL_ERROR;
}


//----------------------------------------------------------------------------------------------------------------------


HAL_StatusTypeDef terminateNormalOperation(SPI_HandleTypeDef hspi, struct CsPin csPin)
{
    // DS P25

    // 1- go to program mode
    setDeviceMode(hspi,PROGRAM,csPin);
    // 2- no config
    // 3- clear all interrupts and clear fifo
    union StatusRegister statusRegister={0,0,0,0,0};
    statusRegister.raw=0x80FF;
    writeToRegister(STATUS_REGISTER,statusRegister.raw,hspi,csPin);

    // no need to stop the 32k Clk
}

 uint32_t startTime=0;
bool isTimeout=false;
union StatusRegister statusRegister={0,0,0,0,0};

static uint8_t dataA[64]={};

static uint8_t dataB[64]={};

union FifoData
{
    struct FifoDataBytes
    {
        uint32_t SLOTA;
        uint32_t SLOTB;
    }bytes;
    uint64_t raw;
};
void  readFifo( SPI_HandleTypeDef hspi, struct CsPin csPin)
{
    uint16_t rxData[8]={};
    union FifoData fifoData={0,0};
    uint8_t j=0;
    for (uint8_t i=0;i<8;i++)
    {
        rxData[i] = readRegisterData(FIFO_ACCESS_REGISTER,hspi,csPin);
//        if(rxData[i]!=0x0000 && rxData[i]!=0xFFFF )
//        {
//            HAL_Delay(1);
//        }
    }
    memcpy(&fifoData.raw,rxData,8);



    if(fifoData.bytes.SLOTA>840)
    {
        HAL_Delay(1);
    }

    if(fifoData.bytes.SLOTB>1220)
    {
        HAL_Delay(1);
    }
//    rxData[0]= readRegisterData(0x64,hspi,csPin);
//    //    HAL_Delay(1);
//
//    rxData[1]=readRegisterData(0x70,hspi,csPin);
//    rxData[2]=readRegisterData(0x74,hspi,csPin);
//
//    rxData[3]=readRegisterData(0x78,hspi,csPin);
//    rxData[4]=readRegisterData(0x79,hspi,csPin);


    HAL_Delay(1);
    // register size is always 2 bytes + 1 Byte for Address
//    for (uint8_t i=0;i<64;i=i+2)
//    {
//        HAL_SPI_TransmitReceive(&hspi,(uint8_t *)(&spiMessage),&payload[i],2,100);
//    }


}
uint16_t  fifoData=0;
uint16_t modes=0;
uint32_t readData(SPI_HandleTypeDef hspi, struct CsPin csPin)
{

    if(!isTimeout)
    {
        startTime=HAL_GetTick();
        isTimeout=true;
//        union StatusRegister statusRegister={0,0,0,0,0};
//        statusRegister.raw=0x80FF;
//        writeToRegister(STATUS_REGISTER,statusRegister.raw,hspi,csPin);

        union DataAccessCtrl dataAccessCtrl={0,1,1,0};
        writeToRegister(DATA_ACCESS_CONTROL_REGISTER,dataAccessCtrl.raw,hspi,csPin);

    }
    if(HAL_GetTick()-startTime>=200)  // default sampling time is 200 Hz 0x28 on register x
    {

//        statusRegister.raw= readRegisterData(STATUS_REGISTER,hspi,csPin);
//        modes=readRegisterData(DATA_ACCESS_CONTROL_REGISTER,hspi,csPin);

        // read Fifo data
        isTimeout=false;
//        fifoData= readRegisterData(FIFO_ACCESS_REGISTER,hspi,csPin);
        readFifo(hspi,csPin);

        union DataAccessCtrl dataAccessCtrl={0,0,0,0};
        writeToRegister(DATA_ACCESS_CONTROL_REGISTER,dataAccessCtrl.raw,hspi,csPin);



    }
    return 0;
}


void config(SPI_HandleTypeDef hspi, struct CsPin csPin)
{
//    union PdLedSelect pdLedSelect={1,3,1,1,0};
//    writeToRegister(PD_LED_SEL_REGISTER,pdLedSelect.raw,hspi,csPin);
    // write recommanded configurations  DS P42
    while( writeToRegister(0x11,0x20A9,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x12,0x0320,hspi,csPin)!=HAL_OK); // 10 Hz  100mS
    while(writeToRegister(0x14,0x011D,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x15,0x0,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x17,0x09,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x18,0x00,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x19,0x3FFF,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x1A,0x3FFF,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x1B,0x3FFF,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x1D,0x09,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x1E,0x00,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x1F,0x3FFF,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x20,0x3FFF,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x21,0x3FFF,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x22,0x3539,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x23,0x3536,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x24,0x1530,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x25,0x630C,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x30,0x320,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x31,0x040E,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x35,0x320,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x36,0x040E,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x39,0x22F0,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x3B,0x22F0,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x3C,0x31C6,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x42,0x1C34,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x43,0xADA5,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x44,0x1C34,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x45,0xADA5,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x58,0x0544,hspi,csPin)!=HAL_OK);
    while(writeToRegister(0x54,0x0AA0,hspi,csPin)!=HAL_OK);
}