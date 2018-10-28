#pragma once

#include <avr/io.h>
#include <avr/interrupt.h>

#include "pins.h"

///////////////////////////////////////////////////////////
//  AGNOSTIC MACROS
///////////////////////////////////////////////////////////

#define ENABLE_INTERRUPTS() sei()
#define DISABLE_INTERRUPTS() cli()

#define CLEAR_REG(reg) (reg = 0x00)

#define MASK_REG_SET_BIT(reg, bit) (reg |= _BV(bit))
#define MASK_REG_CLR_BIT(reg, bit) (reg &= ~(_BV(bit)))

#define BANK_SET_PIN_DIR_OUT(bank, pin) MASK_REG_SET_BIT(bank, pin)
#define BANK_A_SET_PIN_DIR_OUT(pin) BANK_SET_PIN_DIR_OUT(DDRA, pin) 
#define BANK_B_SET_PIN_DIR_OUT(pin) BANK_SET_PIN_DIR_OUT(DDRB, pin) 
#define BANK_C_SET_PIN_DIR_OUT(pin) BANK_SET_PIN_DIR_OUT(DDRC, pin) 
#define BANK_D_SET_PIN_DIR_OUT(pin) BANK_SET_PIN_DIR_OUT(DDRD, pin) 

#define BANK_SET_PIN_DIR_IN(bank, pin) MASK_REG_CLR_BIT(bank, pin)
#define BANK_A_SET_PIN_DIR_IN(pin) BANK_SET_PIN_DIR_IN(DDRA, pin) 
#define BANK_B_SET_PIN_DIR_IN(pin) BANK_SET_PIN_DIR_IN(DDRB, pin) 
#define BANK_C_SET_PIN_DIR_IN(pin) BANK_SET_PIN_DIR_IN(DDRC, pin) 
#define BANK_D_SET_PIN_DIR_IN(pin) BANK_SET_PIN_DIR_IN(DDRD, pin)

#define SPI_RX_CALLBACK__isr ISR(SPI_STC_vect)
#define TIMER_CALLBACK__isr  ISR(TIMER1_COMPA_vect)

#define SPIN() for(;;);
#define HALT_SPIN() DISABLE_INTERRUPTS(); SPIN(); 



///////////////////////////////////////////////////////////
//  KICKER_MACROS
///////////////////////////////////////////////////////////


