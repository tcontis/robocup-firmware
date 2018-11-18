#include "io.h"
#include <avr/io.h>
#include "pins.h"

inline void init_io() {
	DDRD |= _BV(LT_CHARGE);
	DDRD &= (~(_BV(LT_DONE)));
	DDRD &= (~(_BV(LT_FAULT)));
	DDRD |= _BV(MCU_INIT);
	DDRD |= _BV(BB_TX);
	DDRD |= _BV(MCU_MODE);
	DDRD |= _BV(MCU_ERR);
	DDRD |= _BV(BB_ACTIVE);

	DDRA &= (~(_BV(BB_RX)));
	DDRA |= _BV(HV_IND_MIN);
	DDRA |= _BV(HV_IND_LOW);
	DDRA |= _BV(HV_IND_MID);
	DDRA |= _BV(HV_IND_HIGH);
	DDRA |= _BV(HV_IND_MAX);
	DDRA &=(~(_BV(HV_VMON)));

	DDRB |= _BV(SPI_SS);
	DDRB |= _BV(SPI_MOSI);
	DDRB &= (~(_BV(SPI_MISO)));
	DDRB |= _BV(SPI_SCK);

	DDRC &= (~(_BV(SW_DBG_MODE)));
	DDRC &= (~(_BV(SW_CHARGE)));
	DDRC &= (~(_BV(SW_KICK)));
	DDRC &= (~(_BV(SW_CHIP)));
	DDRC |= _BV(KICK);
	DDRC |= _BV(CHIP);
} 





