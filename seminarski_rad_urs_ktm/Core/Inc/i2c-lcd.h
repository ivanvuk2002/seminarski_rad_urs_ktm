#ifndef __I2C_LCD_H
#define __I2C_LCD_H

#include "stm32f4xx_hal.h"

void lcd_init(void);
void lcd__cmd(char cmd);
void lcd__data(char data);
void lcd__string(char *str);
void lcd_clear(void);
void lcd_put_cur(int row, int col);
void lcd_print_distance(uint32_t distance);

#endif /* __I2C_LCD_H */
