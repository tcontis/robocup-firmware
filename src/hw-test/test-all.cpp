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

bool test_io_expander(I2CMasterRtos& i2c) {
    bool writeAck;
    bool success;

    // Config call with inputs 0x00FF, 0x00FF, 0x00FF
    uint8_t regAddress = MCP23017::IODIR;
    uint16_t data = 0x00FF;
    char buffer[] = {regAddress, (char)(data & 0xff), (char)(data >> 8)};
    writeAck = i2c.write(RJ_IO_EXPANDER_I2C_ADDRESS, buffer, sizeof(buffer));
    success = writeAck;

    regAddress = MCP23017::GPPU;
    buffer[0] = regAddress;
    writeAck = i2c.write(RJ_IO_EXPANDER_I2C_ADDRESS, buffer, sizeof(buffer));
    success = success && writeAck;

    regAddress = MCP23017::IPOL;
    buffer[0] = regAddress;
    writeAck = i2c.write(RJ_IO_EXPANDER_I2C_ADDRESS, buffer, sizeof(buffer));
    success = success && writeAck;

    return success;
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

bool test_FPGA(auto& sharedSPI) {
    const bool fpgaInitialized =
        FPGA::Instance->configure("/local/rj-fpga.nib");

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
    const auto mainID = Thread::gettid();  // DO NOT DELETE RTOS WILL BREAK

    Serial pc(RJ_SERIAL_RXTX);
    pc.baud(57600);

    I2CMasterRtos i2c(RJ_I2C_SDA, RJ_I2C_SCL);
    i2c.frequency(400000);

    // Replace auto
    auto sharedSPI =
        make_shared<SharedSPI>(RJ_SPI_MOSI, RJ_SPI_MISO, RJ_SPI_SCK);
    sharedSPI->format(8, 0);  // 8 bits per transfer

    FPGA::Instance = new FPGA(sharedSPI, RJ_FPGA_nCS, RJ_FPGA_INIT_B,
                              RJ_FPGA_PROG_B, RJ_FPGA_DONE);

    MPU6050 mpu(RJ_I2C_SDA, RJ_I2C_SCL, 400000);

    MCP23017 io_expander(RJ_I2C_SDA, RJ_I2C_SCL, RJ_IO_EXPANDER_I2C_ADDRESS);
    io_expander.config(0x00FF, 0x00FF, 0x00FF);

    bool MPU_working = false;
    bool io_expander_working = false;
    bool decawave_working = false;
    bool fpga_working = false;
    bool attiny_working = false;
    bool kicker_connected = true;

    pc.printf("========= STARTING TESTS =========\r\n\r\n");

    pc.printf("-- Testing IMU at 400kHz --\r\n\n");
    MPU_working = mpu.testConnection();
    pc.printf("   Test %s\n\n\n\r", (MPU_working ? "Succeeded" : "Failed"));

    pc.printf("-- Testing IO Expander --\n\n\r");
    io_expander_working = test_io_expander(i2c);
    pc.printf("   Test %s\n\n\n\r",
              (io_expander_working ? "Succeeded" : "Failed"));

    if (io_expander_working) {
        test_rotaryKnob(io_expander);

        test_dipSwitch(io_expander);
    } else {
        pc.printf(
            "Since IO Expander isn't working, skipping rotary knob and dip "
            "switch checks\n\n\r");
    }

    pc.printf("-- Testing decawave -- \n\r");
    decawave_working = test_decawave(sharedSPI);
    pc.printf("   Test %s\n\n\n\r",
              (decawave_working ? "Succeeded" : "Failed"));

    pc.printf("-- Testing FPGA -- \n\r");
    fpga_working = test_FPGA(sharedSPI);
    pc.printf("   Test %s\n\n\n\r", (fpga_working ? "Succeeded" : "Failed"));

    if (kicker_connected) {
        pc.printf("-- Testing kicker ATTiny -- \n\r");
        attiny_working = test_attiny(sharedSPI);
        pc.printf("   Test %s\n\n\n\r",
                  (attiny_working ? "Succeeded" : "Failed"));
    }

    pc.printf("========= END OF TESTS =========\r\n\r\n");

    while (true) {
    }
}
