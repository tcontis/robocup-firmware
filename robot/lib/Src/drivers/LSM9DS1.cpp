#include "drivers/LSM9DS1.hpp"


LSM9DS1::LSM9DS1(std::shared_ptr<SPI> spi, PinName cs_pin)
    : _spi(spi),
      _cs(cs_pin, PullType::PullNone, PinType::PushPull, PinSpeed::Low, false){

};

void LSM9DS1::init(){

}

uint8_t LSM9DS1::read_register(uint8_t address){

}


void LSM9DS1::write_register(uint8_t address, uint8_t value){

}

void LSM9DS1::chip_select(bool active){

}