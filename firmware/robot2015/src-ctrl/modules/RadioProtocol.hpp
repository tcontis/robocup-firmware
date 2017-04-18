#pragma once

#include <rtos.h>
#include "CC1201.hpp"
#include "CommModule.hpp"
#include "Decawave.hpp"
#include "RtosTimerHelper.hpp"
#include "protobuf/nanopb/RadioTx.pb.h"
#include "protobuf/nanopb/Control.pb.h"
#include "protobuf/nanopb/RadioRx.pb.h"
#include "pb_decode.h"
#include "zlib.h"

class RadioProtocol {
public:
    enum State {
        STOPPED,
        DISCONNECTED,
        CONNECTED,
    };

    /// After this number of milliseconds without receiving a packet from the
    /// base station, we are considered "disconnected"
    static const uint32_t TIMEOUT_INTERVAL = 2000;

    RadioProtocol(std::shared_ptr<CommModule> commModule, Decawave* radio,
                  uint8_t uid = rtp::INVALID_ROBOT_UID)
        : _commModule(commModule),
          _radio(radio),
          _uid(uid),
          _state(STOPPED),
          _replyTimer(this, &RadioProtocol::reply, osTimerOnce),
          _timeoutTimer(this, &RadioProtocol::_timeout, osTimerOnce) {
        ASSERT(commModule != nullptr);
        ASSERT(radio != nullptr);
        _radio->setAddress(rtp::ROBOT_ADDRESS);
    }

    ~RadioProtocol() { stop(); }

    /// Set robot unique id.  Also update address.
    void setUID(uint8_t uid) { _uid = uid; }

    /**
     * Callback that is called whenever a packet is received.  Set this in
     * order to handle parsing the packet and creating a response.  This
     * callback
     * should return a formatted reply buffer, which will be sent in the
     * appropriate reply slot.
     *
     * @param msg A pointer to the start of the message addressed to this robot
     * @return formatted reply buffer
     */
    std::function<std::vector<uint8_t>(const Packet_RadioRobotControl* msg,
                                       const bool addresed)> rxCallback;

    void start() {
        _state = DISCONNECTED;

        _commModule->setRxHandler(this, &RadioProtocol::rxHandler,
                                  rtp::Port::CONTROL);
        _commModule->setTxHandler((CommLink*)global_radio,
                                  &CommLink::sendPacket, rtp::Port::CONTROL);

        LOG(INF1, "Radio protocol listening on port %d", rtp::Port::CONTROL);
    }

    void stop() {
        _commModule->close(rtp::Port::CONTROL);

        _replyTimer.stop();
        _state = STOPPED;

        LOG(INF1, "Radio protocol stopped");
    }

    State state() const { return _state; }

    int gunzip(unsigned char *dst, unsigned long *dst_length, unsigned char *src, unsigned long src_length)
    {
        z_stream stream;
        memset(&stream, 0, sizeof(stream));
     
        stream.next_in = src;
        stream.avail_in = src_length;
     
        stream.next_out = dst;
        stream.avail_out = *dst_length;
     
        int rv = inflateInit2(&stream, 15 + 16);
        if (Z_OK == rv) {
            rv = inflate(&stream, Z_NO_FLUSH);
            if (Z_STREAM_END == rv) {
                inflateEnd(&stream);
                rv = Z_OK;
            }
        }
     
        if (Z_OK == rv) {
            *dst_length = stream.total_out;
        } else {
            *dst_length = 0;
        }
     
        return rv;
    }

    void rxHandler(rtp::packet pkt) {
        //LOG(INIT, "got pkt!");
        // TODO: check packet size before parsing
        bool addressed = false;
        //const rtp::ControlMessage* msg = nullptr;
        
        //printf("UUIDs: ");
        Packet_RobotsTxPacket test ;
        //auto t = Packet_RadioTx_fields;
        //LOG(INIT, "%d", sizeof(Packet_RobotsTxPacket));
        //LOG(INIT, "test1\r\n");
        //fflush(stdout);
       // Thread::wait(1000);

        const Packet_RadioRobotControl *controlMessage = nullptr;

        unsigned long output_data_length = 0;
        unsigned char output_data[Packet_RobotsTxPacket_size];
        int rv = gunzip(output_data, &output_data_length, pkt.payload.data(), pkt.payload.size());

        auto stream = pb_istream_from_buffer(output_data, output_data_length);
        bool worked = pb_decode(&stream, Packet_RobotsTxPacket_fields, &test); 
        //LOG(INIT, "%d:%d", 1, worked);
        //Thread::wait(1000);
        size_t slot;

        //LOG(INF1, "%d:%d", test.robots[0].message.control.integer_vel_commands.xVelocity ,worked);
        //LOG(INIT, "test.robots_count:%d", test.robots_count);
        if (worked) {

            for (slot = 0; slot < test.robots_count; slot++) {
                //LOG(INIT, "slot:%d", slot);
                const auto& robot = test.robots[slot];

                //LOG(INIT, "Slot %d; UID: %d ", slot, robot.uid);
                
                //size_t offset = slot * sizeof(rtp::ControlMessage);
                //msg = (const rtp::ControlMessage*)(pkt.payload.data() + offset);

                if (robot.uid == _uid) {
                    // LOG(INIT, "")
                    addressed = true;
                    if (robot.which_message == Packet_RobotTxPacket_control_tag) {
                        //LOG(INIT, "control Message");
                        controlMessage = &robot.message.control;
                    }
                    break;
                }
                
            }
        }
        
        // printf("\r\n");

        /// time, in ms, for each reply slot
        // TODO(justin): double-check this
        const uint32_t SLOT_DELAY = 2;

        _state = CONNECTED;

        // reset timeout whenever we receive a packet
        _timeoutTimer.stop();
        _timeoutTimer.start(TIMEOUT_INTERVAL);

        // TODO: this is bad and lazy
        
        if (addressed) {
            //LOG(INIT, "replyTimer STart");
            _replyTimer.start(1 + SLOT_DELAY * slot);
        } else {
            _replyTimer.start(1 + SLOT_DELAY * (_uid % 6));
        }
        
        if (rxCallback) {
            _reply = std::move(rxCallback(controlMessage, addressed));
        } else {
            LOG(WARN, "no callback set");
        }
        //LOG(INIT, "doNE");
    }

private:
    void reply() {

        //LOG(INIT, "Replyyyy");
        rtp::packet pkt;
        pkt.header.port = rtp::Port::CONTROL;
        pkt.header.address = rtp::BASE_STATION_ADDRESS;

        pkt.payload = std::move(_reply);

        _commModule->send(std::move(pkt));
    }

    void _timeout() { _state = DISCONNECTED; }

    std::shared_ptr<CommModule> _commModule;
    Decawave* _radio;

    uint32_t _lastReceiveTime = 0;

    uint8_t _uid;
    State _state;

    std::vector<uint8_t> _reply;

    RtosTimerHelper _replyTimer;
    RtosTimerHelper _timeoutTimer;
};
