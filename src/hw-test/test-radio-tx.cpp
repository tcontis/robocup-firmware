// #include "Mbed.hpp"
// #include "Rtos.hpp"
// #include "Decawave.hpp"
// #include "Logger.hpp"
// #include "SharedSPI.hpp"
#include "pins-control.hpp"
// #include "robot-config.hpp"

#include <cmsis_os.h>
#include <memory>
#include "Mbed.hpp"

#include "Decawave.hpp"
#include "Logger.hpp"
#include "Logger.hpp"
#include "SharedSPI.hpp"

// #include <pb_encode.h>
// #include <pb_decode.h>
// #include "Packet.pb.h"

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

// int recv = 0;
// void radioRxHandler(rtp::SubPacket pkt) {
//     LOG(DEBUG, "radioRxHandler()");
//     recv++;
// }

int main() {
    Serial s(RJ_SERIAL_RXTX);
    s.baud(57600);

    // Set the default logging configurations
    isLogging = RJ_LOGGING_EN;
    rjLogLevel = INFO;

    printf("****************************************\r\n");
    LOG(INFO, "Radio transmit test starting");


    if (initRadio()) {
        // CommModule::Instance->setRxHandler(&radioRxHandler, rtp::PING);
        // CommModule::Instance->setTxHandler(dynamic_cast<CommLink*>(globalRadio.get()), &CommLink::sendPacket);
        //
        // CommModule::Instance->setRxHandler(this, &RadioProtocol::rxHandler, rtp::PortType::CONTROL);
        // CommModule::Instance->setTxHandler(globalRadio.get(), &CommLink::sendPacket, rtp::PortType::CONTROL);
        CommModule::Instance->setTxHandler(dynamic_cast<CommLink*>(globalRadio.get()),&CommLink::sendPacket, rtp::PortType::CONTROL);

    } else {
        LOG(SEVERE, "No radio interface found!");
    }

    // static_cast<Decawave*>(globalRadio.get())->setAddress(rtp::ROBOT_PAN, 0x0001);
    // globalRadio->setAddress(1);
    globalRadio->setAddress(rtp::BASE_STATION_ADDRESS);

    DigitalOut radioStatusLed(LED4, globalRadio->isConnected());

    // int sent = 0;

    while (true) {
        printf("Sent?\r\n");
        rtp::Packet pkt;
        pkt.header.port = rtp::PortType::CONTROL;
        pkt.header.type = rtp::MessageType::CONTROL;
        pkt.header.address = rtp::BASE_STATION_ADDRESS;

        rtp::RobotStatusMessage reply;
        reply.uid = 0;

        std::vector<uint8_t> replyBuf;
        rtp::serializeToVector(reply, &replyBuf);

        pkt.payload = std::move(replyBuf);

        CommModule::Instance->send(std::move(pkt));

        // construct packet from buffer received over USB
        // rtp::Packet pkt;
        // pkt.macInfo.destAddr = 0x0002;
        // pkt.macInfo.ackRequest = 1;
        // pkt.macInfo.seqNum = sent;
        // pkt.empty = false;
        // rtp::SubPacket subPacket;
        // subPacket.header.type = rtp::MessageType::PING;
        // vector<uint8_t> vect(500, 0);
        // subPacket.payload = std::move(vect);
        // // printf("size: %d\r\n", subPacket.payload.size());
        // subPacket.empty = false;
        // pkt.subPackets.push_back(subPacket);
        // CommModule::Instance->send(std::move(pkt));
        //
        // sent++;
        // // printf("Sent: %d\r\n", sent);
        // printf("Sent: %d, Recv: %d %d\r\n\033[1A", sent, recv, radio_size);
        // // if (sent>500) break;
        Thread::wait(400);
    }
}
