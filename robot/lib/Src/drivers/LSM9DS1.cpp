#include "drivers/LSM9DS1.hpp"

LSM9DS1::LSM9DS1(std::shared_ptr<SPI> spi_bus, PinName _csAG, PinName _csM) : spi_bus(spi_bus), csAG(_csAG), csM(_csM) {}


unsigned int LSM9DS1::WriteRegAG( uint8_t WriteAddr, uint8_t WriteData)
{
    selectAG();
    spi_bus->transmit(WriteAddr);
    unsigned int temp_val = spi_bus->transmitReceive(WriteData);
    deselectAG();
    HAL_Delay(1);
    // wait_us(5);
    return temp_val;
}

unsigned int LSM9DS1::WriteRegM( uint8_t WriteAddr, uint8_t WriteData)
{
    selectM();
    spi_bus->transmit(WriteAddr);
    unsigned int temp_val= spi_bus->transmitReceive(WriteData);
    deselectM();
    HAL_Delay(1);
    // wait_us(5);
    return temp_val;
}

unsigned int LSM9DS1::ReadRegAG( uint8_t WriteAddr, uint8_t WriteData)
{
    return WriteRegAG(WriteAddr | READ_FLAG, WriteData);
}

unsigned int LSM9DS1::ReadRegM( uint8_t WriteAddr, uint8_t WriteData)
{
    return WriteRegM(WriteAddr | READ_FLAG, WriteData);
}


void LSM9DS1::ReadRegsAG( uint8_t ReadAddr, uint8_t *ReadBuf, unsigned int Bytes )
{
    std::vector<uint8_t> WriteBuf;
    WriteBuf.assign(Bytes, 0x00);

    selectAG();
    spi_bus->transmit(ReadAddr | READ_FLAG);
    spi_bus->transmitReceive(WriteBuf.data(), ReadBuf, Bytes);
    deselectAG();
}

void LSM9DS1::ReadRegsM( uint8_t ReadAddr, uint8_t *ReadBuf, unsigned int Bytes )
{
    std::vector<uint8_t> WriteBuf;
    WriteBuf.assign(Bytes, 0x00);

    selectM();
    spi_bus->transmit(ReadAddr | READ_FLAG);
    spi_bus->transmitReceive(WriteBuf.data(), ReadBuf, Bytes);
    deselectM();
}

#define LSM_InitRegNumAG 10
#define LSM_InitRegNumM 6

/*--------------------------------------INIT-----------------------------------------------
Possible values for BW / FS / ODR:

Gyroscope full-scale setting:
FS_G_245DPS
FS_G_500DPS
FS_G_2000DPS

Gyroscope output data rate setting:
ODR_G_15HZ
ODR_G_60HZ
ODR_G_119HZ
ODR_G_238HZ
ODR_G_476HZ
ODR_G_952HZ

Gyro bandwidth dependant on ODR, these freq's are correct for ODR=476Hz or 952Hz. Check datasheet for other ODR's
BW_G_33HZ
BW_G_40HZ
BW_G_58HZ
BW_G_100HZ

Accelerometer full-scale setting:
FS_A_2G
FS_A_4G
FS_A_8G
FS_A_16G

Magnetometer full-scale setting:
FS_M_4GAUSS
FS_M_8GAUSS
FS_M_12GAUSS
FS_M_16GAUSS


Set sensitivity according to full-scale settings. Possible values are:
ACC_SENS_2G
ACC_SENS_4G
ACC_SENS_8G
ACC_SENS_16G
GYR_SENS_245DPS
GYR_SENS_500DPS
GYR_SENS_2000DPS
MAG_SENS_4GAUSS
MAG_SENS_8GAUSS
MAG_SENS_12GAUSS
MAG_SENS_16GAUSS
*/


