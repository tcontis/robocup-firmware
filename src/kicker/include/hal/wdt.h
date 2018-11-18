#pragma once

/**
 * Sets the interval of the watchdog timer.
 *
 * @param <char> time
 * @return NONE
 */
void watchdog_interval(char time);

/**
 * Enables the watchdog.
 *
 * @param NONE
 * @return NONE
 */
void watchdog_enable();

/**
 * Disables the watchdog.
 *
 * @param NONE
 * @return NONE
 */
void watchdog_disable();

/**
 * Feeds the watchdog. If the watchdog isn't fed
 * by every interval specified by watchdog_interval()
 * it will fire the reset vector, rebooting the device.
 */
void watchdog_feed();

