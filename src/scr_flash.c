#include "scr_flash.h"
#include "flash_if.h"
#include "main.h"
#include "stm32f7xx_hal_flash.h"

static __IO uint32_t flash_first_address;

int8_t erase_sector(uint32_t start_address, uint32_t start_sector, uint32_t number_sector)
{
  uint32_t FlashAddress;
 
  FlashAddress = start_address;

  /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
     be done by word */ 
 
  if (FlashAddress <= (uint32_t) USER_FLASH_LAST_PAGE_ADDRESS)
  {
    FLASH_EraseInitTypeDef FLASH_EraseInitStruct;
    uint32_t sectornb = 0;
    
    FLASH_EraseInitStruct.TypeErase 		= FLASH_TYPEERASE_SECTORS; /*! <Массовое стирание или стирание сектора. Этот параметр может иметь значение @ref FLASHEx_Type_Erase */
    FLASH_EraseInitStruct.Sector 				= start_sector;						/*! <Начальный сектор FLASH для стирания, когда Mass Erase отключено. Этот параметр должен иметь значение @ref FLASHEx_Sectors */
    FLASH_EraseInitStruct.NbSectors 		= number_sector; /*!<Количество секторов, которые нужно стереть. Этот параметр должен иметь значение от 1 до (максимальное количество секторов - значение исходного сектора)*/
    FLASH_EraseInitStruct.VoltageRange 	= FLASH_VOLTAGE_RANGE_3; /*! <Диапазон напряжения устройства, который определяет параллельность стирания. Этот параметр должен иметь значение @ref FLASHEx_Voltage_Range */
    
    if (HAL_FLASHEx_Erase(&FLASH_EraseInitStruct, &sectornb) != HAL_OK)
      return (1);
  }
  else
  {
    return (1);
  }

  return (0);
}

void clear_key(void)
{
	FLASH_If_Init();
	erase_sector(KEY_ADDRESS, FLASH_SECTOR_2, 1);
	HAL_FLASH_Lock();
}

uint32_t read_key_from_flash(void)
{
	return (*(__IO uint32_t *)KEY_ADDRESS);
}

void write_key_to_flash(void)
{
	static uint32_t key_boot = KEY_VALUE;
	flash_first_address = KEY_ADDRESS;
	
	FLASH_If_Init();
	FLASH_If_Write(&flash_first_address, &key_boot, 1);
	HAL_FLASH_Lock();
}
