#include "i2c-lcd.h"
#include <stdio.h>
extern I2C_HandleTypeDef hi2c1;

#define SLAVE_ADDRESS_LCD 0x4E    // Definira adresu slave uređaja (LCD) na I2C sabirnici.

// Funkcija za slanje komande LCD-u.
void lcd__cmd (char cmd)
{
    char data_u, data_l;
    uint8_t data_t[4];
    data_u = (cmd & 0xf0);
    data_l = ((cmd << 4) & 0xf0);
    data_t[0] = data_u | 0x0C;
    data_t[1] = data_u | 0x08;
    data_t[2] = data_l | 0x0C;
    data_t[3] = data_l | 0x08;
    HAL_I2C_Master_Transmit(&hi2c1, SLAVE_ADDRESS_LCD, (uint8_t *) data_t, 4, 100);
}

// Funkcija za slanje podataka LCD-u.
void lcd__data (char data)
{
    char data_u, data_l;
    uint8_t data_t[4];
    data_u = (data & 0xf0);
    data_l = ((data << 4) & 0xf0);
    data_t[0] = data_u | 0x0D;
    data_t[1] = data_u | 0x09;
    data_t[2] = data_l | 0x0D;
    data_t[3] = data_l | 0x09;
    HAL_I2C_Master_Transmit(&hi2c1, SLAVE_ADDRESS_LCD, (uint8_t *) data_t, 4, 100);
}

// Funkcija za čišćenje LCD zaslona.
void lcd_clear (void)
{
    lcd__cmd (0x01);
    HAL_Delay(2);
}

// Funkcija za postavljanje kursora na određenu poziciju na LCD zaslonu.
void lcd_put_cur(int row, int col)
{
    switch (row)
    {
        case 0:
            col |= 0x80;
            break;
        case 1:
            col |= 0xC0;
            break;
    }

    lcd__cmd(col);
}

// Funkcija za inicijalizaciju LCD zaslona.
void lcd_init (void)
{
    HAL_Delay(50);
    lcd__cmd(0x30);
    HAL_Delay(5);
    lcd__cmd(0x30);
    HAL_Delay(1);
    lcd__cmd(0x30);
    HAL_Delay(10);
    lcd__cmd(0x20);
    HAL_Delay(10);

    // Postavlja LCD parametre.
    lcd__cmd(0x28);
    HAL_Delay(1);
    lcd__cmd(0x08);
    HAL_Delay(1);
    lcd__cmd(0x01);
    HAL_Delay(2);
    lcd__cmd(0x06);
    HAL_Delay(1);
    lcd__cmd(0x0C);
}

// Funkcija za prikazivanje stringa na LCD zaslonu.
void lcd__string (char *str)
{
    while (*str) lcd__data (*str++);
}

// Funkcija za prikazivanje udaljenosti na LCD zaslonu.
void lcd_print_distance(uint32_t Distance)
{
    char buf[16];
    lcd_put_cur(0, 0);
    lcd__string(" Udaljenost je");
    lcd_put_cur(1, 4);
    sprintf(buf, " %3lu cm", Distance);

    lcd__string(buf);
}
