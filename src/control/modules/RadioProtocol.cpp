#include "RadioProtocol.hpp"
#include "KickerBoard.hpp"
#include "RobotModel.hpp"
#include "motors.hpp"
#include "FPGA.hpp"

#include <errno.h>
#include <string.h>

void Task_Controller_UpdateTarget(Eigen::Vector3f targetVel);
void Task_Controller_UpdateDribbler(uint8_t dribbler);

RadioProtocol::RadioProtocol(std::shared_ptr<CommModule> commModule,
              std::shared_ptr<CommLink> commLink,
              uint8_t uid)
    : m_commModule(commModule),
      m_radio(commLink),
      m_uid(uid),
      m_state(State::STOPPED),
      m_robotStatusTimer(this, &RadioProtocol::txRobotStatus, osTimerOnce),
      m_radioTimeoutTimer(this, &RadioProtocol::radioTimeout, osTimerOnce),
      m_timeoutTimer(this, &RadioProtocol::timeout, osTimerOnce) {
    ASSERT(m_commModule != nullptr);
    ASSERT(m_radio != nullptr);
    m_radioTimeoutTimer.start(RADIO_TIMEOUT_INTERVAL);
    setUID(m_uid);
}

void RadioProtocol::start() {
    m_state = State::DISCONNECTED;

    m_commModule->setRxHandler(this, &RadioProtocol::rxControl,
                               rtp::MessageType::CONTROL);
    m_commModule->setRxHandler(this, &RadioProtocol::rxFile,
                              rtp::MessageType::FILE);
    m_commModule->setTxHandler(m_radio.get(), &CommLink::sendPacket,
                               rtp::MessageType::ROBOT_STATUS);
    m_commModule->setTxHandler(m_radio.get(), &CommLink::sendPacket,
                              rtp::MessageType::PKT_ACK);

    LOG(INFO, "Radio protocol listening for type %d",
        rtp::MessageType::CONTROL);

    if (m_radio->isConnected() == true) {
        LOG(OK, "Radio interface ready!");
    } else {
        LOG(SEVERE, "No radio interface found!");
    }

    while (!m_commModule->isReady()) {
        Thread::wait(50);
    }
}

void RadioProtocol::stop() {
    m_commModule->close(rtp::MessageType::CONTROL);

    m_robotStatusTimer.stop();
    m_radioTimeoutTimer.stop();
    m_timeoutTimer.stop();

    m_state = State::STOPPED;

    LOG(INFO, "Radio protocol stopped");
}


