#include "hal/wdt.h"

#include <avr/io.h>
#include <avr/wdt.h>

const unsigned char WDT_INT_16MS = 0x00;
const unsigned char WDT_INT_32MS = 0x01;
const unsigned char WDT_INT_65MS = 0x02;
const unsigned char WDT_INT_130MS = 0x03;
const unsigned char WDT_INT_260MS = 0x04;
const unsigned char WDT_INT_560MS = 0x05;
const unsigned char WDT_INT_1S = 0x06;
const unsigned char WDT_INT_2S = 0x07;

__attribute__((always_inline))
inline void watchdog_interval(char val) {
	WDTCR |= (val & 0x03);
}

__attribute__((always_inline))
inline void watchdog_enable() {
	WDTCR |= (_BV(WDE));
}

__attribute__((always_inline))
inline void watchdog_disable() {
	WDTCR |= (_BV(WDTOE) | _BV(WDE));
	WDTCR &= (~(_BV(WDE)));
}

__attribute__((always_inline))
inline void watchdog_feed() {
	__asm__ volatile("wdr");
}

