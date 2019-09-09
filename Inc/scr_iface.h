#ifndef __SCR_IFACE_H
#define __SCR_IFACE_H

#include "stm32f7xx_hal.h"

void mode_simple(char * data, uint32_t len);
void write_on_flash(char * data, uint32_t len);
void choise_mode(char * data, uint32_t len);

void parsing_message_and_check_crc(char * data, uint32_t len);
void write_sending_firmware_on_nand(char * data, uint32_t len);
void write_firmware_to_nand(char * data, uint32_t len);
void test_nand(void);
void read_nand(void);
void erase_all_flash(void);
void parsing_message_and_write_on_flash(char * data, uint32_t len);
void read_flash(void);
void from_nand_to_flash_with_key(uint32_t len_firmware, uint32_t cnt_pages_nand, uint32_t bytes_not_enough_for_full_page);

#endif
