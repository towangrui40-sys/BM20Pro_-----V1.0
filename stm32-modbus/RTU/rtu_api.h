#ifndef _RTU_API_H_
#define _RTU_API_H_

//================================
//		缓冲区有关的结构体
//================================
typedef struct
{
	unsigned char  Remote_Addr;
	unsigned char  funcode;
	unsigned short  addr;
	unsigned short  len;
	unsigned short  Data[128];
	unsigned short  Crc16;
} API_StructDef;

typedef struct
{
	unsigned short Data[128];
} API_Data_StructDef;

//================================
//		Funtions Pointer
//================================
typedef void (*API_Callback)( API_StructDef *       request
                            , API_Data_StructDef *   response_data
);

typedef void (*API_DataReceived)( unsigned char* buffer
                                , unsigned char  size
);
//================================
//		Marco Defined
//================================
#define API_OK      0x00
#define API_ERROR   0x01
//#define NULL        0x00

#define MAIN_485ADDR 0XF0
#define MODULE_485ADDR 0XF0
#define HANDLE_485ADDR 0XF1

																
//#define ROLE_485_SLAVE 
																
#ifdef  ROLE_485_SLAVE													
#define M_TARGET_ADDR MODULE_485ADDR
#define M_SOURCE_ADDR MAIN_485ADDR

#define H_TARGET_ADDR HANDLE_485ADDR
#define H_SOURCE_ADDR	MODULE_485ADDR	

#else

#define M_TARGET_ADDR MAIN_485ADDR
#define M_SOURCE_ADDR MODULE_485ADDR

#define H_TARGET_ADDR HANDLE_485ADDR
#define H_SOURCE_ADDR	MODULE_485ADDR	

#endif																


#define RTU_FUNC_CODE_RD 			0x03
#define RTU_FUNC_CODE_WR 			0X06
#define RTU_FUNC_CODE_CWR 		0X10
#define RTU_FUNC_CODE_ERROR 	0X83

//================================
//		Funtions
//================================

/** Change community key
  * return :
 **/
/**
 * Change community key
 */
unsigned char* API_ChangeKey( void );

unsigned char* API_BuildPackage( API_StructDef *api);
                                
unsigned char* API_Response( unsigned char* RxBuffer
);    

unsigned char  API_SerailLoop( unsigned char     rx_data
                             , API_DataReceived  rx_callback
);
unsigned char  API_SerailLoopAddrFix( unsigned char     rx_data
                             , API_DataReceived  rx_callback
);
void API_Callback_Handler(API_StructDef* request, API_Data_StructDef* response_data);
#endif 
