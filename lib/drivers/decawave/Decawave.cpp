#include "Decawave.hpp"

#include "Assert.hpp"
#include "Logger.hpp"

#include <memory>

/*
Channel(RX/TX usually must be same): varies centre frequency and bandwidth
PRF(pulse repetition frequency)(16 or 64 MHz)(RX/TX must be same): higher PRF
more accuracy/range, additional power consumption
    Different PRFs will not be picked up or interfere
Preamble length(TX only): number of symbols repeated in preamble
    Selected based on data rate
    Longer preamble gives improved range but longer air time
PAC(Preamble acquisition chunk) size(RX only): chunk size used to cross
correlate to detect preamble
    Determined by expected preamble length
    Larger PAC gives better performance when preamble long enough to allow it
TX preamble code(TX only): used as specification for complex channel
    Allows for multiple unique complex channels on one channel
    Prevents overlapping channels from communicating
    Selection based on the chosen channel and PRF
RX preamble code(RX only): same as the TX preamble code
SFD(standard/non-standard)(RX/TX must be same): 0 for IEEE 802.15.4 standard, 1
for non-standard decawave found to be more robust
Data rate(RX/TX must be same): rate of transmission, higher means faster but
less range
PHY header mode(RX/TX must be same): use standard 128 or extended 1024 octet
frame
SFD timeout(RX only): timout from start of aquiring preamble to recover from
false preamble detection
    Reccomended length: (preamble length + 1 + SFD length - PAC size)
*/

/*
Summary:
    Channel          (choose)
    PRF              (choose)
    Preamble length  (based on data rate)
    PAC size         (based on preamble length)
    TX preamble code (based on channel and PRF)
    RX preamble code (based on TX preamble code)
    SFD              (choose, likely 0)
    Data rate        (choose)
    PHY header mode  (choose, likely STD)
    SFD timout       (based on preamble length)
*/

namespace {
dwt_config_t config = {
    4,                  // Channel
    DWT_PRF_64M,        // PRF
    DWT_PLEN_128,       // Preamble length
    DWT_PAC8,           // PAC size
    17,                 // TX preamble code
    17,                 // RX preamble code
    0,                  // standard/non-standard SFD
    DWT_BR_6M8,         // Data rate
    DWT_PHRMODE_STD,    // PHY header mode
    (128 + 1 + 64 - 8)  // SFD timeout
};

static dwt_txconfig_t txconfig = {
    0x95,        // PG delay
    0x9A9A9A9A,  // TX power
};

constexpr auto TX_TO_RX_DELAY_UUS = 60;
constexpr auto RX_RESP_TO_UUS = 5000;
}

Decawave::Decawave(SpiPtrT sharedSPI, PinName nCs, PinName intPin,
                   PinName _nReset)
    : CommLink(sharedSPI, nCs, intPin), dw1000_api(), nReset(_nReset) {
    reset();
    if (m_isInit) {
        LOG(INFO, "Decawave ready!");
    } else {
        LOG(SEVERE, "Decawave not initialized");
    }
}

// Virtual functions from CommLink
int32_t Decawave::sendPacket(const rtp::Packet* pkt) {
    // Return failutre if not initialized
    if (!m_isInit) return COMM_FAILURE;

    const auto bufferSize = sizeof(MACHeader) + pkt->size() + 2;
    if (bufferSize > MAX_PACKET_LENGTH || pkt->empty) {
        return COMM_DEV_BUF_ERR;
    }

    dwt_rxreset();
    dwt_forcetrxoff();

    m_txBuffer.clear();
    m_txBuffer.reserve(bufferSize);

    MACHeader header;
    header.control.frameType = DATA;
    header.control.ackRequest = pkt->macInfo.ackRequest;
    header.seqNum = pkt->macInfo.seqNum;
    header.control.framePending = pkt->macInfo.framePending;
    header.destPAN = pkt->macInfo.destPAN;
    header.destAddr = pkt->macInfo.destAddr;

    if (pkt->macInfo.srcAddr == rtp::BROADCAST_ADDR) {
        header.srcAddr = static_cast<uint16_t>(m_address);
    } else {
        header.srcAddr = pkt->macInfo.srcAddr;
    }

    const auto headerFirstPtr = reinterpret_cast<const uint8_t*>(&header);
    const auto headerLastPtr = headerFirstPtr + sizeof(header);
    // MAC layer header for Decawave
    m_txBuffer.insert(m_txBuffer.end(), headerFirstPtr, headerLastPtr);

    for (auto subPacket : pkt->subPackets) {
        const auto rtpHeaderFirstPtr = reinterpret_cast<const uint8_t*>(&subPacket.header);
        const auto rtpHeaderLastPtr = rtpHeaderFirstPtr + sizeof(rtp::Header);
        // insert the rtp header
        m_txBuffer.insert(m_txBuffer.end(), rtpHeaderFirstPtr, rtpHeaderLastPtr);

        // insert the rtp payloads
        m_txBuffer.insert(m_txBuffer.end(), subPacket.payload.begin(), subPacket.payload.end());
    }
    // insert padding for CRC
    m_txBuffer.insert(m_txBuffer.end(), {0x00, 0x00});

    // send to the Decawave's API
    dwt_writetxdata(m_txBuffer.size(), m_txBuffer.data(), 0);
    dwt_writetxfctrl(m_txBuffer.size(), 0, 0);

    const auto sentStatus =
        dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);
    const auto hadError = sentStatus != DWT_SUCCESS;

    return hadError ? COMM_FAILURE : COMM_SUCCESS;
}

