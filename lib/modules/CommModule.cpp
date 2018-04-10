#include "CommModule.hpp"

#include "Assert.hpp"
#include "CommPort.hpp"
#include "Console.hpp"
#include "HelperFuncs.hpp"
#include "Logger.hpp"

#include <ctime>

std::shared_ptr<CommModule> CommModule::Instance = nullptr;

void CommModule::send(rtp::Packet packet, uint8_t portNum) {
    // Check if callback exists
    if (m_txCallbacks.find(portNum) != m_txCallbacks.end()) {
        m_txCallbacks.at(portNum)(&packet);
        m_txCount[portNum]++;
    }
}

void CommModule::receive(rtp::Packet packet) {
    for (auto subPacket : packet.subPackets) {
        const auto portNum = subPacket.header.type;
        // Check if callback exists
        if (m_rxCallbacks.find(portNum) != m_rxCallbacks.end()) {
            m_rxCallbacks.at(portNum)(subPacket);
            m_rxCount[portNum]++;
        }
    }
}

void CommModule::setRxHandler(RxCallbackT callback, rtp::MessageType portNum) {
    m_rxCallbacks[portNum] = callback;
    m_rxCount[portNum] = 0;
}

void CommModule::setTxHandler(TxCallbackT callback, uint8_t portNum) {
    m_txCallbacks[portNum] = callback;
    m_txCount[portNum] = 0;
}

void CommModule::closeRx(rtp::MessageType portNum) noexcept {
    m_rxCallbacks.erase(portNum);
    m_rxCount.erase(portNum);
}
void CommModule::closeTx(uint8_t portNum) noexcept {
    m_txCallbacks.erase(portNum);
    m_txCount.erase(portNum);
}

#ifndef NDEBUG
unsigned int CommModule::numOpenSockets() const {
    return m_rxCallbacks.size();
}

unsigned int CommModule::numRxPackets() const {
    auto countSum = 0;
    for (const auto& kvpair : m_rxCount) countSum += kvpair.second;
    return countSum;
}

unsigned int CommModule::numTxPackets() const {
    auto countSum = 0;
    for (const auto& kvpair : m_txCount) countSum += kvpair.second;
    return countSum;
}

void CommModule::resetCount(rtp::MessageType portNbr) {
    for (auto& kvpair : m_txCount) kvpair.second = 0;
    for (auto& kvpair : m_rxCount) kvpair.second = 0;
}

void CommModule::printInfo() const {
    // printf("PORT\t\tIN\tOUT\tRX CBCK\t\tTX CBCK\r\n");
    //
    // for (const auto& kvpair : m_rxCount) {
    //     const PortT& p = kvpair.second;
    //     printf("%d\t\t%u\t%u\t%s\t\t%s\r\n", kvpair.first, p.getRxCount(),
    //            p.getTxCount(), p.hasRxCallback() ? "YES" : "NO",
    //            p.hasTxCallback() ? "YES" : "NO");
    // }
    //
    // printf(
    //     "==========================\r\n"
    //     "Total:\t\t%u\t%u\r\n",
    //     numRxPackets(), numTxPackets());
    //
    // Console::Instance->Flush();
}
#endif
