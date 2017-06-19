#pragma once

#include <avr/io.h>

/* Inputs */
#define N_KICK_CS_PIN (PA6)
#define KICK_MOSI_PIN (PA4)
#define V_MONITOR_PIN (PA0)
// active low buttons
#define DB_KICK_PIN (PA3)  // should send a normal kick commmand
// #define DB_CHIP_PIN (PORT)  // should send a normal chip command
#define DB_CHG_PIN (PA1)   // pressed = enable_charging
                           // unpressed = disable_charging
//#define DB_SWITCH (PORTA3)

/* Outputs */
#define CHARGE_PIN (PB2)
// #define CHIP_PIN (PORT)
#define KICK_PIN (PB1)

/* Tri-State */
#define KICK_MISO_PIN (PA2)

/* Interrupts for PCMASK0 or PCMASK1 */
// #define INT_N_KICK_CS (PCINT7)
#define INT_DB_KICK (PCINT3)
// #define INT_DB_CHIP (PCINT9)
#define INT_DB_CHG (PCINT1)
