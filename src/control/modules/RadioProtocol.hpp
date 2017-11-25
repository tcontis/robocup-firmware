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

    ~RadioProtocol() { stop(); }

    void start();
    void stop();

    /// Set robot unique id.  Also update address.
    void setUID(uint8_t uid);
    State state() const { return m_state; }


private:
    void rxControl(rtp::Packet pkt);
    void txRobotStatus();

    void radioTimeout();
    void timeout();

    std::shared_ptr<CommModule> m_commModule;
    std::shared_ptr<CommLink> m_radio;

    uint32_t m_lastReceiveTime = 0;
    uint8_t m_uid;
    State m_state;

    std::vector<uint8_t> m_robotStatus;

    RtosTimerHelper m_robotStatusTimer;
    RtosTimerHelper m_radioTimeoutTimer;
    RtosTimerHelper m_timeoutTimer;
};
