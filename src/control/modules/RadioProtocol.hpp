#pragma once

#include "CommModule.hpp"
#include "Decawave.hpp"
#include "Rtos.hpp"
#include "RtosTimerHelper.hpp"
#include "firmware-common/rtp.hpp"
#include "Logger.hpp"

class RadioProtocol {
public:
    enum class State {
        STOPPED,
        DISCONNECTED,
        CONNECTED,
    };

    /// After this number of milliseconds without receiving a packet from the
    /// base station, we are considered "disconnected"
    static const uint32_t TIMEOUT_INTERVAL = 2000;
    static const uint32_t RADIO_TIMEOUT_INTERVAL = 2000;

    RadioProtocol(std::shared_ptr<CommModule> commModule,
                  std::shared_ptr<CommLink> commLink,
                  uint8_t uid = rtp::INVALID_ROBOT_UID);

    static std::shared_ptr<RadioProtocol> Instance;

    ~RadioProtocol() { stop(); }

    void start();
    void stop();

    /// Set robot unique id.  Also update address.
    void setUID(uint8_t uid);
    State state() const { return m_state; }

    void updateDebug(rtp::DebugVar debugVar, float value, int index = 0);
    float getDebug(rtp::DebugVar debugVar, int index = 0);

private:
    void rxControl(rtp::SubPacket subPkt);
    void rxDebugRequest(rtp::SubPacket subPkt);
    // void rxFile(rtp::Packet pkt);
    void txRobotStatus();

    void radioTimeout();
    void timeout();

    std::shared_ptr<CommModule> m_commModule;
    std::shared_ptr<CommLink> m_radio;

    uint32_t m_lastReceiveTime = 0;
    uint8_t m_uid;
    State m_state;

    // std::vector<uint8_t> m_robotStatus;
    std::vector<rtp::SubPacket> m_subPackets;

    std::map<rtp::DebugVar, std::array<float, 5>> m_debugVars;

    RtosTimerHelper m_robotStatusTimer;
    RtosTimerHelper m_radioTimeoutTimer;
    RtosTimerHelper m_timeoutTimer;
};
