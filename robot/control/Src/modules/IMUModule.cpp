#include "modules/IMUModule.hpp"
#include "mtrain.hpp"
#include <cmath>
#include "PinDefs.hpp"


IMUModule::IMUModule(std::shared_ptr<SPI> spi, LockedStruct<IMUData>& imuData)
    : GenericModule(kPeriod, "imu", kPriority),
      imu(spi), imuData(imuData, p17) {
    auto imuDataLock = imuData.unsafe_value();
    imuDataLock->isValid = false;
    imuDataLock->lastUpdate = 0;

    for (int i = 0; i < 3; i++) {
        imuDataLock->accelerations[i] = 0.0f;
        imuDataLock->omegas[i] = 0.0f;
    }
}

void IMUModule::start() {
    imu.initialize();

    printf("INFO: IMU initialized\r\n");
    imuData.lock()->initialized = true;
}

void IMUModule::entry(void) {
    // IMU removed
    // Occasionally the MPU6050 holds the data line low
    // causing a consistent timeout on the i2c bus
    // We tried to recover the bus by clocking out a ton
    // of just clock signals, but it did not seem to solve
    // the problem
    // A new imu is on the docket
    imuDataLock->omegas[2] = imu.gyro_Z();
    imuDataLock->isValid = true;
    imuDataLock->lastUpdate = HAL_GetTick();

}