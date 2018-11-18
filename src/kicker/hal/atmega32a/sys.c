#include "hal/sys.h"

#include <avr/interrupt.h>

inline void enable_interrupts() {
	sei();
}

inline void disable_interrupts() {
	cli();
}

inline void spin() {
	for (;;);
}

inline void halt() {
	disable_interrupts();
	spin();
}
