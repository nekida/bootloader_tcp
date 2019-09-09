#include "iap_tcp.h"
#include "flash_if.h"
#include "main.h"

extern UART_HandleTypeDef huart1;

static __IO uint32_t flash_first_address;

static void iap_tcp_write_data(char * data_for_write, uint32_t len_data)
{
	uint32_t count_full_32_word = 0, count_half_32_word = 0;
	
	count_half_32_word = len_data % 4;
	/*
	if(count_half_32_word > 0)
	{
		switch(count_half_32_word)
		{
			case(1):
				data_for_write[len_data] 			= 0xFF;
				data_for_write[len_data + 1] 	= 0xFF;
				data_for_write[len_data + 2] 	= 0xFF;
				break;
			
			case(2):
				data_for_write[len_data] 			= 0xFF;
				data_for_write[len_data + 1] 	= 0xFF;
				break;
			
			case(3):
				data_for_write[len_data] 			= 0xFF;
				break;
		}
	}*/
	count_full_32_word = len_data / 4;
	HAL_UART_Transmit(&huart1, (uint8_t*)data_for_write, len_data, HAL_MAX_DELAY);
	//FLASH_If_Write(&flash_first_address, (uint32_t *)data_for_write, count_full_32_word );
}

void iap_tcp_start(char * ptr, uint32_t len)
{
	static uint8_t flg_flash_init = 1;
	if(flg_flash_init == 0)
	{
		flg_flash_init = 1;
		FLASH_If_Init();
		FLASH_If_Erase(USER_FLASH_FIRST_PAGE_ADDRESS);
		flash_first_address = USER_FLASH_FIRST_PAGE_ADDRESS;
	}
	iap_tcp_write_data(ptr, len);
	//HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
}
