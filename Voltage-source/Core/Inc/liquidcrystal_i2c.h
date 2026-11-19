/*
 * liquidcrystal_i2c.h
 *
 *  Created on: Sep 1, 2026
 *      Author: josef
 */

#ifndef INC_LIQUIDCRYSTAL_I2C_H_
#define INC_LIQUIDCRYSTAL_I2C_H_

#include "stm32f3xx_hal.h"

// Externí proměnná pro I2C vygenerovaná v main.c
extern I2C_HandleTypeDef hi2c1;

// Základní I2C adresa
#define SLAVE_ADDRESS_LCD 0x4E

void lcd_init(void);
void lcd_send_cmd(char cmd);
void lcd_send_data(char data);
void lcd_clear(void);
void lcd_setCursor(int row, int col);
void lcd_print(char *str);

#endif /* INC_LIQUIDCRYSTAL_I2C_H_ */
