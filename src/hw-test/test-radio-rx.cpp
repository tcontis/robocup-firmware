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

    // RX/TX leds
    // auto rxTimeoutLED = make_shared<FlashingTimeoutLED>(LED1);
    // auto txTimeoutLED = make_shared<FlashingTimeoutLED>(LED2);

    // Startup the CommModule interface
    CommModule::Instance = make_shared<CommModule>();

    // Construct an object pointer for the radio
    globalRadio = std::make_unique<Decawave>(sharedSPI, RJ_RADIO_nCS,
                                             RJ_RADIO_INT, RJ_RADIO_nRESET);

    return globalRadio->isConnected();
}

int recieved = 0;
void radioPingHandler(rtp::Packet pkt) {
    LOG(DEBUG, "radioRxHandler()");
    recieved++;
    printf("Recv: %d\r\n\033[1A", recieved);

    if (pkt.macInfo.ackRequest) {
        rtp::Packet pkt2;
        pkt2.macInfo.destAddr = 0x0001;
        pkt2.macInfo.seqNum = pkt.macInfo.seqNum;
        pkt2.header.type = rtp::MessageType::PING;
        pkt2.empty = false;
        CommModule::Instance->send(std::move(pkt2));
    }
}

int main() {
    Serial s(RJ_SERIAL_RXTX);
    s.baud(57600);

    // Set the default logging configurations
    isLogging = RJ_LOGGING_EN;
    rjLogLevel = INFO;

    printf("****************************************\r\n");
    LOG(INFO, "Radio recieve test starting");

    if (initRadio()) {
        CommModule::Instance->setRxHandler(&radioPingHandler, rtp::PING);
        CommModule::Instance->setTxHandler(dynamic_cast<CommLink*>(globalRadio.get()), &CommLink::sendPacket, rtp::PING);
    } else {
        LOG(SEVERE, "No radio interface found!");
    }

    globalRadio->setAddress(2);

    DigitalOut radioStatusLed(LED4, globalRadio->isConnected());

    while (true) {
        Thread::wait(1000);
    }
}