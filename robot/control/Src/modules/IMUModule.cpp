#include "modules/IMUModule.hpp"
#include "mtrain.hpp"
#include <cmath>

#include "iodefs.h"
#include "PinDefs.hpp"

IMUModule::IMUModule(std::shared_ptr<SPI> sharedSPI, IMUData * imuData)
    : imu(sharedSPI, IMU_CS, IMU_CS2), imuData(imuData) {

    imu.initialize();

    printf("INFO: IMU initialized\r\n");

    imuData->isValid = false;
    imuData->lastUpdate = 0;

    for (int i = 0; i < 3; i++) {
        imuData->accelerations[i] = 0.0f;
        imuData->omegas[i] = 0.0f;
    }
}

void IMUModule::entry(void) {
    printf("INFO: IMU entry\r\n");
    //TODO: Fix how ineffecient this is with data passing
    imu.read_gyr();

    imuData->omegas[0] = imu.gyro_x();
    imuData->omegas[1] = imu.gyro_y();
    imuData->omegas[2] = imu.gyro_z();

    imuData->isValid = true;
    imuData->lastUpdate = HAL_GetTick();
    printf("INFO: Gryo Data %d\r\n", (int) imuData->omegas[2]);
}
