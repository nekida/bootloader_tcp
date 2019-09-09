#ifndef SCR_FLASH_H_
#define SCR_FLASH_H_

#include "stm32f7xx_hal.h"

#define KEY_ADDRESS										(uint32_t)0x08010000
#define KEY_VALUE											0xAABBCCDD
#define ERASED_MEMORY									0xFFFFFFFF
#define USER_FLASH_FIRST_PAGE_ADDRESS 0x08020000
#define USER_FLASH_LAST_PAGE_ADDRESS  0x080C0000
#define USER_FLASH_END_ADDRESS        0x080FFFFF

void write_key_to_flash(void);
void clear_key(void);
uint32_t read_key_from_flash(void);
int8_t erase_sector(uint32_t start_address, uint32_t start_sector, uint32_t number_sector);

#endif 
