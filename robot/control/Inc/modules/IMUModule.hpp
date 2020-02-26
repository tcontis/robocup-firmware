#pragma once

#include "SPI.hpp"
#include "GenericModule.hpp"
#include "MicroPackets.hpp" 
#include "drivers/LSM9DS1.hpp"
#include "LockedStruct.hpp"
#include <memory>

class IMUModule : public GenericModule {
public:
    // How many times per second this module should run
    static constexpr float kFrequency = 200.0f; // Hz
    static constexpr std::chrono::milliseconds kPeriod{static_cast<int>(1000 / kFrequency)};
    static constexpr int kPriority = 3;

    IMUModule(LockedStruct<SPI>& spi, LockedStruct<IMUData>& imuData);

    void start() override;

    void entry() override;

private:
    LSM9DS1 imu;
    LockedStruct<IMUData>& imuData;
};