A simple voltmeter project based on the AVR Attiny13. It features dynamic indication on a 7-segment display. The display digits are cycled every 5 milliseconds using Timer 0 in CTC mode with an interrupt and a prescaler of 64. Measurements from ADC1 are performed in the main loop. A standard resistive voltage divider with 10 kΩ and 100 kΩ resistors is used for measurement.

https://github.com/user-attachments/assets/e18203a0-9940-447d-96e3-510df051b6fb

