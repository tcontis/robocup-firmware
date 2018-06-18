#include "CommModule.hpp"

#include "Assert.hpp"
#include "CommPort.hpp"
#include "Console.hpp"
#include "HelperFuncs.hpp"
#include "Logger.hpp"

#include <ctime>

std::shared_ptr<CommModule> CommModule::Instance = nullptr;

void CommModule::send(rtp::Packet packet) {
    bool valid = false;

    for (auto subPacket = packet.subPackets.begin(); subPacket !=
            packet.subPackets.end(); ) {
        const auto portNum = (*subPacket).header.type;
        const bool portExists = m_ports.find(portNum) != m_ports.end();
        const bool hasCallback = m_ports[portNum].hasTxCallback();
        // LOG(INFO, "portnum: %d, portexists: %d, hascallback: %d", portNum, portExists, hasCallback);
        if (portExists && hasCallback) {
            subPacket++;
            valid = true;
        } else {
            subPacket = packet.subPackets.erase(subPacket);
        }
    }

    // Check to make sure a socket for the port exists
    if (valid) {
        const auto portNum = packet.subPackets[0].header.type;

        // grab an iterator to the port, lookup only once
        const auto portIter = m_ports.find(portNum);

        // invoke callback if port exists and has an attached function
        if (portIter != m_ports.end()) {
            if (portIter->second.hasTxCallback()) {
                portIter->second.getTxCallback()(&packet);
            }
        }
    } else {
        // LOG(WARN,
        //     "Failed to send %u byte packet: No TX socket on port %u exists",
        //     packet.payload.size(), packet.header.type);
    }
}

void CommModule::receive(rtp::Packet packet) {
    for (auto subPacket : packet.subPackets) {
        const auto portNum = subPacket.header.type;
        const bool portExists = m_ports.find(portNum) != m_ports.end();
        const bool hasCallback = m_ports[portNum].hasRxCallback();
        if (portExists && hasCallback) {
            const auto portIter = m_ports.find(portNum);
            if (portIter->second.hasRxCallback()) {
                portIter->second.getRxCallback()(subPacket);
            }
        }
    }
}

void CommModule::setRxHandler(RxCallbackT callback, rtp::MessageType portNbr) {
    m_ports[portNbr].setRxCallback(std::bind(callback, std::placeholders::_1));
}

void CommModule::setTxHandler(TxCallbackT callback, rtp::MessageType portNbr) {
    m_ports[portNbr].setTxCallback(std::bind(callback, std::placeholders::_1));
}

void CommModule::close(rtp::MessageType portNbr) noexcept {
    if (m_ports.count(portNbr)) m_ports.erase(portNbr);
}

#ifndef NDEBUG
unsigned int CommModule::numOpenSockets() const {
    auto count = 0;
    for (const auto& kvpair : m_ports) {
        if (kvpair.second.hasRxCallback() || kvpair.second.hasTxCallback())
            ++count;
    }
    return count;
}

unsigned int CommModule::numRxPackets() const {
    auto count = 0;
    for (const auto& kvpair : m_ports) count += kvpair.second.getRxCount();
    return count;
}

unsigned int CommModule::numTxPackets() const {
    auto count = 0;
    for (const auto& kvpair : m_ports) count += kvpair.second.getTxCount();
    return count;
}

void CommModule::resetCount(rtp::MessageType portNbr) {
    m_ports[portNbr].resetCounts();
}

void CommModule::printInfo() const {
    printf("PORT\t\tIN\tOUT\tRX CBCK\t\tTX CBCK\r\n");

    for (const auto& kvpair : m_ports) {
        const PortT& p = kvpair.second;
        printf("%d\t\t%u\t%u\t%s\t\t%s\r\n", kvpair.first, p.getRxCount(),
               p.getTxCount(), p.hasRxCallback() ? "YES" : "NO",
               p.hasTxCallback() ? "YES" : "NO");
    }

    printf(
        "==========================\r\n"
        "Total:\t\t%u\t%u\r\n",
        numRxPackets(), numTxPackets());

    Console::Instance->Flush();
}
#endif