void RadioProtocol::rxControl(rtp::Packet pkt) {
    const rtp::ControlMessage* msg =
        reinterpret_cast<const rtp::ControlMessage*>(pkt.payload.data());

    m_state = State::CONNECTED;

    // TODO: timout timer
    m_radioTimeoutTimer.start(RADIO_TIMEOUT_INTERVAL);
    m_timeoutTimer.start(TIMEOUT_INTERVAL);


    // TODO: check this
    const uint32_t SLOT_DELAY = 2;

    // TODO: very not good
    // LOG(INFO, "Packet recieved");
    m_robotStatusTimer.start(1 + SLOT_DELAY * (m_uid % 6));

    // update target velocity from packet
    Task_Controller_UpdateTarget({
        static_cast<float>(msg->bodyX) / rtp::VELOCITY_SCALE_FACTOR,
        static_cast<float>(msg->bodyY) / rtp::VELOCITY_SCALE_FACTOR,
        static_cast<float>(msg->bodyW) / rtp::VELOCITY_SCALE_FACTOR,
    });

    // dribbler
    Task_Controller_UpdateDribbler(msg->dribbler);

    if (msg->triggerMode == 0) {
        KickerBoard::Instance->cancelBreakbeam();
    }

    // kick!
    if (msg->shootMode == 0) {
        uint8_t kickStrength = msg->kickStrength;
        if (msg->triggerMode == 1) {
            // kick immediate
            KickerBoard::Instance->kick(kickStrength);
        } else if (msg->triggerMode == 2) {
            // kick on break beam
            KickerBoard::Instance->kickOnBreakbeam(kickStrength);
        } else {
            KickerBoard::Instance->cancelBreakbeam();
        }
    }

    KickerBoard::Instance->setChargeAllowed(true);


    rtp::RobotStatusMessage reply;
    // reply.uid = robotShellID;
    reply.battVoltage = 0;// TODO
    reply.ballSenseStatus = KickerBoard::Instance->isBallSensed();

    // report any motor errors
    reply.motorErrors = 0;
    for (auto i = 0; i < 5; i++) {
        auto err = global_motors[i].status.hasError;
        if (err) reply.motorErrors |= (1 << i);
    }

    // for (auto i = 0; i < wheelStallDetection.size(); i++) {
    //     if (wheelStallDetection[i].stalled) {
    //         reply.motorErrors |= (1 << i);
    //     }
    // }

    // fpga status
    if (!FPGA::Instance->isReady()) {
        reply.fpgaStatus = 1;
    // } else if (fpgaError) {
    //     reply.fpgaStatus = 1;
    } else {
        reply.fpgaStatus = 0;  // good
    }

    // kicker status
    reply.kickStatus = KickerBoard::Instance->getVoltage() > 230;
    // reply.kickHealthy = KickerBoard::Instance->isHealthy();

    vector<uint8_t> replyBuf;
    rtp::serializeToVector(reply, &replyBuf);
    m_robotStatus = std::move(replyBuf);
}

int fileCopy(const char* local_file1, const char* local_file2) {
    FILE *rp = fopen(local_file1, "rb");
    if (rp == NULL) {
        return -1;
    }
    remove(local_file2);
    FILE *wp = fopen(local_file2, "wb");
    if (wp == NULL) {
        fclose(rp);
        return -2;
    }
    int c;
    // while ((c = fgetc(rp)) != EOF) {
    //     fputc(c, wp);
    // }
    uint8_t temp[100];
    while (!feof(rp)) {
        // printf("b");
        int count = fread(temp, 1, 100, rp);
        fwrite(temp, 1, count, wp);
        // Thread::wait(10);
    }
    fclose(rp);
    fclose(wp);
    return 0;
}

extern "C" void mbed_reset();

int cleanupBin() {
    struct dirent *p;
    DIR *dir = opendir("/local");
    if (dir == NULL) {
        return -1;
    }
    while ((p = readdir(dir)) != NULL) {
        char *str = p->d_name;
        if ((strstr(str, ".bin") != NULL) || (strstr(str, ".BIN") != NULL)) {
            char buf[BUFSIZ];
            snprintf(buf, sizeof(buf) - 1, "/local/%s", str);
            remove(buf);
        }
    }
    closedir(dir);
    return 0;
}

FILE *fp;
uint16_t fileSeqNum = 0;
void RadioProtocol::rxFile(rtp::Packet pkt) {
    if (pkt.header.seqNum == (fileSeqNum)) {
        if (fileSeqNum == 0) {
            FILE *fp2;
            if ((fp2 = fopen("/local/firm1.bin", "r")) == 0) {
              cleanupBin();
              fp = fopen("/local/firm1.bin", "wb");
            } else {
              cleanupBin();
              fp = fopen("/local/firm2.bin", "wb");
            }
            LOG(INFO, "File transfer opened");
        }
        fileSeqNum++;
        fwrite(pkt.payload.data(), 1, pkt.payload.size(), fp);
        if (pkt.macInfo.framePending != 1) {
            LOG(INFO, "File transfer closed");
            fileSeqNum = -1;
            fclose(fp);
            // remove("/local/tmp.bin");
            // int res = rename("/local/mbed.htm", "/local/tmp.bin");
            // printf("res: %d\r\n", res);
            // printf("Error: %s\r\n", strerror(errno));
            printf("start copy\r\n");
            Thread::wait(3000);
            // fileCopy("/local/temp.bin", "/local/tmp.bin");
            printf("stop copy\r\n");
            mbed_reset();
        }
    }

    printf("%d  %d\r\n", pkt.header.seqNum, fileSeqNum);

    rtp::Packet pkt2;
    pkt2.macInfo.destAddr = 0x0000;
    pkt2.macInfo.destPAN = rtp::BASE_PAN;
    pkt2.header.seqNum = fileSeqNum;
    pkt2.header.type = rtp::MessageType::PKT_ACK;
    pkt2.empty = false;

    m_commModule->send(std::move(pkt2));
}

