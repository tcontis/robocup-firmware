#include "mtrain.hpp"
#include "iodefs.hpp"
#include "SPI.hpp"
#include <vector>
#include <optional>

#include "drivers/LSM9DS1.hpp"
#include "bsp.h"

int main() {
    std::shared_ptr<SPI> sharedSPI = std::make_shared<SPI>(DOT_STAR_SPI_BUS, std::nullopt, 100'000);
    LSM9DS1 lsm(sharedSPI, IMU_CS);
    lsm.initialize();

    while (true) {
        printf("Got value: %f\n", lsm.gyro_z());
    }
}
