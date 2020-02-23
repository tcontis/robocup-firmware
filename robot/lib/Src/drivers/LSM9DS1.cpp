#include "drivers/LSM9DS1.hpp"
#include <math.h>


LSM9DS1::LSM9DS1(std::shared_ptr<SPI> spi, PinName cs_pin)
    : _spi(spi),
      _cs(cs_pin, PullType::PullNone, PinMode::PushPull, PinSpeed::Low, true){
    _cs.write(false);
};

void LSM9DS1::init(){
    //Set gyro full-scale selection to 2000 dps, ODR to 952 HZ (max)
    write_register(CTRL_REG1_G, gyro_odr::G_ODR_952 << 5 |  gyro_scale::G_SCALE_2000DPS << 3);

    //Disable gyroX and gyroY since unused;
    write_register(CTRL_REG4, 0x0 | (1 << 5));

    //Disable accelerometer
    write_register(CTRL_REG6_XL, 0x0);

}

float LSM9DS1::gyro_z(){
    uint16_t gyro_z_lo = read_register(OUT_Z_L_G);
    uint16_t gyro_z_hi = read_register(OUT_Z_H_G);
    uint16_t gyro_z_bin = gyro_z_hi << 8 | gyro_z_lo; //Combine hi and lo bits into the 16 bit number

    float gyro_z_float = gyro_z_bin * 0.07f * M_PI/180.0f; // RAD/S = BIN*(DPS/LSB)*(RAD/DEG)

    return gyro_z_float;

}

uint8_t LSM9DS1::read_register(uint8_t address){
    bool cs_state = cs_active;

    chip_select(true);  //Pull CS pin Low
    const uint8_t data_tx[2] = {address | READ_BIT, 0x0}; //Add read_bit (0x1) to start of address, no data to write
    uint8_t data_rx[2] = {0x0, 0x0}; //data to be modified

    _spi->transmitReceive(&data_tx[0], &data_rx[0], 2);
    _spi->transmitReceive(&data_tx[1], &data_rx[1], 2);


    //Switch CS back to original state
    chip_select(cs_state);

    return data_rx[1];
}


void LSM9DS1::write_register(uint8_t address, uint8_t value){
    bool cs_state = cs_active;

    chip_select(true);  //Pull CS pin Low

    const uint8_t data_tx[2] = {address, value};
    uint8_t data_rx[2] = {0x0, 0x0};

    _spi->transmitReceive(&data_tx[0], &data_rx[0], 2);
    _spi->transmitReceive(&data_tx[1], &data_rx[1], 2);

    //Switch CS back to original state
    chip_select(cs_state);
}

void LSM9DS1::chip_select(bool active){
    cs_active = active;
    _cs.write(active);
}