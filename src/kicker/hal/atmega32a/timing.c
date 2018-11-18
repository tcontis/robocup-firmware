#include "hal/timing.h"

#include <avr/io.h>

#pragma GCC push_options
#pragma GCC optimize ("O3")

const unsigned char TIMER_ID_0 = 0x00;
const unsigned char TIMER_ID_1 = 0x01;
const unsigned char TIMER_ID_2 = 0x02;

inline void timer_configure(unsigned char TIMER_ID, unsigned char time) {

} __attribute__((always_inline, optimize("-03")));

inline void timer_enable(unsigned char TIMER_ID) {

};

inline void timer_disable(unsigned char TIMER_ID) {

}

inline void timer_set_callback(unsigned char TIMER_ID, timer_cb_t timer_cb) {

}

#pragma GCC pop_options

