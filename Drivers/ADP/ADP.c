#include "ADP.h"
//----------------------------------------------------------------------------------------------------------------------

void setDeviceMode(SPI_HandleTypeDef hspi, DeviceStates state, struct CsPin csPin) {
    uint16_t registerValue = 0;
    switch (state) {
        case STANDBY:
            registerValue = 0x00;
            break;
        case PROGRAM:
            registerValue = 0x01;
            break;
        case NORMAL:
            registerValue = 0x02;
            break;
        default:
            0x00;
            break;
    }
    writeToRegister(MODE_REGISTER, registerValue, hspi, csPin, true);
}

//----------------------------------------------------------------------------------------------------------------------

HAL_StatusTypeDef
writeToRegister(const uint8_t registerAddress, const uint16_t data, SPI_HandleTypeDef hspi, struct CsPin csPin,
                bool isVerifyWriting) {
    uint16_t dataRead = 0;
    // address 7bits ,1bit W/R =1
    union SpiRegisterData spiRegisterData = {0, 0, 0, 0};
    spiRegisterData.dataFormat.registerAddress = registerAddress;
    spiRegisterData.dataFormat.writeReadBit = 0x01;
    spiRegisterData.dataFormat.msb = (data >> 8) & 0xFF; // MSB
    spiRegisterData.dataFormat.lsb = data & 0xFF;    // LSB

    HAL_GPIO_WritePin(csPin.gpio, csPin.gpioPinMask, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi, (uint8_t *) &spiRegisterData, 3, 100);
    HAL_GPIO_WritePin(csPin.gpio, csPin.gpioPinMask, GPIO_PIN_SET);
    if (isVerifyWriting) {
        dataRead = readRegisterData(registerAddress, hspi, csPin);
        if (dataRead == data) {
            return HAL_OK;
        }
        return HAL_ERROR;
    }
    return HAL_OK;
}

//----------------------------------------------------------------------------------------------------------------------

uint16_t readRegisterData(const uint8_t registerAddress, SPI_HandleTypeDef hspi, struct CsPin csPin) {

    uint8_t rxData[3] = {0, 0, 0};
    // address 7bits ,1bit W/R =0
    union SpiRegisterData spiRegisterData = {0, 0, 0, 0};
    spiRegisterData.dataFormat.registerAddress = registerAddress;
    spiRegisterData.dataFormat.writeReadBit = 0x00;


    HAL_GPIO_WritePin(csPin.gpio, csPin.gpioPinMask, GPIO_PIN_RESET);
    // register size is always 2 bytes + 1 Byte for Address
    HAL_SPI_TransmitReceive(&hspi, (uint8_t *) (&spiRegisterData), rxData, 3, 100);
    HAL_GPIO_WritePin(csPin.gpio, csPin.gpioPinMask, GPIO_PIN_SET);
    return ((rxData[1] << 8) + rxData[2]);
}

//----------------------------------------------------------------------------------------------------------------------

HAL_StatusTypeDef smokeSensorADPD188Detect(SPI_HandleTypeDef hspi, struct CsPin csPin) {
    // check for ID
    union DeviceIdRegister deviceIdRegister = {0, 0};
    deviceIdRegister.raw = readRegisterData(DEVICE_ID_REGISTER, hspi, csPin);
    if (deviceIdRegister.bytes.deviceID == DEFAULT_DEVICE_ID) {
        return HAL_OK;
    }
    return HAL_ERROR;
}

//----------------------------------------------------------------------------------------------------------------------

HAL_StatusTypeDef smokeSensorADPD188Init(SPI_HandleTypeDef hspi, struct CsPin csPin) {
    uint16_t mode = 0;
    // run  recommended start-up sequence defined in the DS P 24

    // 1- enable 32K clock
    union SampleClockRegister sampleClockRegister = {0x12, 0, 1, 0, 0x26};
    writeToRegister(SAMPLE_CLK_REGISTER, sampleClockRegister.raw, hspi, csPin, true);

    // 2- go to Program mode
    setDeviceMode(hspi, PROGRAM, csPin);
    // 3-   additional configurations --> default config
    // config the sensor with recommended configurations except for sampling frequency and fifo update
    config(hspi, csPin);

    // 4-go to Normal Mode
    setDeviceMode(hspi, NORMAL, csPin);
    mode = readRegisterData(MODE_REGISTER, hspi, csPin);

    if (mode == 2)  // mode =2  --> Normal mode
    {
        return HAL_OK;
    }
    return HAL_ERROR;
}


