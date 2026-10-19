#ifndef __LIS302DL_H
#define __LIS302DL_H

#include "sys.h" 

#define CTRL_REG1           0x20
#define CTRL_REG2           0x21
#define CTRL_REG3           0x22
#define FF_WU_THS_1         0x32
#define FF_WU_DURATION_1    0x33
#define FF_WU_CFG_1         0x30
#define STATUS_REG          0x27


typedef struct
{
    signed short x;
    signed short y;
    signed short z;
    
}MEMS_TYPEDEF;


uint8_t LIS302DL_Check(void);
void LIS302DL_Config(void);
u8 Lis302DL_Output(MEMS_TYPEDEF * mems);

#endif 
