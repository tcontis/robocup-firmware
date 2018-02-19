#pragma once

#include "Mbed.hpp"
#include "Rtos.hpp"

#include "CommPort.hpp"
#include "HelperFuncs.hpp"
#include "firmware-common/rtp.hpp"
#include "TimeoutLED.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <utility>
#include <deque>

/**
 * @brief A high-level firmware class for packet handling & routing
 *
 * The CommModule class provides a software routing service
 * by filtering and routing RX packets to a respective callback
 * function and sending TX packets to a derived CommLink class.
 */
class CommModule {
public:
    /// Type aliases
    using RxCallbackSigT = void(rtp::SubPacket);
    using TxCallbackSigT = uint32_t(const rtp::Packet*);
    using RxCallbackT = std::function<RxCallbackSigT>;
    using TxCallbackT = std::function<TxCallbackSigT>;
    using PortT = CommPort<RxCallbackSigT, TxCallbackSigT>;

    /// Global singleton instance of CommModule
    static std::shared_ptr<CommModule> Instance;

    /// Assign an RX callback function to a port
    void setRxHandler(RxCallbackT callback, rtp::MessageType portNbr);

    /// Assign a TX callback function to a port
    void setTxHandler(TxCallbackT callback, rtp::MessageType portNbr);

    /// Assign an RX callback method to a port
    template <typename B>
    void setRxHandler(B* obj, void (B::*mptr)(rtp::SubPacket), rtp::MessageType portNbr) {
        setRxHandler(std::bind(mptr, obj, std::placeholders::_1), portNbr);
    }

    /// Assign an TX callback method to a port
    template <typename B>
    void setTxHandler(B* obj, int32_t (B::*mptr)(const rtp::Packet*),
                      rtp::MessageType portNbr) {
        setTxHandler(std::bind(mptr, obj, std::placeholders::_1), portNbr);
    }

    /// Send an rtp::Packet over previsouly initialized hardware
    void send(rtp::Packet pkt);

    /// Called by CommLink instances whenever a packet is received via radio
    void receive(rtp::Packet pkt);

    /// Close a port that was previouly assigned callback functions/methods
    void close(rtp::MessageType portNbr) noexcept;

#ifndef NDEBUG
    /// Retuns the number of ports with a binded callback function/method
    unsigned int numOpenSockets() const;

    /// Retuns the number of currently received packets
    unsigned int numRxPackets() const;

    /// Retuns the number of currently sent packets
    unsigned int numTxPackets() const;

    /// Resets the counts for send/received packets
    void resetCount(rtp::MessageType portNbr);

    /// Print debugging information
    void printInfo() const;
#endif

private:
    std::map<rtp::MessageType, PortT> m_ports;
};
