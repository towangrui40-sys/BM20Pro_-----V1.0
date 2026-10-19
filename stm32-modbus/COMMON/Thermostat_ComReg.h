#ifndef __THERMOSTAT_COMREG_H_//Thermostat
#define __THERMOSTAT_COMREG_H_


#define TMS_RTU_MEM(x) (x+TMS_RTU_MEM_BASE)

enum TMS_RTU_ADDR
{
	TMS_ADDR_PV,
	TMS_ADDR_SV,
	TMS_ADDR_ERROR,
	TMS_ADDR_STATE,
	TMS_ADDR_BACKUP,
	TMS_ADDR_SOUT,
	TMS_ADDR_SP1,
	TMS_ADDR_T1,
	TMS_ADDR_HREG_TOTAL,
	TMS_RTU_MEM_BASE= 74,
};


enum TMSAPP_RTU_ADDR
{
	TMSAPP_ADDR_SOFTVER,
	TMSAPP_ADDR_GET_TEMP,
	TMSAPP_ADDR_STATE,
	TMSAPP_ADDR_SET_TEMP,
	TMSAPP_ADDR_ERROR,
	TMSAPP_ADDR_WM,
	TMSAPP_ADDR_HREG_TOTAL,
	TMSAPP_RTU_MEM_BASE= 0,
};

typedef struct
{
	short pv;
	short sv;
	unsigned short err;
	unsigned short state;
	unsigned short backup;
	unsigned short sout;
	short sp1;
	unsigned short t1;
}TMS_RTU_TypeDef;



typedef struct
{
	unsigned short ver;
	short get_temp;
	unsigned short state;
	short set_temp;
	short err ;
	unsigned short work_mode;
}TMS_TypeDef;


typedef enum 
{
	EV_THERMOSTAT_INIT=1,
	EV_THERMOSTAT_SET_TEMP,	
	EV_THERMOSTAT_SET_RUN,
	EV_THERMOSTAT_NULL,
	EV_THERMOSTAT_TOTAL,
} eTMSEventType;


void Init_Thermpstat_RegImage(unsigned short *holdreg);
short Dispose_Thermpstat(TMS_TypeDef * str);

unsigned char TmsEventInit( void );
unsigned char TmsEventPost( eTMSEventType eEvent );
unsigned char TmsEventGet( eTMSEventType * eEvent );



#endif