// void rxHandler(rtp::Packet pkt) {
//     // TODO: check packet size before parsing
//     //        bool addressed = false;
//     const rtp::ControlMessage* controlMessage = nullptr;
//     // printf("UUIDs: ");
//     const auto messages =
//         reinterpret_cast<const rtp::RobotTxMessage*>(pkt.payload.data());
//
//     size_t slot;
//     for (size_t i = 0; i < 6; i++) {
//         auto msg = std::next(messages, i);
//
//         // printf("%d:%d ", slot, msg->uid);
//         if (msg->uid == m_uid) {
//             if (msg->messageType ==
//                 rtp::RobotTxMessage::ControlMessageType) {
//                 controlMessage = &msg->message.controlMessage;
//             }
//             slot = i;
//         }
//     }
//
//     /// time, in ms, for each reply slot
//     // TODO(justin): double-check this
//     const uint32_t SLOT_DELAY = 2;
//
//     m_state = State::CONNECTED;
//
//     // reset timeout whenever we receive a packet
//     m_timeoutTimer.stop();
//     m_timeoutTimer.start(TIMEOUT_INTERVAL);
//
//     // TODO: this is bad and lazy
//     if (controlMessage) {
//         m_replyTimer.start(1 + SLOT_DELAY * slot);
//     } else {
//         m_replyTimer.start(1 + SLOT_DELAY * (m_uid % 6));
//     }
//
//     if (rxCallback) {
//         m_reply = rxCallback(controlMessage, controlMessage != nullptr);
//     } else {
//         LOG(WARN, "no callback set");
//     }
//
//     for (size_t i = 0; i < 6; i++) {
//         auto msg = std::next(messages, i);
//         if (msg->uid == m_uid || msg->uid == rtp::ANY_ROBOT_UID) {
//             if (msg->messageType == rtp::RobotTxMessage::ConfMessageType) {
//                 if (confCallback) {
//                     const auto confMessage = msg->message.confMessage;
//                     confCallback(confMessage);
//                 }
//             } else if (msg->messageType ==
//                        rtp::RobotTxMessage::DebugMessageType) {
//                 if (debugCallback) {
//                     const auto debugMessage = msg->message.debugMessage;
//                     debugCallback(debugMessage);
//                 }
//             }
//         }
//     }
// }

void RadioProtocol::txRobotStatus() {
    rtp::Packet pkt;
    pkt.macInfo.destAddr = 0x0000;
    pkt.macInfo.destPAN = rtp::BASE_PAN;
    pkt.macInfo.seqNum = pkt.macInfo.seqNum;
    pkt.header.type = rtp::MessageType::ROBOT_STATUS;
    pkt.empty = false;

    pkt.payload = std::move(m_robotStatus);

    m_commModule->send(std::move(pkt));
}

void RadioProtocol::timeout() {
    m_state = State::DISCONNECTED;
}

void RadioProtocol::setUID(uint8_t uid) {
    m_uid = uid;
    m_radio->setAddress(0xFF00 + (m_uid & 0xFF), rtp::ROBOT_PAN);
}

void RadioProtocol::radioTimeout() {
    // Reset radio if no RX packet in specified time
    // TODO: check if rx enabled if neccesary to reset
    m_radioTimeoutTimer.start(RADIO_TIMEOUT_INTERVAL);

    m_radio->reset();
    setUID(m_uid);
}
