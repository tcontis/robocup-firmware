#include "drivers/LSM9DS1.hpp"


LSM9DS1::LSM9DS1(std::shared_ptr<SPI> spi, PinName cs_pin)
    : _spi(spi),
      _cs(cs_pin, PullType::PullNone, PinType::PushPull, PinSpeed::Low, true){
    cs_pin.write(false);
};

void LSM9DS1::init(){

}

float LSM9DS1::gyro_z(){
    uint16_t gyro_z_lo = read_register(OUT_Z_L_G)[0];
    uint16_t gyro_z_hi = read_register(OUT_Z_H_G)[0];
    uint16_t gyro_z_bin = gyro_z_hi << 8 | gyro_z_lo; //Combine hi and lo bits into the 16 bit number

    return gyro_z_bin; //Implicit conversion to float

}

uint8_t LSM9DS1::read_register(uint8_t address){

    chip_select(true);  //Pull CS pin Low
    const uint8_t data_tx[2] = {address | READ_BIT, 0x0}; //Add read_bit (0x1) to start of address, no data to write
    uint8_t data_rx[2] = {0x0, 0x0}; //data to be modified

    _spi->transmitReceive(&data_tx, &data_rx, 2);
    chip_select(false);
}


void LSM9DS1::write_register(uint8_t address, uint8_t value){
    chip_select(true);  //Pull CS pin Low

    const uint8_t data_tx[2] = {address, value};
    uint8_t data_rx[2] = {0x0, 0x0};

    _spi->transmitReceive(&data_tx, &data_rx, 2);

    chip_select(false);
}

void LSM9DS1::chip_select(bool active){
    cs_pin.write(active);
}