#include "RadioProtocol.hpp"
#include "KickerBoard.hpp"
#include "RobotModel.hpp"
#include "motors.hpp"
#include "FPGA.hpp"

#include <errno.h>
#include <string.h>

void Task_Controller_UpdateTarget(Eigen::Vector3f targetVel);
void Task_Controller_UpdateDribbler(uint8_t dribbler);

std::shared_ptr<RadioProtocol> RadioProtocol::Instance = nullptr;

RadioProtocol::RadioProtocol(std::shared_ptr<CommModule> commModule,
              std::shared_ptr<CommLink> commLink, uint8_t uid)
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
    m_commModule->setRxHandler(this, &RadioProtocol::rxDebugRequest,
                               rtp::MessageType::DEBUG_REQUEST);
    // m_commModule->setRxHandler(this, &RadioProtocol::rxFile,
    //                           rtp::MessageType::FILE);
    m_commModule->setTxHandler(m_radio.get(), &CommLink::sendPacket,
                               rtp::MessageType::ROBOT_STATUS);
    m_commModule->setTxHandler(m_radio.get(), &CommLink::sendPacket,
                               rtp::MessageType::DEBUG_RESPONSE);
    m_commModule->setTxHandler(m_radio.get(), &CommLink::sendPacket,
                              rtp::MessageType::PKT_ACK);

    LOG(INFO, "Radio protocol listening for type %d",
        rtp::MessageType::CONTROL);

    if (m_radio->isConnected() == true) {
        LOG(OK, "Radio interface ready!");
    } else {
        LOG(SEVERE, "No radio interface found!");
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


void RadioProtocol::rxControl(rtp::SubPacket subPkt) {
    const rtp::ControlMessage* msg =
        reinterpret_cast<const rtp::ControlMessage*>(subPkt.payload.data());

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

    rtp::SubPacket subPktResp;
    rtp::serializeToVector(reply, &subPktResp.payload);
    subPktResp.header.type = rtp::MessageType::ROBOT_STATUS;
    subPktResp.empty = false;
    m_subPackets.push_back(std::move(subPktResp));
}

void RadioProtocol::rxDebugRequest(rtp::SubPacket subPkt) {
    const rtp::DebugRequestMessage* msg =
        reinterpret_cast<const rtp::DebugRequestMessage*>(subPkt.payload.data());

    if (m_debugVars.find(msg->debugType) != m_debugVars.end()) {
        // TODO: don't do this
        const uint32_t SLOT_DELAY = 2;
        m_robotStatusTimer.start(1 + SLOT_DELAY * (m_uid % 6));

        rtp::DebugResponseMessage response;

        response.debugType = msg->debugType;
        std::copy(m_debugVars.at(msg->debugType).begin(),
            m_debugVars.at(msg->debugType).end(), std::begin(response.values));

        rtp::SubPacket subPktResp;
        subPktResp.header.type = rtp::MessageType::DEBUG_RESPONSE;
        subPktResp.empty = false;
        rtp::serializeToVector(response, &subPktResp.payload);
        m_subPackets.push_back(std::move(subPktResp));
    }
}

void RadioProtocol::txRobotStatus() {
    rtp::Packet pkt;
    pkt.macInfo.destAddr = 0x0000;
    pkt.macInfo.destPAN = rtp::BASE_PAN;
    pkt.macInfo.seqNum = pkt.macInfo.seqNum;
    pkt.empty = false;

    std::copy(m_subPackets.begin(), m_subPackets.end(),
        std::back_inserter(pkt.subPackets));
    m_commModule->send(std::move(pkt));
    m_subPackets.clear();
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

void RadioProtocol::updateDebug(rtp::DebugVar debugVar, float value, int index) {
    if (m_debugVars.find(debugVar) != m_debugVars.end()) {
        m_debugVars.at(debugVar)[index] = value;
    } else {
        std::array<float, 5> array;
        array[index] = value;
        m_debugVars.emplace(debugVar, array);
    }
    // m_debugVars.at(debugVar)[index] = value;
}

float RadioProtocol::getDebug(rtp::DebugVar debugVar, int index) {
    return m_debugVars.at(debugVar)[index];
}
