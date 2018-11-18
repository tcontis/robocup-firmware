#pragma once

#include <avr/io.h>

// SPI/Flash
#define SPI_SCK  	(PB7)
#define SPI_MISO 	(PB6)
#define SPI_MOSI 	(PB5)
#define SPI_SS		(PB4)

// LT3751
#define LT_CHARGE	(PD0)
#define LT_DONE		(PD1)
#define LT_FAULT	(PD2)

// Breakbeam
#define BB_RX		(PA0)
#define BB_TX		(PD4)

// Status LEDs
#define MCU_INIT	(PD3)
#define MCU_MODE	(PD5)
#define MCU_ERR		(PD6)

#define BB_ACTIVE	(PB7)

#define HV_IND_MIN	(PA1)
#define HV_IND_LOW	(PA2)
#define HV_IND_MID	(PA3)
#define HV_IND_HIGH	(PA4)
#define HV_IND_MAX	(PA5)

// Vmon
#define HV_VMON		(PA6)

// Debug Inputs
#define SW_DBG_MODE	(PC0)
#define SW_CHARGE	(PC1)
#define SW_KICK		(PC2)
#define SW_CHIP		(PC3)

/* Inputs */
#define N_KICK_CS_PIN (PA6)
#define KICK_MOSI_PIN (PA4)
#define V_MONITOR_PIN (PA0)

#define DB_KICK_PIN (PA3)
#define DB_CHG_PIN (PA1)
/* Outputs */
#define CHARGE_PIN (PB2)
// #define CHIP_PIN (PORT)
#define KICK_PIN (PB1)
#define BALL_SENSE_TX (PB4)
#define BALL_SENSE_RX (PB3)
#define DB_SWITCH (PB5)

/* Tri-State */
#define KICK_MISO_PIN (PA2)

/* Interrupts for PCMASK0 or PCMASK1 */
#define INT_DB_KICK (PCINT3)
#define INT_DB_CHG (PCINT1)
