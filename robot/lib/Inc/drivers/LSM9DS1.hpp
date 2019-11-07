#ifndef __LSM9DS1_H__
#define __LSM9DS1_H__

#include "mbed.h"

/////////////////////////////////////////
// LSM9DS1 Accel/Gyro (XL/G) Registers //
/////////////////////////////////////////
#define ACT_THS             0x04
#define ACT_DUR             0x05
#define INT_GEN_CFG_XL      0x06
#define INT_GEN_THS_X_XL    0x07
#define INT_GEN_THS_Y_XL    0x08
#define INT_GEN_THS_Z_XL    0x09
#define INT_GEN_DUR_XL      0x0A
#define REFERENCE_G         0x0B
#define INT1_CTRL           0x0C
#define INT2_CTRL           0x0D
#define WHO_AM_I_XG         0x0F
#define CTRL_REG1_G         0x10
#define CTRL_REG2_G         0x11
#define CTRL_REG3_G         0x12
#define ORIENT_CFG_G        0x13
#define INT_GEN_SRC_G       0x14
#define OUT_TEMP_L          0x15
#define OUT_TEMP_H          0x16
#define STATUS_REG_0        0x17
#define OUT_X_L_G           0x18
#define OUT_X_H_G           0x19
#define OUT_Y_L_G           0x1A
#define OUT_Y_H_G           0x1B
#define OUT_Z_L_G           0x1C
#define OUT_Z_H_G           0x1D
#define CTRL_REG4           0x1E
#define CTRL_REG5_XL        0x1F
#define CTRL_REG6_XL        0x20
#define CTRL_REG7_XL        0x21
#define CTRL_REG8           0x22
#define CTRL_REG9           0x23
#define CTRL_REG10          0x24
#define INT_GEN_SRC_XL      0x26
#define STATUS_REG_1        0x27
#define OUT_X_L_XL          0x28
#define OUT_X_H_XL          0x29
#define OUT_Y_L_XL          0x2A
#define OUT_Y_H_XL          0x2B
#define OUT_Z_L_XL          0x2C
#define OUT_Z_H_XL          0x2D
#define FIFO_CTRL           0x2E
#define FIFO_SRC            0x2F
#define INT_GEN_CFG_G       0x30
#define INT_GEN_THS_XH_G    0x31
#define INT_GEN_THS_XL_G    0x32
#define INT_GEN_THS_YH_G    0x33
#define INT_GEN_THS_YL_G    0x34
#define INT_GEN_THS_ZH_G    0x35
#define INT_GEN_THS_ZL_G    0x36
#define INT_GEN_DUR_G       0x37

///////////////////////////////
// LSM9DS1 Magneto Registers //
///////////////////////////////
#define OFFSET_X_REG_L_M    0x05
#define OFFSET_X_REG_H_M    0x06
#define OFFSET_Y_REG_L_M    0x07
#define OFFSET_Y_REG_H_M    0x08
#define OFFSET_Z_REG_L_M    0x09
#define OFFSET_Z_REG_H_M    0x0A
#define WHO_AM_I_M          0x0F
#define CTRL_REG1_M         0x20
#define CTRL_REG2_M         0x21
#define CTRL_REG3_M         0x22
#define CTRL_REG4_M         0x23
#define CTRL_REG5_M         0x24
#define STATUS_REG_M        0x27
#define OUT_X_L_M           0x28
#define OUT_X_H_M           0x29
#define OUT_Y_L_M           0x2A
#define OUT_Y_H_M           0x2B
#define OUT_Z_L_M           0x2C
#define OUT_Z_H_M           0x2D
#define INT_CFG_M           0x30
#define INT_SRC_M           0x30
#define INT_THS_L_M         0x32
#define INT_THS_H_M         0x33

////////////////////////////////
// LSM9DS1 WHO_AM_I Responses //
////////////////////////////////
#define WHO_AM_I_AG_RSP     0x68
#define WHO_AM_I_M_RSP      0x3D

#define FS_G_245DPS         0x00
#define FS_G_500DPS         0x08
#define FS_G_2000DPS        0x18
#define ODR_G_15HZ          0x20
#define ODR_G_60HZ          0x40
#define ODR_G_119HZ         0x60
#define ODR_G_238HZ         0x80
#define ODR_G_476HZ         0xA0
#define ODR_G_952HZ         0xC0
#define BW_G_33HZ           0x00    // Bandwidth dependant on ODR, these freq's are correct for ODR=476Hz or 952Hz. Check datasheet for other ODR's
#define BW_G_40HZ           0x01
#define BW_G_58HZ           0x02
#define BW_G_100HZ          0x03
#define FS_A_2G             0x00
#define FS_A_4G             0x10
#define FS_A_8G             0x18
#define FS_A_16G            0x08
#define FS_M_4GAUSS         0x00
#define FS_M_8GAUSS         0x20
#define FS_M_12GAUSS        0x40
#define FS_M_16GAUSS        0x60

#define ACC_SENS_2G         0.000061f
#define ACC_SENS_4G         0.000122f
#define ACC_SENS_8G         0.000244f
#define ACC_SENS_16G        0.000732f
#define GYR_SENS_245DPS     0.00875f
#define GYR_SENS_500DPS     0.0175f
#define GYR_SENS_2000DPS    0.07f
#define MAG_SENS_4GAUSS     0.00014f
#define MAG_SENS_8GAUSS     0.00029f
#define MAG_SENS_12GAUSS    0.00043f
#define MAG_SENS_16GAUSS    0.000732f

#define READ_FLAG 0x80



class LSM9DS1
{
        SPI& spi;
        DigitalOut csAG;
        DigitalOut csM;

    public:
        LSM9DS1(SPI& _spi, PinName _csAG, PinName _csM);
        unsigned int WriteRegAG(uint8_t WriteAddr, uint8_t WriteData);
        unsigned int WriteRegM(uint8_t WriteAddr, uint8_t WriteData);
        unsigned int ReadRegAG(uint8_t WriteAddr, uint8_t WriteData);
        unsigned int ReadRegM(uint8_t WriteAddr, uint8_t WriteData);
        void ReadRegsAG(uint8_t ReadAddr, uint8_t *ReadBuf, unsigned int Bytes );
        void ReadRegsM(uint8_t ReadAddr, uint8_t *ReadBuf, unsigned int Bytes );

        void selectAG();
        void selectM();
        void deselectAG();
        void deselectM();

        void init();
        unsigned int whoami();
        unsigned int whoamiM();
        float read_temp();      // reads temperature in degrees C
        void read_acc();        // reads accelerometer in G
        void read_gyr();        // reads gyroscope in DPS
        void read_mag();        // reads magnetometer in gauss
        void read_all();        // reads acc / gyro / magneto

        float acc_multiplier;
        float gyro_multiplier;
        float mag_multiplier;
        float accelerometer_data[3];
        float gyroscope_data[3];
        float magnetometer_data[3];

};

#endif
