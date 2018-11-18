#pragma once

typedef void (*timer_cb_t)(void);

void timer_configure(unsigned char TIMER_ID, unsigned char time);
void timer_enable(unsigned char TIMER_ID);
void timer_disable(unsigned char TIMER_ID);
void timer_set_callback(unsigned char TIMER_ID, timer_cb_t timer_cb);

