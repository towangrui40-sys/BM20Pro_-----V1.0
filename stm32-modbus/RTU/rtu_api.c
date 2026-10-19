#include "sys.h"
#include "crc_calc16.h"
#include "rtu_api.h"
#include "usart.h"
#include "rtu_protocol.h"
#include "rtu_process.h"
#include "string.h"

API_StructDef      api_response;
API_Data_StructDef api_data;
unsigned char      api_encrtypt_data[20];


unsigned char  API_CheckSum( const unsigned char* buffer_address
                           , unsigned char        size
){
	char i = 0;
	char buffer_sum = 0;
	for (i = 0; i < size - 1; i++) {
		buffer_sum += buffer_address[i];
	}
	return buffer_sum;
}

unsigned char  API_CheckCrc8( unsigned char* buffer_address
                           , unsigned char        size
){
	char i = 0;
	char buffer_crc8 = 0;
	while(size--)
	{
		buffer_crc8^=*buffer_address++;
		for(i=0;i<8;i++)
		{
			if(buffer_crc8&0x01)buffer_crc8=(buffer_crc8>>1)^0x8C;
			else buffer_crc8 >>= 1;
		}
	}
	return buffer_crc8;
}
unsigned short API_CheckCrc16(unsigned char* buffer_address
                           , unsigned char        size)
{
	unsigned short buffer_crc16 = 0;
	buffer_crc16 =CRC16_MODBUS(buffer_address,size);
	return buffer_crc16;
}
unsigned short API_CheckError(unsigned char* rx) 
{
    unsigned short sum = 0, i;
    for (i=0; i<12; i++) {
        sum += rx[i];
    }
    return sum;
}

#ifdef TEA_CHECK
//
// Build Encrypt Package
//
unsigned short API_BuildSendPackage( unsigned char* desc
                                   , unsigned char* src
                                   , unsigned short size
                                   , unsigned char  type
){
    static unsigned char buffer[20] = {0};
    unsigned char i,buffer_sum = 0;
    
    // Head (3)
    desc[0] = 0x6A;                                   // 1
    desc[1] = 0x55;                                   // 2
    switch (type)                                     // 3
    {
        case 1: desc[2] = 0x31; break;
        case 2: desc[2] = 0x32; break;
        case 3: desc[2] = 0x30; break;
    }
  
    // Key(4) + Data(12) = (16)
    memcpy(buffer, (unsigned char*)&tea_checkkey, 4); // 4-7 :key  (4)
    memcpy(buffer + 4, src, size);                    // 8-19:data (12)
    tea_encrypt(buffer, desc + 3, size, type);        // encrypt

    // Sum (1)
    for (i = 0; i < size + 4 + 3 + 1 - 1; i++) {
        buffer_sum += desc[i];
    }
    desc[size + 4 + 3] = buffer_sum;                  // 20

    return size + 4 + 3 + 1;
}

unsigned char* API_SendKeyError( void )
{
    static unsigned char mErrorKey[12] = {0};
    uint8_t* p = (uint8_t*)&tea_checkkey;
    
    // Data for key 
    mErrorKey[0] = 0x32;
    mErrorKey[1] = 0x32;
    mErrorKey[2] = p[0]; 
    mErrorKey[3] = p[1];
    mErrorKey[4] = p[2];
    mErrorKey[5] = p[3];
    
    // ¼ÓÃÜ
    API_BuildSendPackage(api_encrtypt_data, mErrorKey, 12, 2);
    return api_encrtypt_data;
}
#endif

//unsigned char* API_BuildPackage( API_StructDef *api)
//{
//  static unsigned char local_addr = 0;
//		#ifdef  ROLE_485_SLAVE	
//		local_addr = ReadAddress();
//		#endif
//		local_addr = local_addr|M_TARGET_ADDR;
//	
//		// data init
//		memset((void*)(api_data.Data), 0, 8);
//		
//		// response api request.
//		api_response.Remote_Addr = api->Remote_Addr;
//		api_response.Source_Addr = local_addr;//api->Source_Addr;
//		api_response.Command     = api->Command;				
//	
//		memcpy((void*)(api_response.Data), (void*)(api_data.Data), 8);
//	
//		api_response.Crc8  = API_CheckCrc8((unsigned char*)&api_response, 11);
//        
//    
//	return (unsigned char*)&api_response;
//}

