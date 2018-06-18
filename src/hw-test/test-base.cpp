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
    LOG(INFO, "Radio base test starting");

    if (initRadio()) {
        CommModule::Instance->setRxHandler(&radioRxHandler, rtp::ROBOT_STATUS);
        CommModule::Instance->setTxHandler(dynamic_cast<CommLink*>(globalRadio.get()), &CommLink::sendPacket, rtp::CONTROL);

    } else {
        LOG(SEVERE, "No radio interface found!");
    }

    // static_cast<Decawave*>(globalRadio.get())->setAddress(rtp::ROBOT_PAN, 0x0001);
    globalRadio->setAddress(0, rtp::BASE_PAN);

    DigitalOut radioStatusLed(LED4, globalRadio->isConnected());

    int sent = 0;

    while (true) {
        // construct packet from buffer received over USB
        rtp::Packet pkt;
        pkt.macInfo.destAddr = 0xFF01;
        pkt.macInfo.destPAN = rtp::ROBOT_PAN;
        pkt.macInfo.ackRequest = 0;
        pkt.macInfo.seqNum = sent;
        pkt.header.type = rtp::MessageType::CONTROL;
        pkt.empty = false;

        rtp::ControlMessage message;
        message.bodyX = 0;
        message.bodyY = 0;
        message.bodyW = 0;
        message.dribbler = 0;
        message.kickStrength = 0;
        message.shootMode = 0;
        message.triggerMode = 0;

        vector<uint8_t> payload;
        rtp::serializeToVector(message, &payload);

        pkt.payload = std::move(payload);

        CommModule::Instance->send(std::move(pkt));

        sent++;
        // printf("Sent: %d\r\n", sent);
        printf("Sent: %d, Recv: %d\r\n\033[1A", sent, recv);
        if (sent>500) break;
        Thread::wait(16);
    }
}