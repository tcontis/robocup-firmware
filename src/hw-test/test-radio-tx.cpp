#include <cmsis_os.h>
#include <memory>
#include "Mbed.hpp"

#include "Decawave.hpp"
#include "Logger.hpp"
#include "SharedSPI.hpp"
#include "pins-control.hpp"

// #define RJ_WATCHDOG_TIMER_VALUE 2  // seconds

using namespace std;

bool initRadio() {
    // setup SPI bus
    shared_ptr<SharedSPI> sharedSPI =
        make_shared<SharedSPI>(RJ_SPI_MOSI, RJ_SPI_MISO, RJ_SPI_SCK);
    sharedSPI->format(8, 0);  // 8 bits per transfer

    // Startup the CommModule interface
    CommModule::Instance = make_shared<CommModule>();

    // Construct an object pointer for the radio
    globalRadio = std::make_unique<Decawave>(sharedSPI, RJ_RADIO_nCS,
                                             RJ_RADIO_INT, RJ_RADIO_nRESET);

    return globalRadio->isConnected();
}

int main() {
    // set baud rate to higher value than the default for faster terminal
    Serial s(RJ_SERIAL_RXTX);
    s.baud(57600);

    // Set the default logging configurations
    isLogging = RJ_LOGGING_EN;
    rjLogLevel = INFO;

    printf("****************************************\r\n");
    LOG(INFO, "Base station starting...");

    if (initRadio()) {
        for (rtp::PortType port : {rtp::PortType::CONTROL, rtp::PortType::PING}) {
            CommModule::Instance->setTxHandler(
                dynamic_cast<CommLink*>(globalRadio.get()),
                &CommLink::sendPacket, port);
        }
    } else {
        LOG(SEVERE, "No radio interface found!");
    }

    globalRadio->setAddress(rtp::ROBOT_ADDRESS);

    DigitalOut radioStatusLed(LED4, globalRadio->isConnected());


    // buffer to read data from usb bulk transfers into
    auto buf = std::array<uint8_t, 200>{};
    auto bufSize = uint32_t{};

    while (true) {
        for (int i = 0; i < 6; i++) {
            // construct packet from buffer received over USB
            rtp::Packet pkt(buf);

            // send to all robots
            // pkt.header.address = rtp::ROBOT_ADDRESS;
            pkt.header.port = rtp::PortType::CONTROL;
            pkt.header.type = rtp::MessageType::CONTROL;
            pkt.header.address = rtp::BASE_STATION_ADDRESS;

            rtp::RobotStatusMessage reply;
            reply.uid = i;

            vector<uint8_t> replyBuf;
            rtp::serializeToVector(reply, &replyBuf);
            pkt.payload = std::move(replyBuf);

            // transmit!
            CommModule::Instance->send(std::move(pkt));
            Thread::wait(1);
        }
    }
}
