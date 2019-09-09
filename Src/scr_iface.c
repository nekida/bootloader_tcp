#include "scr_iface.h"
#include "stdlib.h"
#include "stdbool.h"
#include "string.h"
#include "stm32f7xx_hal_crc.h"
#include "fmc.h"
#include "scr_flash.h"
#include "scr_crc.h"
#include <math.h>
#include "flash_if.h"
#include "tcp_server.h"

#define MAX_SECTOR			1280
#define BYTE_IN_PAGE		2048
#define PAGE_IN_BLOCK		64
#define BYTE_IN_BLOCK		(BYTE_INPAGE * PAGE_IN_BLOCK)
#define SECTOR_ON_PAGES	20

#define SERVICE_BYTES_SHIFT	8

extern UART_HandleTypeDef huart1;
extern CRC_HandleTypeDef 	hcrc;

struct strct_crc{
	uint32_t crc_msg;
	uint32_t crc_calc;
};

/*
*function:		hardware check crc32
*parameters:	array data, number bytes in array
*return:			crc32 two parts array
*note:				first call create crc1 for further calculation. 
							No use returned value in first call function
*/
uint32_t check_crc_on_parts(uint32_t * array, uint32_t number_bytes)
{
	static struct strct_crc{
		uint32_t temp;
		uint32_t result;
	} crc;

	static bool flg_first_crc = true;
	
	if(flg_first_crc)
	{
		flg_first_crc = false;
		crc.temp 			= ~HAL_CRC_Calculate(&hcrc, array, number_bytes);
		crc.result 		= 0;
	}
	else
	{
		crc.result 	= crc32_combine_(crc.temp, (~HAL_CRC_Calculate(&hcrc, array, number_bytes)), number_bytes);
		crc.temp 		= crc.result;
	}
	return crc.result;
}

void erase_block_start_with_page(uint16_t block, uint16_t start_page)
{
	NAND_AddressTypeDef erase_address;
	
	erase_address.Block = block;
	erase_address.Page 	= start_page;
	HAL_NAND_Erase_Block(&hnand1, &erase_address);
}

void choise_mode(char * data, uint32_t len)
{
	static bool flg_first_starting = false;
	static uint8_t mode = 0;
	
	if(!flg_first_starting)
	{
		flg_first_starting = true;
		mode = data[4];
	}
	
	if(mode == 0)
	{
		write_on_flash(data, len);
	}
	
	if( (mode == 30) || (mode == 40) )
	{
		mode_simple(data, len);
	}
}

void mode_simple(char * data, uint32_t len)
{
	static bool flg_first_staring 						= false;
	static uint8_t command 										= 0;
	static uint32_t cnt_bytes_firmware_in_msg = 0;
	static struct strct_crc crc;
	
	if(!flg_first_staring)
	{
		command 									= data[4];
		cnt_bytes_firmware_in_msg = data[5] | (data[6] << 8) | (data[7] << 16);
		crc.crc_msg								= data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
	}
	
	crc.crc_calc = ~HAL_CRC_Calculate(&hcrc, (uint32_t *)&data[4], (len - 4));
	
	if(crc.crc_calc == crc.crc_msg)
	{
		switch(command)
		{
			case(30):
				FLASH_If_Init();
				erase_sector(USER_FLASH_FIRST_PAGE_ADDRESS, FLASH_SECTOR_4, 1);
				HAL_FLASH_Lock();
				clear_key();
				send_str_if_flg_open_connect("memory erased");
				wait_time_and_listen(10);
				NVIC_SystemReset();
				break;
			
			case(40):
				send_str_if_flg_open_connect("reset");
				wait_time_and_listen(15);
				NVIC_SystemReset();
				break;
		}
	}
}

