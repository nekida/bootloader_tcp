# bootloader_tcp
Bootloader STM32F746IGT on Ethernet interface TCP protocol. The adopted firmware is saved to the external NAND memory with the integrity check of the CRC

Загрузчик, записанный во флеш память контроллера через интерфейс SWD/JTAG, ожидает прошивку по интерфейсу Ethernet протоколу TCP. При старте микроконтроллера, загрузчик ждет 10 мс в ожидании прошивки, затем читает специальный ключ по определенному адресу, который сигнализирует о наличии или отсутствии уже принятой прошивки во флеш памяти. При наличии прошивки, загрузчик передает ей управление, при отсутствии - переходит в бесконечный цикл ее ожидания.
Алгоритм: 
подача питания 
-> ожидание 10 мс 
-> чтение ключа и вывод в консоль строки "read key from flash memory" 
-> вывод в консоль строки "key is ready" 
	--> если ключ отсутствует 
	--> ожидание прошивки и вывод в консоль строки "firmware not detected. wait firmware"
		---> если прошивка была отправлена 
		---> запись принимаемой прошивки во внешнюю nand память с аппаратным 			расчетом crc ее частей (для корректного получения итоговой crc, которую в конце 			сообщения считает скрипт отправки) и вывод в консоль строки "firmware accepted" 
		---> если crc всего сообщения корректно 
			----> вывод в консоль строки "crc correct. update firmware and reboot", 				стирание страниц флеш памяти микроконтроллера и запись принятой 				прошивки 
			----> программный сброс микроконтроллера для старта полученной 				прошивки 
		---> если crc всего сообщения некорректно ----> вывод в консоль строки "crc not 			correct"
	--> если ключ прочитан -> вывод в консоль строки "firmware detected. starting firmware" и 	старт принятой прошивки

Примечание:
-Проект для Keil’a: sul.uvprojx
-Проект уже скомпилирован ARM компилятором
-Отправка прошивки (бинарного файла) осуществляется скриптом на языке Python.
-Сообщения доступны только при подключении терминала. С неподключенным терминалом, загрузчик будет работать аналогично, только без печати сообщений.
-При подключении терминала будет выдано сообщение "bootloader server accept" и "waiting for firmware 10 seconds".
-Микроконтроллер выступает в роли сервера.
-Микроконтроллер имеет IP адрес 192.168.0.10 и слушает порт 80.
-Для того, чтобы прошивка правильно стартовала, необходимо, чтобы она была слинкована с определенными адресами. Для этого в проекте прошивки, которую получит загрузчик, перед сборкой в опциях проекта на вкладке Target внизу справа в графе IROM прописать: Start = 0x8080000 и Size = 0x1FFFF. 
Для смещения таблицы векторов: Drivers/CMSIS/system_stm32f7xx.c 
								#define VECT_TAB_OFFSET  0x80000
Все справедливо для проекта, сгенерированного STM32CubeMX и разработанного в IDE Keil Uvision.
