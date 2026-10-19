#ifndef __BUS_IO_COMREG_H_//SM:step motor
#define __BUS_IO_COMREG_H_

enum RTU_BUS_IO_ADDR
{
	BUS_ADDR_SOFTVER,
	BUS_ADDR_DOOR_STATE,
	BUS_ADDR_POT_DOOR_STATE,
	BUS_ADDR_COOL_FAN1,
	BUS_ADDR_COOL_FAN2,
	BUS_ADDR_COOL_FAN3,
	BUS_ADDR_COOL_FAN4,
	BUS_ADDR_SIDE_FAN,
	BUS_ADD_POT_SW,
	BUS_ADD_DOOR_LOCK_SW,
	
	BUS_ADDR_HREG_TOTAL,
	BUS_RTU_MEM_BASE= 0,
};



typedef struct
{
	unsigned short ver;
	unsigned short door_state;
	unsigned short pot_door_state;
	unsigned short cfan1;
	unsigned short cfan2;
	unsigned short cfan3;
	unsigned short cfan4;
	unsigned short sfan;
	unsigned short pot_door_ctrl;
	unsigned short door_lock_ctrl;
}BUS_IO_TypeDef;


typedef enum 
{
	EV_BUS_INIT=1,
	EV_BUS_SET,	
	EV_BUS_NULL,
	EV_BUS_TOTAL,
} eBUSEventType;


void Init_BusIO_RegImage(unsigned short *holdreg);
short Dispose_BusIO(BUS_IO_TypeDef * str);

unsigned char busEventInit( void );
unsigned char busEventPost( eBUSEventType eEvent );
unsigned char busEventGet( eBUSEventType * eEvent );



#endif

