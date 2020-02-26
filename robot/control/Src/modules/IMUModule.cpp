#include "modules/IMUModule.hpp"
#include "mtrain.hpp"
#include <cmath>
#include "PinDefs.hpp"

#include "LockedStruct.hpp"

IMUModule::IMUModule(LockedStruct<SPI>& spi, LockedStruct<IMUData>& imuData)
    : GenericModule(kPeriod, "imu", kPriority),
      imu(spi, p18), imuData(imuData) {
    auto imuDataLock = imuData.unsafe_value();
    imuDataLock->isValid = false;
    imuDataLock->lastUpdate = 0;

    for (int i = 0; i < 3; i++) {
        imuDataLock->accelerations[i] = 0.0f;
        imuDataLock->omegas[i] = 0.0f;
    }
}

void IMUModule::start() {
    imu.init();

    printf("INFO: IMU initialized\r\n");
    imuData.lock()->initialized = true;
}

void IMUModule::entry(void) {
    imuData.lock()->omegas[2] = imu.gyro_z();
    imuData.lock()->isValid = true;
    imuData.lock()->lastUpdate = HAL_GetTick();

}