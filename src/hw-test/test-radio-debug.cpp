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
void radioRxHandler(rtp::SubPacket subPkt) {
    LOG(DEBUG, "radioRxHandler()");
    const rtp::DebugResponseMessage* msg =
        reinterpret_cast<const rtp::DebugResponseMessage*>(subPkt.payload.data());
    LOG(INFO, "Debug: %d: [%f, %f, %f, %f, %f]", msg->debugType, msg->values[0], msg->values[1], msg->values[2], msg->values[3], msg->values[4]);
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
        CommModule::Instance->setRxHandler(&radioRxHandler, rtp::DEBUG_RESPONSE);
        CommModule::Instance->setTxHandler(dynamic_cast<CommLink*>(globalRadio.get()), &CommLink::sendPacket, rtp::DEBUG_REQUEST);

    } else {
        LOG(SEVERE, "No radio interface found!");
    }

    // static_cast<Decawave*>(globalRadio.get())->setAddress(rtp::ROBOT_PAN, 0x0001);
    globalRadio->setAddress(0x0000, rtp::BASE_PAN);

    DigitalOut radioStatusLed(LED4, globalRadio->isConnected());

    int sent = 0;

    while (true) {
        // construct packet from buffer received over USB
        rtp::Packet pkt;
        pkt.macInfo.destAddr = rtp::BROADCAST_ADDR;
        pkt.macInfo.ackRequest = 1;
        pkt.macInfo.seqNum = sent;
        pkt.empty = false;
        rtp::SubPacket subPacket;
        subPacket.header.type = rtp::MessageType::DEBUG_REQUEST;
        subPacket.empty = false;
        rtp::DebugRequestMessage message;
        message.debugType = rtp::TEST;
        rtp::serializeToVector(message, &subPacket.payload);
        pkt.subPackets.push_back(subPacket);
        CommModule::Instance->send(std::move(pkt));

        sent++;
        // printf("Sent: %d\r\n", sent);
        // printf("Sent: %d, Recv: %d\r\n\033[1A", sent, recv);
        // if (sent>500) break;
        Thread::wait(200);
    }
}
