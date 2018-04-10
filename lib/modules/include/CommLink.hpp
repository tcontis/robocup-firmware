#pragma once

#include "CommModule.hpp"
#include "MacroHelpers.hpp"
#include "MailHelpers.hpp"
#include "rc-fshare/rtp.hpp"
#include "Rtos.hpp"
#include "SharedSPI.hpp"

#include <memory>

/// CommLink error levels
enum CommStatus : int32_t { COMM_SUCCESS, COMM_FAILURE, COMM_DEV_BUF_ERR, COMM_NO_DATA };

/**
 * CommLink Class used as the hal (hardware abstraction layer) module for
 * interfacing communication links to the higher-level firmware
 */
class CommLink : public SharedSPIDevice<> {
public:
    // Type aliases
    using BufferT = std::vector<uint8_t>;
    using BufferPtrT = BufferT*;
    using ConstBufferPtrT = const BufferT*;

    /// Constructor
    CommLink(SpiPtrT spiBus, PinName nCs = NC, PinName intPin = NC);

    /// Kills any threads and frees the allocated stack.
    virtual ~CommLink() = default;

    // The pure virtual methods for making CommLink an abstract class
    /// Perform a soft reset for a communication link's hardware device
    virtual void reset() = 0;

    /// Perform tests to determine if the hardware is able to properly function
    virtual int32_t selfTest() = 0;

    /// Determine if communication can occur with another device
    virtual bool isConnected() const = 0;

    /// Send & Receive through the rtp structure
    virtual int32_t sendPacket(const rtp::Packet* pkt) = 0;

    /// Set the MAC layer filtering address for the link
    virtual void setAddress(int addr, int pan = rtp::BROADCAST_PAN) {
        m_address = addr;
        m_pan = pan;
    }

protected:
    static constexpr size_t SIGNAL_START = (1 << 1);
    static constexpr size_t SIGNAL_RX = (1 << 2);

    int m_address = rtp::INVALID_ROBOT_UID;
    int m_pan = rtp::BROADCAST_PAN;
    InterruptIn m_intIn;

    virtual rtp::Packet getData() = 0;

    /// Interrupt Service Routine
    void ISR() { m_rxThread.signal_set(SIGNAL_RX); }

    /// Called by the derived class to begin thread operations
    void ready() { m_rxThread.signal_set(SIGNAL_START); }

private:
    Thread m_rxThread;

    // The working thread for handling RX data queue operations
    void rxThread();

    inline static void rxThreadHelper(const void* linkInst) {
        auto link = reinterpret_cast<CommLink*>(
            const_cast<void*>(linkInst));  // dangerous
        link->rxThread();
    }
};

extern std::shared_ptr<CommLink> globalRadio;
