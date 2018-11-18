/**
 * This file defines includes for basic system functions.
 */

/**
 * Disables maskable interrupts globally.
 * 
 * @param NONE
 * @return NONE
 */
void enable_interrupts();

/** 
 * Enables maskable interrupts globally.
 *
 * @param NONE
 * @return NONE
 */
void disable_interrupts();

/**
 * Locks the main execution thread but leaves interrupts
 * enabled (e.g. ISR still fire)
 */
void spin();

/**
 * Locks the main execution thread after globally disabling
 * interrupts. Will halt the system until external sys rst.
 */
void halt();

