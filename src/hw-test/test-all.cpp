#include "Mbed.h"
#include "RobotDevices.hpp"
#include "Logger.hpp"

#define MPU6050_ADDRESS 0x68 // 7 bit address, shift for i2c
#define MPU6050_RA_WHO_AM_I 0x75

uint16_t test_io_expander(I2C& i2c) {
    char buffer[2];
    uint16_t regAddr = 0x05;
    i2c.write(RJ_IO_EXPANDER_I2C_ADDRESS, (char*)&regAddr, 1);
    i2c.read(RJ_IO_EXPANDER_I2C_ADDRESS, buffer, 2);
    return (uint16_t)(buffer[0] | (buffer[1] << 8));
}

bool test_mpu6050(I2C& i2c, uint8_t regAddr) {
    // To test the MPU, the WHO_AM_I register should contain mpu address
    char buffer;
    i2c.write((MPU6050_ADDRESS << 1), (char*)&regAddr, 1, true);
    i2c.read((MPU6050_ADDRESS << 1), &buffer, 1);
    return (buffer == (MPU6050_ADDRESS & 0xFE)); // Last bit defined to be 0
}

int main() {
    Serial pc(RJ_SERIAL_RXTX);
    I2C i2c(RJ_I2C_SDA, RJ_I2C_SCL);

    pc.baud(57600);

    pc.printf("========= STARTING TEST =========\r\n\r\n");

    pc.printf("-- Testing IO Expander at 100kHz --\r\n");
    i2c.frequency(100000);
    uint16_t data = test_io_expander(i2c);
    pc.printf("Data received is %c", data);

    pc.printf("-- Testing IO Expander at 400kHz --\r\n");
    i2c.frequency(400000);
    data = test_io_expander(i2c);
    pc.printf("SUCCESS STATEMENT HERE (NOT DONE)\r\n\n\n\n");

    pc.printf("-- Testing MPU6050 --\r\n");
    bool passedMPU = test_mpu6050(i2c, MPU6050_RA_WHO_AM_I);
    pc.printf("-- MPU 6050 Test %s --", passedMPU ? "Passed" : "Failed");

    return 0;
}
