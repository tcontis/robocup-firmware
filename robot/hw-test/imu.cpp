#include "drivers/LSM9DS1.hpp"
#include <math.h>
#include "LockedStruct.hpp"
#include "PinDefs.hpp"

int main(void)
{
    //Construct LSM9DS1 object, initialize
    //While loop w/ rtos delay
    static LockedStruct<SPI> spi(SHARED_SPI_BUS, std::nullopt, 100'000);

    LSM9DS1 lsm = LSM9DS1(spi, p18);
    LSM9DS1.init();

    while(true) {
        float z = lsm.gyro_z();
        printf("%.6", z);
        HAL_Delay(500);
    }
}
