#include <avr/io.h>
#include <avr/wdt.h>

#include "pins.h"
#include "kmacros.h"

void init() {
	DISABLE_INTERRUPTS();

	wdt_reset();
	MASK_REG_SET_BIT(WDTCR, WDTOE);
	MASK_REG_SET_BIT(WDTCR, WDE);
	CLEAR_REG(WDTCR);

//	MASK_REG_SET_BIT(CLKPR, CLKPCE);
//	CLEAR_REG(CLKPR);	

	ENABLE_INTERRUPTS();
}

void config_io() {
	PORTD |= _BV(MCU_INIT);
}

void main() {
	init();
	config_io();

	SPIN();
}
