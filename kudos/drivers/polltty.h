/*
 * Polling TTY driver.
 */

#ifndef POLLTTY_H
#define POLLTTY_H

/* Initialize the polling TTY driver */
void polltty_init();

/* Get one character from the polling TTY */
int polltty_getchar();

/* Output one character to the TTY */
void polltty_putchar(int c);

#endif /* POLLTTY_H */