void write_on_flash(char * data, uint32_t len)
{
	static struct strct_crc crc;
	
	static uint32_t cnt_bytes_firmware_in_msg = 0;
	static uint32_t cnt_bytes_firmware_rx 		= 0;
	static uint32_t cnt_bytes_for_page 				= 0;
	
	static uint32_t needed_amount_pages = 0;
	static uint32_t bytes_not_enough_for_full_page = 0;
	
	static bool flg_create_dynamic_arr 				= false;
	static bool flg_erase_and_init_variables 	= false;
	static bool flg_first_page								= true;
	
	static char * data_receive;
	static uint8_t item_page = 0;
	static NAND_AddressTypeDef	write_address;
	
	static uint8_t command = 0;
			
	if(!flg_erase_and_init_variables)		//очищаем флеш и парсим сообщение
	{
		flg_erase_and_init_variables = true;
		
		command 									= data[4];
		cnt_bytes_firmware_in_msg = data[5] | (data[6] << 8) | (data[7] << 16);
		crc.crc_msg								= data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
		
		needed_amount_pages 						= (uint32_t)ceil((double)cnt_bytes_firmware_in_msg / (double)BYTE_IN_PAGE);	//получаем нужное количество страниц nand
		bytes_not_enough_for_full_page 	= cnt_bytes_firmware_in_msg % BYTE_IN_PAGE;											//получаем количество байт на неполную страницу
		
		//очищаем блок с нуля
		erase_block_start_with_page(0, 0);
	}
	
	if(!flg_create_dynamic_arr)		//создание массива размером со страницу флэшки
	{
		flg_create_dynamic_arr 	= true;
		data_receive 						= (char *)calloc(BYTE_IN_PAGE, sizeof(char));	//массив размером со траницу nand
	}
	
	for(int i = 0; i < len; i++)	//заполнение массива
	{
		data_receive[cnt_bytes_for_page + i] = data[i];
	}	
	
	cnt_bytes_for_page 		+= len;	//набираем полную страницу
	cnt_bytes_firmware_rx += len;	//считаем принятые байты
	
	if(cnt_bytes_for_page == BYTE_IN_PAGE)	//если набрали полную страницу
	{
		write_address.Page = item_page;
		cnt_bytes_for_page = 0;
		
		flg_create_dynamic_arr = false;
		
		HAL_NAND_Write_Page(&hnand1, &write_address, (uint8_t *)data_receive, 1);
		
		item_page++;
			
		if(flg_first_page)
		{
			flg_first_page	= false;
			check_crc_on_parts((uint32_t *)(&data_receive[SERVICE_BYTES_SHIFT]), (BYTE_IN_PAGE - SERVICE_BYTES_SHIFT));
			free(data_receive);
		}
		else
		{
			crc.crc_calc = check_crc_on_parts((uint32_t *)data_receive, BYTE_IN_PAGE);
			free(data_receive);
		}	
	}
	else if(cnt_bytes_for_page == bytes_not_enough_for_full_page)	//иначе пишем неполную
	{
		write_address.Page = item_page;
		
		HAL_NAND_Write_Page(&hnand1, &write_address, (uint8_t *)data_receive, 1);
		
		crc.crc_calc = check_crc_on_parts((uint32_t *)data_receive, bytes_not_enough_for_full_page);
		free(data_receive);
	}		
	
	if(cnt_bytes_firmware_in_msg == cnt_bytes_firmware_rx)
	{
		send_str_if_flg_open_connect("firmware accepted\n\r");
		if(crc.crc_calc == crc.crc_msg)
		{		
			send_str_if_flg_open_connect("crc correct. update firmware and reboot\n\r");
			wait_time_and_listen(15);	
			from_nand_to_flash_with_key(cnt_bytes_firmware_in_msg, needed_amount_pages, bytes_not_enough_for_full_page);
		}
		else
		{
			send_str_if_flg_open_connect("crc not correct\n\r");
			wait_time_and_listen(15);	
		}
	}
}

void from_nand_to_flash_with_key(uint32_t len_firmware, uint32_t cnt_pages_nand, uint32_t bytes_not_enough_for_full_page)
{
	bool flg_first_page_nand = true;
	uint8_t temp_array[BYTE_IN_PAGE];
	NAND_AddressTypeDef	read_address;
	static __IO uint32_t flash_first_address;
	
	uint32_t count_full_32_word = 0;
	
	while(cnt_pages_nand)
	{
		if(flg_first_page_nand)		//если первая страница
		{
			flg_first_page_nand = false;
			
			FLASH_If_Init();
			FLASH_If_Erase(USER_FLASH_FIRST_PAGE_ADDRESS);
			flash_first_address = USER_FLASH_FIRST_PAGE_ADDRESS;
			
			read_address.Block 	= 0;
			read_address.Page 	= 0;
			HAL_Delay(100);
			HAL_NAND_Read_Page(&hnand1, &read_address, temp_array, 1);
			
			count_full_32_word = (BYTE_IN_PAGE - SERVICE_BYTES_SHIFT) / 4;
			FLASH_If_Write(&flash_first_address, (uint32_t *)&temp_array[SERVICE_BYTES_SHIFT], count_full_32_word );
			
			read_address.Page++;
			len_firmware -= BYTE_IN_PAGE;
		}
		else if(len_firmware >= BYTE_IN_PAGE)	//если хватает на полную страницу
		{
			HAL_Delay(100);
			HAL_NAND_Read_Page(&hnand1, &read_address, temp_array, 1);
			
			count_full_32_word = BYTE_IN_PAGE/ 4;
			FLASH_If_Write(&flash_first_address, (uint32_t *)temp_array, count_full_32_word );
			
			read_address.Page++;
			len_firmware -= BYTE_IN_PAGE;
			
			if( (len_firmware == 0) && (bytes_not_enough_for_full_page == 0) ) //если прошивка выровнена по границе страницы флешки и нет больше байт для записи
			{
				write_key_to_flash();
				NVIC_SystemReset();
			}
		}
		else if(bytes_not_enough_for_full_page != 0)												//пишем неполную последнюю
		{
			HAL_Delay(100);
			HAL_NAND_Read_Page(&hnand1, &read_address, temp_array, 1);
			
			count_full_32_word = bytes_not_enough_for_full_page / 4;
			FLASH_If_Write(&flash_first_address, (uint32_t *)temp_array, count_full_32_word );

			write_key_to_flash();
			NVIC_SystemReset();
		}
		cnt_pages_nand--;
	}
}
