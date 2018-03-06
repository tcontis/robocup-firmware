#include <set>

#include "CommModule.hpp"
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
    // i2c.write() return 0 on success (ack)
    MCP23017::Register regAddress = MCP23017::IODIR;
    uint16_t data = 0x00FF;
    char buffer[] = {regAddress, (char)(data & 0xff), (char)(data >> 8)};
    writeAck = !i2c.write(RJ_IO_EXPANDER_I2C_ADDRESS, buffer, sizeof(buffer));
    success = writeAck;

    regAddress = MCP23017::GPPU;
    buffer[0] = regAddress;
    writeAck = !i2c.write(RJ_IO_EXPANDER_I2C_ADDRESS, buffer, sizeof(buffer));
    success = success && writeAck;

    regAddress = MCP23017::IPOL;
    buffer[0] = regAddress;
    writeAck = !i2c.write(RJ_IO_EXPANDER_I2C_ADDRESS, buffer, sizeof(buffer));
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
    bool kickerSuccess = KickerBoard::Instance->flash(false, false);
    return kickerSuccess;
}

bool test_decawave(auto& sharedSPI) {
    globalRadio = std::make_unique<Decawave>(sharedSPI, RJ_RADIO_nCS,
                                             RJ_RADIO_INT, RJ_RADIO_nRESET);
    uint32_t check = globalRadio->selfTest();
    return check == 0;
}

int main() {
    const auto mainID = Thread::gettid();  // DO NOT DELETE RTOS WILL BREAK

    Serial pc(RJ_SERIAL_RXTX);
    pc.baud(57600);

    I2CMasterRtos i2c(RJ_I2C_SDA, RJ_I2C_SCL);
    i2c.frequency(400000);

    auto sharedSPI =
        make_shared<SharedSPI>(RJ_SPI_MOSI, RJ_SPI_MISO, RJ_SPI_SCK);
    sharedSPI->format(8, 0);  // 8 bits per transfer

    KickerBoard::Instance = std::make_shared<KickerBoard>(
        sharedSPI, RJ_KICKER_nCS, RJ_KICKER_nRESET, RJ_BALL_LED,
        "/local/rj-kickr.nib");

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
    bool kicker_connected = false;

    char input;
    pc.printf("Is the kicker connected? (y/n): ");
    while (input != 'y' && input != 'n') {
        input = pc.getc();
        if (input == 'y') {
            pc.printf("y\n\r");
            kicker_connected = true;
        } else if (input == 'n') {
            pc.printf("n\n\r");
        }
    }

    pc.printf("========= STARTING TESTS =========\r\n\r\n");

    pc.printf("-- Testing IMU at 400kHz --\r\n");
    MPU_working = mpu.testConnection();
    pc.printf("   Test %s\n\n\n\r", (MPU_working ? "Succeeded" : "Failed"));

    pc.printf("-- Testing IO Expander --\n\r");
    io_expander_working = test_io_expander(i2c);
    pc.printf("   Test %s\n\n\n\r",
              (io_expander_working ? "Succeeded" : "Failed"));

    std::set<int> rotarySet;
    if (io_expander_working) {
        char returnedVal;
        pc.printf("-- Testing Rotary Knob --\n\r");

        pc.printf("Rotate the rotary knob fully\n\r");

        // pc.printf("Expected value of rotary knob in hex (0-F): ");
        // while (!((input >= '0' && input <= '9') ||
        //          (input >= 'a' && input <= 'f') ||
        //          (input >= 'A' && input <= 'F'))) {
        //     input = pc.getc();
        //     pc.printf("%c\n\r", input);
        // }

        returnedVal = test_rotaryKnob(io_expander);
        if (input >= '0' && input <= '9')
            input -= '0';
        else if (input >= 'a' && input <= 'f')
            input = input - 'a' + 10;
        else if (input >= 'A' && input <= 'F')
            input = input - 'A' + 10;

        pc.printf("   Test %s\n\n\n\r",
                  (returnedVal == input ? "Succeeded" : "Failed"));

        pc.printf("-- Testing DIP Switch --\n\r");

        input = 'a';  // In case value from rotary knob is 0-7
        pc.printf(
            "Expected value of DIP switch as interpreted as binary (0-7): ");
        while (!(input >= '0' && input <= '7')) {
            input = pc.getc();
            pc.printf("%c\n\r", input);
        }

        returnedVal = test_dipSwitch(io_expander);
        input -= '0';
        pc.printf("   Test %s\n\n\n\r",
                  (returnedVal == input ? "Succeeded" : "Failed"));
    } else {
        pc.printf(
            "Since IO Expander isn't working, skipping rotary knob and DIP "
            "switch checks\n\n\r");
    }

    if (kicker_connected) {
        pc.printf("-- Testing kicker ATTiny -- \n\r");
        attiny_working = test_attiny(sharedSPI);
        pc.printf("   Test %s\n\n\n\r",
                  (attiny_working ? "Succeeded" : "Failed"));
    }

    pc.printf("-- Testing FPGA -- \n\r");
    fpga_working = test_FPGA(sharedSPI);
    pc.printf("   Test %s\n\n\n\r", (fpga_working ? "Succeeded" : "Failed"));

    pc.printf("-- Testing Decawave -- \n\r");
    decawave_working = test_decawave(sharedSPI);
    pc.printf("   Test %s\n\n\n\r", (decawave_working ? "Succeeded" : "Failed"));

    pc.printf("========= END OF TESTS =========\r\n\r\n");

    while (true) {
    }
}
