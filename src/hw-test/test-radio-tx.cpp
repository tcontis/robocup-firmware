#include "Mbed.hpp"
#include "Rtos.hpp"
#include "Decawave.hpp"
#include "Logger.hpp"
#include "SharedSPI.hpp"
#include "pins-control.hpp"
#include "robot-config.hpp"

#include <memory>

bool initRadio() {
    // setup SPI bus
    shared_ptr<SharedSPI> sharedSPI =
        make_shared<SharedSPI>(RJ_SPI_MOSI, RJ_SPI_MISO, RJ_SPI_SCK);
    sharedSPI->format(8, 0);  // 8 bits per transfer

    // // RX/TX leds
    // auto rxTimeoutLED = make_shared<FlashingTimeoutLED>(LED1);
    // auto txTimeoutLED = make_shared<FlashingTimeoutLED>(LED2);

    // Startup the CommModule interface
    CommModule::Instance = make_shared<CommModule>();

    // Construct an object pointer for the radio
    globalRadio = std::make_unique<Decawave>(sharedSPI, RJ_RADIO_nCS,
                                             RJ_RADIO_INT, RJ_RADIO_nRESET);

    return globalRadio->isConnected();
}

int recv = 0;
void radioRxHandler(rtp::Packet pkt) {
    LOG(DEBUG, "radioRxHandler()");
    recv++;
}

int main() {
    Serial s(RJ_SERIAL_RXTX);
    s.baud(57600);

    // Set the default logging configurations
    isLogging = RJ_LOGGING_EN;
    rjLogLevel = INFO;

    printf("****************************************\r\n");
    LOG(INFO, "Radio transmit test starting");

    if (initRadio()) {
        CommModule::Instance->setRxHandler(&radioRxHandler, rtp::PING);
        CommModule::Instance->setTxHandler(dynamic_cast<CommLink*>(globalRadio.get()), &CommLink::sendPacket, rtp::PING);

    } else {
        LOG(SEVERE, "No radio interface found!");
    }

    // static_cast<Decawave*>(globalRadio.get())->setAddress(rtp::ROBOT_PAN, 0x0001);
    globalRadio->setAddress(1);

    DigitalOut radioStatusLed(LED4, globalRadio->isConnected());

    int sent = 0;

    while (true) {
        // construct packet from buffer received over USB
        rtp::Packet pkt;
        pkt.macInfo.destAddr = 0x0002;
        pkt.macInfo.ackRequest = 1;
        pkt.macInfo.seqNum = sent;
        pkt.header.type = rtp::MessageType::PING;
        pkt.empty = false;
        CommModule::Instance->send(std::move(pkt));

        sent++;
        // printf("Sent: %d\r\n", sent);
        printf("Sent: %d, Recv: %d\r\n\033[1A", sent, recv);
        // if (sent>500) break;
        Thread::wait(20);
    }
}
