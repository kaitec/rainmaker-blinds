#ifndef FLASH_H
#define FLASH_H

void flash_init(void);
void write_flash(uint16_t value);
uint16_t read_flash(void);

#endif