unsigned char* API_Response( unsigned char* RxBuffer                           
){
		u8  i;
		// data init
		memset((void*)(api_response.Data), 0, 128);        
		// call dispose at main
		api_response.Remote_Addr = RxBuffer[0];
		api_response.funcode = RxBuffer[1];	
		api_response.addr = rtu_structure.addr;
		if(api_response.funcode == 0x03)
		{
			api_response.len = RxBuffer[2]/2;;
			for(i = 0;i<api_response.len;i++)
			{
				api_response.Data[i] =  RxBuffer[4+(i*2)]+ (RxBuffer[3+(i*2)]<<8);   
			}			
		}
		else
		{
			api_response.len = RxBuffer[5]+ (RxBuffer[4]<<8);
		}	
		API_Callback_Handler(&api_response, &api_data);
	
	// response api request.
//	api_response.Remote_Addr = api->Remote_Addr;
//	api_response.funcode     = api->funcode;
//	api_response.addr = api->addr;
//	api_response.len = api->len;
//	memcpy((void*)(api_response.Data), (void*)(api_data.Data), api->len*2);         
	return (unsigned char*)&api_response;
}
unsigned char  API_SerailLoop( unsigned char     rx_data
                             , API_DataReceived  rx_callback
){
	static unsigned char  mPkg[256]            = {0};
	static unsigned char rx_state            = 0;
	static unsigned char mPkg_length_counter = 0;
	static unsigned char data_length_counter = 0;
	static unsigned char data_length         = 0;
	unsigned short  recv_crc16 = 0;
		
	switch(rx_state)
    {
        case 0:                                             // device station
		if(rx_data == 0x03)
		{
			mPkg[mPkg_length_counter++] = rx_data;
			rx_state++;					
		}
		else
		{
			rx_state = 0;
			mPkg_length_counter =0;
			memset(mPkg, 0, 256);							
		}
        break;
        case 1:                                             // func code
		switch(rx_data)
		{
			case RTU_FUNC_CODE_RD:                                  //rd
				mPkg[mPkg_length_counter++] = rx_data;
				rx_state++;
			break;
			case RTU_FUNC_CODE_WR:					//wr
				
				mPkg[mPkg_length_counter++] = rx_data;
				data_length = 4;
				data_length_counter = 0;
				rx_state = 3;
			break;
			
			case RTU_FUNC_CODE_CWR:																	//cwr
			case RTU_FUNC_CODE_ERROR:																//errorcode
				mPkg[mPkg_length_counter++] = rx_data;
					//	
				rx_state = 6;
			break;
			default:
			rx_state = 0;
					mPkg_length_counter = 0;
					memset(mPkg, 0, 256);
			break;
        }
        break;
        case 2:                                             // len
			mPkg[mPkg_length_counter++] = rx_data;	
			data_length = rx_data;
			data_length_counter = 0;				
			rx_state++;
        break;
        case 3:                                             // data
            mPkg[mPkg_length_counter++] = rx_data;
						data_length_counter++;
            if(data_length_counter == data_length)
                rx_state++;
        break;
        case 4:                                             // rev crc 
            mPkg[mPkg_length_counter++] = rx_data;
						rx_state++;
        break;
		case 5:										// check crc
			
			mPkg[mPkg_length_counter++] = rx_data;
			recv_crc16 = mPkg[mPkg_length_counter-2]+(mPkg[mPkg_length_counter-1]<<8);
			if (API_CheckCrc16(mPkg, mPkg_length_counter-2) == recv_crc16)
			{
				rx_callback(mPkg , mPkg_length_counter-2);  //save data,back callback function	
				rx_state = 0;
				mPkg_length_counter = 0;
				memset(mPkg, 0, 256);  
				return 1;
            }
            rx_state = 0;
            mPkg_length_counter = 0;
			memset(mPkg, 0, 256); 
		break;
		case 6:												//error code
				mPkg[mPkg_length_counter++] = rx_data;
					rx_state = 4;	
		break;
        default:											// error
            rx_state = 0;
            mPkg_length_counter = 0;
						memset(mPkg, 0, 256);
        break;
    }
	
    return 0;
}
