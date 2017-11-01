#include "Decawave.hpp"
#include "FPGA.hpp"
#include "KickerBoard.hpp"
#include "Mbed.h"
#include "mpu-6050.hpp"
#include "Logger.hpp"
#include "RobotDevices.hpp"
#include "Rtos.hpp"
#include "RtosTimerHelper.hpp"

#ifdef NDEBUG
LocalFileSystem local("local");
#endif

uint16_t test_io_expander(I2CMasterRtos& i2c) {
    // Config call with inputs 0x00FF, 0x00FF, 0x00FF
    uint8_t regAddress = MCP23017::IODIR;
    uint16_t data = 0x00FF;
    char buffer[] = {regAddress, (char)(data & 0xff), (char)(data >> 8)};
    i2c.write(RJ_IO_EXPANDER_I2C_ADDRESS, buffer, sizeof(buffer));

    regAddress = MCP23017::GPPU;
    buffer[0] = regAddress;
    i2c.write(RJ_IO_EXPANDER_I2C_ADDRESS, buffer, sizeof(buffer));

    regAddress = MCP23017::IPOL;
    buffer[0] = regAddress;
    i2c.write(RJ_IO_EXPANDER_I2C_ADDRESS, buffer, sizeof(buffer));

    // Now read register
    regAddress = 0x05;
    char received[2];
    i2c.write(RJ_IO_EXPANDER_I2C_ADDRESS, (char*)&regAddress, 1);
    i2c.read(RJ_IO_EXPANDER_I2C_ADDRESS, received, 2);
    return (uint16_t)(received[0] | (received[1] << 8));
}

char test_rotaryKnob(MCP23017& io_expander) {
    char value = io_expander.readPin(MCP23017::PinA5) << 3;
    value |= io_expander.readPin(MCP23017::PinA6) << 2;
    value |= io_expander.readPin(MCP23017::PinA4) << 1;
    value |= io_expander.readPin(MCP23017::PinA7);
    return value;
}

char test_dipSwitch(MCP23017& io_expander) {
    char value = io_expander.readPin(MCP23017::PinA3) << 2;
    value |= io_expander.readPin(MCP23017::PinA2) << 1;
    value |= io_expander.readPin(MCP23017::PinA1);
    return value;
}

bool test_mpu6050(I2CMasterRtos& i2c, uint8_t regAddr) {
    // To test the MPU, the WHO_AM_I register should contain mpu address
    char buffer;
    i2c.write((MPU6050_ADDRESS << 1), (char*)&regAddr, 1, true);
    i2c.read((MPU6050_ADDRESS << 1), &buffer, 1);
    return (buffer == (MPU6050_ADDRESS & 0xFE));  // Last bit defined to be 0
}

bool test_FPGA(auto& sharedSPI) {
    FPGA Instance(sharedSPI, RJ_FPGA_nCS, RJ_FPGA_INIT_B,
                              RJ_FPGA_PROG_B, RJ_FPGA_DONE);

    const bool fpgaInitialized = Instance.configure("/local/rj-fpga.nib");

    return fpgaInitialized;
}

bool test_attiny(auto& sharedSPI) {
    //  initialize kicker board and flash it with new firmware if necessary
    KickerBoard kickerBoard(sharedSPI, RJ_KICKER_nCS, RJ_KICKER_nRESET,
                          RJ_BALL_LED, "/local/rj-kickr.nib");
    bool kickerSuccess = !kickerBoard.flash(true, true);
    return kickerSuccess;
}

bool test_decawave(auto& sharedSPI) {
    auto radio = std::make_unique<Decawave>(sharedSPI, RJ_RADIO_nCS,
                             RJ_RADIO_INT, RJ_RADIO_nRESET);
    uint32_t check = radio->selfTest();
    return check == 0;
}

int main() {
    const auto mainID = Thread::gettid(); // DO NOT DELETE RTOS WILL BREAK

    Serial pc(RJ_SERIAL_RXTX);
    I2CMasterRtos i2c(RJ_I2C_SDA, RJ_I2C_SCL);
    pc.baud(57600);

    // MPU6050 mpu(RJ_I2C_SDA, RJ_I2C_SCL, 400000);
    // bool val = mpu.testConnection();
    // pc.printf("%s", (val ? "True" : "False"));

    // Replace auto
    auto sharedSPI = make_shared<SharedSPI>(RJ_SPI_MOSI, RJ_SPI_MISO, RJ_SPI_SCK);
    sharedSPI->format(8, 0);  // 8 bits per transfer
    bool val = test_decawave(sharedSPI);
    pc.printf("%s", (val ? "True" : "False"));

    // bool val = test_attiny(spiBus);
    // pc.printf("%s", (val ? "True" : "False"));
    // MCP23017 io_expander(RJ_I2C_SDA, RJ_I2C_SCL, RJ_IO_EXPANDER_I2C_ADDRESS);
    // io_expander.config(0x00FF, 0x00FF, 0x00FF);
    // uint8_t data = io_expander.readPin(MCP23017::PinA1);
    // pc.printf("\n\rData is %d", data);

    // pc.printf("========= STARTING TEST =========\r\n\r\n");
    //
    // pc.printf("-- Testing IO Expander at 100kHz --\r\n");
    // i2c.frequency(100000);
    // uint16_t data = test_io_expander(i2c);
    // pc.printf("Data received is %d\n\n\r", data);

    // pc.printf("-- Testing IO Expander at 400kHz --\r\n");
    // i2c.frequency(400000);
    // data = test_io_expander(i2c);
    // pc.printf("SUCCESS STATEMENT HERE (NOT DONE)\r\n\n\n\n");

    // pc.printf("\r\n\n-- Testing MPU6050 --\r\n");
    // bool passedMPU = test_mpu6050(i2c, MPU6050_RA_WHO_AM_I);
    // pc.printf("-- MPU 6050 Test %s --", passedMPU ? "Passed" : "Failed");
    //
    // pc.printf("\r\n\n-- TESTING ROTARY KNOB --\r\n");
    // char val = test_rotaryKnob(io_expander);
    // pc.printf("-- Rotary Knob value read: %x", val);

}