rtp::Packet Decawave::getData() {
    // TODO: more optimized return with std::move?
    rtp::Packet pkt;

    // Return empty data if not initialized
    if (!m_isInit) return pkt;

    // manually invoke the isr routine & set back into RX mode
    // TODO: probably a better way to do this?
    dwt_isr();
    // our buffer is now filled with the received bytes if everything went ok
    dwt_rxenable(DWT_START_RX_IMMEDIATE);

    // return empty buffer if the isr routine failed to fill it with anything
    if (m_rxBuffer.empty()) return pkt;

    auto bufOffset = 0;

    MACHeader macHeader = *(reinterpret_cast<const MACHeader*>(m_rxBuffer.data() + bufOffset));
    bufOffset += sizeof(MACHeader);

    while (m_rxBuffer.begin() + bufOffset != m_rxBuffer.end() - 2) {
        rtp::SubPacket subPacket;
        subPacket.header = *(reinterpret_cast<const rtp::Header*>(m_rxBuffer.data() + bufOffset));
        bufOffset += sizeof(rtp::Header);
        size_t packetSize = rtp::SubPacket::messageSize(subPacket.header.type);

        subPacket.payload.assign(m_rxBuffer.begin() + bufOffset, m_rxBuffer.begin() + bufOffset + packetSize);
        bufOffset += packetSize;
        pkt.subPackets.push_back(std::move(subPacket));
    }

    pkt.macInfo.seqNum = macHeader.seqNum;
    pkt.macInfo.destPAN = macHeader.destPAN;
    pkt.macInfo.destAddr = macHeader.destAddr;
    pkt.macInfo.srcAddr = macHeader.srcAddr;
    pkt.macInfo.ackRequest = macHeader.control.ackRequest;
    pkt.macInfo.framePending = macHeader.control.framePending;
    pkt.empty = false;

    // move the buffer to the caller
    return pkt;
}

int32_t Decawave::selfTest() {
    m_chipVersion = dwt_readdevid();

    if (m_chipVersion != DWT_DEVICE_ID) {
        return -1;
    } else {
        m_isInit = true;
        return 0;
    }
}

void Decawave::reset() {
    nReset = 0;
    wait_ms(3);
    nReset = 1;

    m_isInit = 0;
    setSPIFrequency(2'000'000);

    if (dwt_initialise(DWT_LOADUCODE) == DWT_ERROR) {
        return;
    }

    // reset();
    dwt_softreset();
    selfTest();

    if (m_isInit) {
        dwt_configure(&config);
        dwt_configuretxrf(&txconfig);

        setSPIFrequency(8'000'000);  // won't work after 8MHz (or will it?)

        dwt_setrxaftertxdelay(TX_TO_RX_DELAY_UUS);
        dwt_setcallbacks(
            nullptr, static_cast<dwt_cb_t>(&Decawave::getDataSuccess), nullptr,
            static_cast<dwt_cb_t>(&Decawave::getDataFail));
        dwt_setinterrupt(DWT_INT_RFCG, 1);

        dwt_setautorxreenable(1);

        setLED(true);
        dwt_forcetrxoff();  // TODO: Better way than force off then reset?
        dwt_rxreset();
         // TODO: should this be commented?
        dwt_rxenable(DWT_START_RX_IMMEDIATE);

        CommLink::ready();
    }
}

void Decawave::setAddress(int addr, int pan) {
    m_address = addr;
    m_pan = pan;
    CommLink::setAddress(addr, pan);
    dwt_setpanid(static_cast<uint16_t>(m_pan));
    dwt_setaddress16(static_cast<uint16_t>(m_address));
    dwt_enableframefilter(DWT_FF_DATA_EN);
}

// Virtual functions from dw1000_api
int Decawave::writetospi(uint16 headerLength, const uint8* headerBuffer,
                         uint32 bodylength, const uint8* bodyBuffer) {
    chipSelect();
    for (size_t i = 0; i < headerLength; ++i) m_spi->write(headerBuffer[i]);
    for (size_t i = 0; i < bodylength; ++i) m_spi->write(bodyBuffer[i]);
    chipDeselect();
    return 0;
}

int Decawave::readfromspi(uint16 headerLength, const uint8* headerBuffer,
                          uint32 readlength, uint8* readBuffer) {
    chipSelect();
    for (size_t i = 0; i < headerLength; ++i) m_spi->write(headerBuffer[i]);
    for (size_t i = 0; i < readlength; ++i) readBuffer[i] = m_spi->write(0);
    chipDeselect();
    return 0;
}

// Callback functions for decawave interrupt
void Decawave::getDataSuccess(const dwt_cb_data_t* cb_data) {
    const auto dataLength = cb_data->datalength;
    // Read recived data to m_rxBufferPtr array
    m_rxBuffer.resize(dataLength);
    dwt_readrxdata(m_rxBuffer.data(), dataLength, 0);
}

void Decawave::getDataFail(const dwt_cb_data_t* cb_data) {}