void LSM9DS1::initialize() {

    deselectAG();
    deselectM();

    uint8_t LSM_Init_Data[LSM_InitRegNumAG][2] = {
        {ODR_G_952HZ | FS_G_500DPS | BW_G_100HZ, CTRL_REG1_G},
        {0x00, CTRL_REG2_G}, // enable FIFO, disable HPF/LPF2 <-- something seems to be wrong with the filters, check datasheet if you want to use them
        {0x09, CTRL_REG3_G}, // Disable HPF
        {0x00, ORIENT_CFG_G},
        {0x38, CTRL_REG4},      //Enable gyro outputs, no latched interrupt
        {0x38, CTRL_REG5_XL}, //No decimation of accel data, Enable accelerometer outputs
        {0xC0 | FS_A_2G, CTRL_REG6_XL}, // Accel ODR  952Hz, BW 408 Hz, +-2g
        {0xC0, CTRL_REG7_XL}, //LP cutoff ODR/9, LPF enabled, HPF bypassed, bypass LPF = 0xC0
        {0x16, CTRL_REG9},      //Temperature FIFO enabled, I2C disabled, FIFO enabled
        {0xC0, FIFO_CTRL},      //Continuous mode, overwrite if FIFO is full
    };

    uint8_t LSM_Init_DataM[LSM_InitRegNumM][2] = {
        {0x7C, CTRL_REG1_M}, // Ultra-high performance mode (x & y axis), ODR 80Hz, no temp comp
        {FS_M_4GAUSS, CTRL_REG2_M}, // FS +- 4 gauss
        {0x84, CTRL_REG3_M}, // Disable I2C, enable SPI read/write, continuous-conversion mode
        {0x0C, CTRL_REG4_M}, // Ultra-high performance mode (z-axis)
        {0x00, CTRL_REG5_M}, // Fast read disabled, continuous update
        {0x02, INT_CFG_M},
    };

    // spi_bus->format(8,3);
    spi_bus->frequency(10000000);

    for(int i=0; i<LSM_InitRegNumAG; i++) {
        WriteRegAG(LSM_Init_Data[i][1], LSM_Init_Data[i][0]);
    }

    for(int i=0; i<LSM_InitRegNumM; i++) {
        WriteRegM(LSM_Init_DataM[i][1], LSM_Init_DataM[i][0]);
    }

    // Sensitivity settings
    acc_multiplier = ACC_SENS_2G;
    gyro_multiplier = GYR_SENS_500DPS;
    mag_multiplier = MAG_SENS_4GAUSS;
}

void LSM9DS1::read_acc()
{
    uint8_t response[6];
    int16_t bit_data;
    float data;
    int i;
    ReadRegsAG(OUT_X_L_XL,response,6);
    for(i=0; i<3; i++) {
        bit_data=((int16_t)response[i*2+1]<<8)|response[i*2];
        data=(float)bit_data;
        accelerometer_data[i]=data*acc_multiplier;
    }
}

void LSM9DS1::read_gyr()
{
    uint8_t response[6];
    int16_t bit_data;
    float data;
    int i;
    ReadRegsAG(OUT_X_L_G,response,6);
    for(i=0; i<3; i++) {
        bit_data=((int16_t)response[i*2+1]<<8)|response[i*2];
        data=(float)bit_data;
        gyroscope_data[i]=data*gyro_multiplier;
    }

}

float LSM9DS1::read_temp(){
    uint8_t response[2];
    int16_t bit_data;
    float data;
    ReadRegsAG(OUT_TEMP_L,response,2);

    bit_data=((int16_t)response[1]<<8)|response[0];
    data=(float)bit_data;
    data = data/16;
        return (data+25);
}

void LSM9DS1::read_mag()
{
    uint8_t response[6];
    int16_t bit_data;
    float data;
    int i;
    ReadRegsM(OUT_X_L_M,response,6);
    for(i=0; i<3; i++) {
        bit_data=((int16_t)response[i*2+1]<<8)|response[i*2];
        data=(float)bit_data;
        magnetometer_data[i]=data*mag_multiplier;
    }

}

void LSM9DS1::read_all()
{
    read_acc();
    read_gyr();
    read_mag();
}

float LSM9DS1::gyro_x() {
    return gyroscope_data[0];
}

float LSM9DS1::gyro_y() {
    return gyroscope_data[1];
}

float LSM9DS1::gyro_z() {
    return gyroscope_data[2];
}

unsigned int LSM9DS1::whoami()
{
    return ReadRegAG(WHO_AM_I_XG, 0x00);
}

unsigned int LSM9DS1::whoamiM()
{
    return ReadRegM(WHO_AM_I_M, 0x00);
}

void LSM9DS1::selectAG() {
    csAG = 0;
}

void LSM9DS1::selectM() {
    csM = 0;
}

void LSM9DS1::deselectAG() {
    csAG = 1;
}

void LSM9DS1::deselectM() {
    csM = 1;
}
