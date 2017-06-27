#include <mbed.h>
#include <rtos.h>

#include "KickerBoard.hpp"
#include "SharedSPI.hpp"
#include "pins-ctrl-2015.hpp"

using namespace std;

const int BAUD_RATE = 57600;
const float ALIVE_BLINK_RATE = 0.25;
const float LOG_RATE = 1.0f / 50; // 10 Hz?

LocalFileSystem fs("local");

Ticker lifeLight;
Ticker dataLogger;
Timer log_time;
DigitalOut ledOne(LED1);
DigitalOut ledTwo(LED2);

Serial pc(USBTX, USBRX);  // tx and rx

std::unique_ptr<KickerBoard> kicker;
/**
 * timer interrupt based light flicker
 */
void imAlive() { ledOne = !ledOne; }

void log() {
    uint8_t voltage;
    kicker->read_voltage(&voltage);
    printf("%.3f,%d\r\n", log_time.read(), voltage);
}

void set_logging(bool should_log) {
    if (should_log) {
        log_time.reset();
        log_time.start();
        dataLogger.attach(&log, LOG_RATE);
    } else {
        dataLogger.detach();
    }
}


std::string bool_to_string(bool b) { return b ? "true" : "false"; }

int main() {
    isLogging = false;
    rjLogLevel = INF2;
    lifeLight.attach(&imAlive, ALIVE_BLINK_RATE);

    pc.baud(BAUD_RATE);  // set up the serial
    shared_ptr<SharedSPI> sharedSPI =
        make_shared<SharedSPI>(RJ_SPI_MOSI, RJ_SPI_MISO, RJ_SPI_SCK);
    sharedSPI->format(8, 0);
    kicker = std::make_unique<KickerBoard>(sharedSPI, RJ_KICKER_nCS, RJ_KICKER_nRESET,
                                                  "/local/rj-kickr.nib");  // nCs, nReset
    bool kickerReady = kicker->flash(false, true);
    printf("Flashed kicker, success = %s\r\n", kickerReady ? "TRUE" : "FALSE");

    char getCmd;

    string response;
    bool success = false;
    bool data_dump = false;
    bool charging = false;

    while (true) {

        if (pc.readable()) {
            getCmd = pc.getc();

            pc.printf("%c: ", getCmd);
            switch (getCmd) {
                case 'k':
                    response = "Kicked";
                    success = kicker->kick(DB_KICK_TIME);
                    break;
                case 'r':
                    response = "Read Voltage";
                    uint8_t volts;
                    success = kicker->read_voltage(&volts);
                    response += ", voltage: " + to_string(volts);
                    break;
                case 'd':
                    // data dump voltages
                    response = "Toggled Logging";
                    data_dump = !data_dump;
                    set_logging(data_dump);
                    success = true;
                    break;
                case 'c':
                    response = "Toggle charging";
                    // toggle charging
                    charging = !charging;
                    success = charging ? kicker->charge() : kicker->stop_charging();
                    break;
                case 'p':
                    response = "Pinged";
                    success = kicker->is_pingable();
                    break;
                default:
                    response = "Invalid command";
                    break;
            }

            response += ", success: " + bool_to_string(success);
            pc.printf(response.c_str());
            pc.printf("\r\n");
            fflush(stdout);


        }
        Thread::wait(100);
    }
}
