#pragma once

#include "CommLink.hpp"
#include "decadriver/deca_device_api.hpp"

enum FrameType {
    BEACON = 0b000,
    DATA = 0b001,
    ACK = 0b010,
    MAC = 0b011
    // 0b100 to 0b111 reserved
};

enum addrMode {
    NONE = 0b00,
    // 0b01 reserved
    SHORT = 0b10, // 16 bit address
    EXTENDED = 0b11 // 32 bit address
};

struct FrameControl {
    FrameType frameType : 3;
    unsigned security : 1;
    unsigned framePending : 1;
    unsigned ackRequest : 1;
    const unsigned panID : 1;
    const unsigned reserved: 3;
    const addrMode destAddrMode : 2;
    unsigned frameVersion : 2;
    const addrMode srcAddrMode : 2;
    FrameControl(unsigned panID, addrMode destAddrMode, addrMode srcAddrMode) :
        frameType(DATA), security(0), framePending(0), ackRequest(0),
        panID(panID), reserved(0), destAddrMode(destAddrMode), frameVersion(0),
        srcAddrMode(srcAddrMode) {}
} __attribute__((packed));

struct MACHeader {
    FrameControl control;
    uint8_t seqNum;
    uint16_t destPAN;
    uint16_t destAddr;
    uint16_t srcAddr;
    MACHeader() : control(0b1, SHORT, SHORT), seqNum(0),
        destPAN(rtp::BROADCAST_PAN), destAddr(rtp::BROADCAST_ADDR),
        srcAddr(rtp::BROADCAST_ADDR) {}
} __attribute__((packed));

class Decawave : public CommLink, private dw1000_api {
public:
    Decawave(SpiPtrT sharedSPI, PinName nCs, PinName intPin, PinName _nReset);

    virtual int32_t sendPacket(const rtp::Packet* pkt) override;

    // virtual void reset() override { dwt_softreset(); }
    virtual void reset() override;

    virtual int32_t selfTest() override;

    virtual bool isConnected() const override { return m_isInit; }

    virtual void setAddress(int addr, int pan = rtp::BROADCAST_PAN) override;

protected:
    virtual rtp::Packet getData() override;

private:
    static constexpr auto MAX_PACKET_LENGTH = 127;

    BufferT m_rxBuffer;
    BufferT m_txBuffer;
    uint32_t m_chipVersion;
    bool m_isInit = false;
    DigitalOut nReset;

    virtual int writetospi(uint16 headerLength, const uint8* headerBuffer,
                           uint32 bodylength, const uint8* bodyBuffer) override;
    virtual int readfromspi(uint16 headerLength, const uint8* headerBuffer,
                            uint32 readlength, uint8* readBuffer) override;
    virtual decaIrqStatus_t decamutexon() override { return 0; }

    //TODO: why?
    void setLED(bool ledOn) { dwt_setleds(ledOn); };

    void getDataSuccess(const dwt_cb_data_t* cb_data);
    void getDataFail(const dwt_cb_data_t* cb_data);
};
