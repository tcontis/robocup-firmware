#include "Mbed.hpp"
#include "Rtos.hpp"
#include "Decawave.hpp"
#include "Logger.hpp"
#include "SharedSPI.hpp"
#include "pins-control.hpp"
#include "robot-config.hpp"

#include <memory>

osThreadId mainThreadID;

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

uint8_t recvSeqNum = 0;
uint16_t fileSeqNum = 0;
void radioRxHandler(rtp::Packet pkt) {
    LOG(DEBUG, "radioRxHandler()");
    fileSeqNum = pkt.header.seqNum;
    // fileSeqNum += ((recvSeqNum+256) - (fileSeqNum % 256)) % 256;
    printf("Recv: %d\r\n", fileSeqNum);
    osSignalSet(mainThreadID, 0x1);
}

LocalFileSystem local("local");

int main() {
    Serial s(RJ_SERIAL_RXTX);
    s.baud(57600);

    // Set the default logging configurations
    isLogging = RJ_LOGGING_EN;
    rjLogLevel = INFO;

    printf("****************************************\r\n");
    LOG(INFO, "Radio base test starting");

    mainThreadID = osThreadGetId();

    FILE *fp = fopen("/local/rj-ctrl", "rb");

    // fseek(fp, 0, SEEK_END);
    // long lSize = ftell(fp);
    // rewind(fp);

    // uint8_t* buffer = (uint8_t*) malloc(sizeof(uint8_t)*50);
    // fread (buffer, 1, 50, fp);

    if (initRadio()) {
        CommModule::Instance->setRxHandler(&radioRxHandler, rtp::PKT_ACK);
        CommModule::Instance->setTxHandler(dynamic_cast<CommLink*>(globalRadio.get()), &CommLink::sendPacket, rtp::FILE);

    } else {
        LOG(SEVERE, "No radio interface found!");
    }

    // static_cast<Decawave*>(globalRadio.get())->setAddress(rtp::ROBOT_PAN, 0x0001);
    globalRadio->setAddress(0, rtp::BASE_PAN);

    DigitalOut radioStatusLed(LED4, globalRadio->isConnected());

    int sent = 0;


    while (!feof(fp)) {
        // construct packet from buffer received over USB


        rtp::Packet pkt;
        pkt.macInfo.destAddr = 0xFF01;
        pkt.macInfo.destPAN = rtp::ROBOT_PAN;
        pkt.macInfo.ackRequest = 0;
        pkt.macInfo.framePending = 1;
        pkt.header.seqNum = fileSeqNum;
        pkt.header.type = rtp::MessageType::FILE;
        pkt.empty = false;


        vector<uint8_t> payload;
        payload.resize(100);
        fseek(fp, fileSeqNum*100, SEEK_SET);
        size_t result = fread(payload.data(), 1, 100, fp);
        payload.resize(result);

        if (feof(fp)) {
            pkt.macInfo.framePending = 0;
        }
        printf("framepending: %d\r\n", pkt.macInfo.framePending);

        pkt.payload = std::move(payload);

        CommModule::Instance->send(std::move(pkt));


        sent++;
        printf("Sent: %d %d\r\n", fileSeqNum, pkt.macInfo.framePending);
        // printf("Sent: %d, Recv: %d\r\n\033[1A", sent, recv);
        // if (sent>500) break;
        // Thread::wait(16);
        osSignalWait(0x1, 100);
    }

    fclose(fp);
}
