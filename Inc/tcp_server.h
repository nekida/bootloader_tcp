#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_
//-----------------------------------------------
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "lwip.h"
#include "lwip/tcp.h"
//-----------------------------------------------
void tcp_server_init(void);
void sendstring(char* buf_str);
void tcp_data_send(char* buf_str);
//-----------------------------------------------

//-----------------------------------------------
#endif