//----------------------------------------------------------------------------------------------------------------------


HAL_StatusTypeDef terminateNormalOperation(SPI_HandleTypeDef hspi, struct CsPin csPin) {
    // DS P25

    // 1- go to program mode
    setDeviceMode(hspi, PROGRAM, csPin);
    // 2- no config
    // 3- clear all interrupts and clear fifo
    union StatusRegister statusRegister = {0, 0, 0, 0, 0};
    statusRegister.raw = 0x80FF;
    writeToRegister(STATUS_REGISTER, statusRegister.raw, hspi, csPin, true);

}

//----------------------------------------------------------------------------------------------------------------------

union FifoData readFifo(SPI_HandleTypeDef hspi, struct CsPin csPin) {
    // read 64 bits of fifo  (32 bits for each slot )
    uint16_t rxData[4] = {};
    union FifoData fifoData = {0, 0};
    for (uint8_t i = 0; i < 4; i++) {
        rxData[i] = readRegisterData(FIFO_ACCESS_REGISTER, hspi, csPin);

    }
    // copy rxData into fifoData structure
    memcpy(&fifoData.raw, rxData, 8);
    return fifoData;
}

//----------------------------------------------------------------------------------------------------------------------

uint32_t startTime = 0;
bool isTimeout = false;

union FifoData readData(SPI_HandleTypeDef hspi, struct CsPin csPin) {

    union FifoData fifoData1 = {0, 0};
    // polling method   sampling frequency 100mS/ 10Hz
    if (!isTimeout) {
        // read new Ticks
        startTime = HAL_GetTick();
        isTimeout = true;
        // update Fifo with new readings
        union DataAccessCtrl dataAccessCtrl = {0, 0, 0, 0};
        writeToRegister(DATA_ACCESS_CONTROL_REGISTER, dataAccessCtrl.raw, hspi, csPin, true);

    }
    if (HAL_GetTick() - startTime >= 100)  //  sampling frequency = 10HZ  / 100mS
    {
        // if sampling  time is elapsed Hold Samples in the Fifo
        union DataAccessCtrl dataAccessCtrl = {0, 1, 1, 0};
        writeToRegister(DATA_ACCESS_CONTROL_REGISTER, dataAccessCtrl.raw, hspi, csPin, true);

        // read Fifo data

        fifoData1 = readFifo(hspi, csPin);
        isTimeout = false;
        return fifoData1;

    }
    // sampling time is not elapsed return 0 for each time slot
    fifoData1.raw = 0;
    return fifoData1;
}

//----------------------------------------------------------------------------------------------------------------------

void config(SPI_HandleTypeDef hspi, struct CsPin csPin) {

    // write recommanded configurations  DS P42
    while (writeToRegister(0x11, 0x20A9, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x12, 0x0320, hspi, csPin, true) != HAL_OK); // 10 Hz  100mS
    while (writeToRegister(0x14, 0x011D, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x15, 0x0, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x17, 0x09, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x18, 0x00, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x19, 0x3FFF, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x1A, 0x3FFF, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x1B, 0x3FFF, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x1D, 0x09, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x1E, 0x00, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x1F, 0x3FFF, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x20, 0x3FFF, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x21, 0x3FFF, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x22, 0x3539, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x23, 0x3536, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x24, 0x1530, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x25, 0x630C, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x30, 0x320, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x31, 0x040E, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x35, 0x320, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x36, 0x040E, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x39, 0x22F0, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x3B, 0x22F0, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x3C, 0x31C6, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x42, 0x1C34, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x43, 0xADA5, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x44, 0x1C34, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x45, 0xADA5, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x58, 0x0544, hspi, csPin, true) != HAL_OK);
    while (writeToRegister(0x54, 0x0AA0, hspi, csPin, true) != HAL_OK);
}