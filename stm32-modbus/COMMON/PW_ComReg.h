#ifndef __PW_COMREG_H_//adjust power
#define __PW_COMREG_H_


#define PW_RTU_MEM(x) (x+PW_RTU_MEM_BASE)

enum PW_RTU_ADDR
{
	PW_ADDR_STATE,
	PW_ADDR_OVOL,
	PW_ADDR_OCUR,
	PW_ADDR_OPW,
	PW_ADDR_SV,
	PW_ADDR_BV,
	PW_ADDR_ERROR,
	PW_ADDR_LOAD_RES,
	PW_ADDR_IVOL,
	PW_ADDR_IFRQ,
	PW_ADDR_TT,
	PW_ADDR_HREG_TOTAL,
	PW_RTU_MEM_BASE= 0,
};


enum PWAPP_RTU_ADDR
{
	PWSAPP_ADDR_SOFTVER,
	PWSAPP_ADDR_GET_CUR,
	PWSAPP_ADDR_STATE,
	PWSAPP_ADDR_SET_CUR,
	PWSAPP_ADDR_ERROR,
	PWSAPP_ADDR_WM,
	PWSAPP_ADDR_HREG_TOTAL,
	PWSAPP_RTU_MEM_BASE= 0,
};

typedef struct
{
	unsigned short state;
	unsigned short ovol;
	unsigned short ocur;
	unsigned short opw;
	unsigned short sv;
	unsigned short bv;
	unsigned short err;
	unsigned short lres;
	unsigned short ivol;
	unsigned short ireq;
	unsigned short tt;
}PW_RTU_TypeDef;



typedef struct
{
	unsigned short ver;
	unsigned short get_cur;
	unsigned short state;
	unsigned short set_cur;
	unsigned short err;
	unsigned short work_mode;
}PW_TypeDef;


typedef enum 
{
	EV_PW_INIT=1,
	EV_PW_SET_CUR,	
	EV_PW_SET_RUN,
	EV_PW_NULL,
	EV_PW_TOTAL,
} ePWEventType;


void Init_PW_RegImage(unsigned short *holdreg);
short Dispose_PW(PW_TypeDef * str);

unsigned char pwEventInit( void );
unsigned char pwEventPost(ePWEventType eEvent );
unsigned char pwEventGet( ePWEventType * eEvent );



#endif

