#include "myiic.h"
#include "lIS302DL.h"


//
void LIS302DL_Write(unsigned char reg,unsigned char info)
{   
    IIC_Start(); 
    IIC_Send_Byte(0x38);  
    IIC_Wait_Ack();     
    IIC_Send_Byte(reg);  
    IIC_Wait_Ack(); 
    IIC_Send_Byte(info); 
    IIC_Wait_Ack();
    IIC_Stop();
}

//
int8_t LIS302DL_Read(uint8_t address)
{
    signed char val;
    IIC_Start(); 
    IIC_Send_Byte(0x38);  
    IIC_Wait_Ack();     

    IIC_Send_Byte(address);  
    IIC_Wait_Ack(); 

    IIC_Start();
    IIC_Send_Byte(0x39); 
    IIC_Wait_Ack();
    val = IIC_Read_Byte(0); 
    IIC_NAck();
    IIC_Stop();

    return(val);
}

u8 Lis302DL_Output(MEMS_TYPEDEF* mems)
{
    if((LIS302DL_Read(0x27) & 0x08) != 0){

        mems->x = (s16)LIS302DL_Read(0x29);  //x
        mems->y = (s16)LIS302DL_Read(0x2B);  //y
        mems->z = (s16)LIS302DL_Read(0x2D);  //z
        return 0;
    }

    return 1;
}

//
void LIS302DL_Config(void)
{
	LIS302DL_Write(CTRL_REG1,0xc7);
	//DR=1,加速度的采集频率为 400Hz;
	//PD=1,LIS302DL工作在普通功耗模式;
	//FS=0,最大测量范围为 2g（2.3g）,灵敏度为 18mg/LSB（16.2）,受温度影响为 0.01%, 在0加速度时.飘移为 40mg;
	//STP=0,STM=0,表示 LIS302DL 工作在普通模式,即非自检模式;
	//Zen=1,表示使能 Z 轴方向加速度采集; 
	//Yen=1,表示使能 Y 轴方向加速度采集; 
	//Xen=1,表示使能 X 轴方向加速度采集; 
  LIS302DL_Write(CTRL_REG2,0x00);
	//下面操作是对中断和高通滤波器等配置，我们不使用不用配置，就默认值即可
	//	LIS302DL_Write(CTRL_REG2,0x00);
	//SIM=0,表示 SPI 口为 4 总线;（这个要先看cs决定）
	//BOOT=0,表示不要刷新存储器的内容,为普通模式;
	//FDS=0,表示禁用高通滤器;
	//HP_FF_W_U2=0, 禁用自由落体/唤醒高通滤波器 2;
	//HP_FF_W_U1=0, 禁用自由落体/唤醒高通滤波器 1;
	//HP_coeff2=0,HP_coeff1=0,DR=0, 表示自由落体/唤醒高通滤波器的输出截止频率为 2Hz;
  LIS302DL_Write(CTRL_REG3,0xC1);
	//	LIS302DL_Write(CTRL_REG3,0xC1);
	//IHL=1,表示在中断发生时,中断脚(INT1 和 INT2)输出低电平;
	//PP_OD=1,表示中断脚开漏输出;
	//I2CFG2=0,I2CFG1=0,I2CFG0=0,表示 INT2 脚输出低电平;
	//I1CFG2=0,I1CFG1=0,I1CFG0=1,表示 INT1 脚是根据自由落体/唤醒 1 寄存器的输出而发生变化;
  LIS302DL_Write(FF_WU_THS_1,0x28);
	//	LIS302DL_Write(FF_WU_THS_1,0x28);		
	//DCRM=0,表示在不再有中断发生时,持续时间计数器会被复位到初值;
	//THS0~THS6,用来设置自由落体/唤醒的下限值--720mg;	
  LIS302DL_Write(FF_WU_DURATION_1,40);
	//	LIS302DL_Write(FF_WU_DURATION_1,40);	
	//当 DR=1 时,加速度的采集频率为 ;			
	LIS302DL_Write(FF_WU_CFG_1,0x10); 	
	//AOI=1,表示所有中断事件相或后输出;
	//LIR=0,表示将中断请求不锁存;
	//ZHIE=0,表示在 Z 输出寄存器的值大于 Z 轴的设置值时,将不会使能 Z 轴中断;
	//ZLIE=1,表示在 Z 输出寄存器的值小于 Z 轴的设置值时,将不会使能 Z 轴中断;
	//YHIE=0,表示在 Y 输出寄存器的值大于 Y 轴的设置值时,将不会使能 Y 轴中断;
	//YLIE=1,表示在 Y 输出寄存器的值小于 Y 轴的设置值时,将不会使能 Y 轴中断;
	//XHIE=1,表示在 X 输出寄存器的值大于 X 轴的设置值时,将不会使能 X 轴中断;
	//XLIE=0,表示在 X 输出寄存器的值小于 X 轴的设置值时,将不会使能 X 轴中断;
}

//
uint8_t LIS302DL_Check(void)
{
    if(LIS302DL_Read(0x0f)){
        return 1;
    }else{
        return 0;
    }
}
