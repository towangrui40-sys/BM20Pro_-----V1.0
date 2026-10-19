/******************** (C) COPYRIGHT 2012 北京康鼎医疗 ***************************
 * 文件名  ：adc.c
 * 描述    ：线性光耦隔离的热敏电阻的温度采集
 * 硬件连接：PB0 - ADC12 - in6 连接外部电压(热敏电阻通过线性光耦隔离的电压)
 			 PB1 - ADC12 - in7 连接外部电压(热敏电阻通过线性光耦隔离的电压)
 * 库版本  ：ST3.5.0 
**********************************************************************************/
#include "adc.h"

#define ADC1_DR_Address    ((u32)0x40012400+0x4c)


__IO uint16_t ADC_ConvertedValue[TIMES][CHANNEL];
//__IO u16 ADC_ConvertedValueLocal;
//__IO uint16_t ADC_ConvertedValue[3];

/*
 * 函数名：ADC1_GPIO_Config
 * 描述  ：配置模拟输入通道PC0,PC1;
 * 输入  : 无
 * 输出  ：无
 * 调用  ：内部调用
 */
static void ADC1_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable DMA clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	
	/* Enable ADC1 and GPIOC clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOB, ENABLE);
	//Configure PB.00 -- NTC  PB.01 -- CURRENT
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOB, &GPIO_InitStructure);					
}


/* 函数名：ADC1_Mode_Config
 * 描述  ：配置ADC1的工作模式为MDA模式
 * 输入  : 无
 * 输出  ：无
 * 调用  ：内部调用
 */
static void ADC1_Mode_Config(void)
{
	DMA_InitTypeDef DMA_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	
	/* DMA channel1 configuration */
	DMA_DeInit(DMA1_Channel1);
	DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;	 				//ADC地址
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)&ADC_ConvertedValue;			//内存地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = TIMES*CHANNEL;										//转换完成的数据存储大小，需要和通道数对应
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;			//外设地址固定
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;						//设置DMA的内存递增模式，
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;	//半字
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;								//循环传输
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);
	
	/* Enable DMA channel1 */
	DMA_Cmd(DMA1_Channel1, ENABLE);
	
	/* ADC1 configuration */
	
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;						//独立ADC模式
	ADC_InitStructure.ADC_ScanConvMode = ENABLE ; 	 						//扫描模式，扫描模式用于多通道采集
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;						//关闭连续转换模式，只转换一次，连续转换会造成SysTick定时器死机
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;		//不使用外部触发转换
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; 					//采集数据右对齐
	ADC_InitStructure.ADC_NbrOfChannel = CHANNEL;	 								//要转换的通道数目1
	ADC_Init(ADC1, &ADC_InitStructure);
	
	/*配置ADC时钟，为PCLK2的8分频，即9MHz*/
	RCC_ADCCLKConfig(RCC_PCLK2_Div8); 
	/*配置ADC1的通道6为55.	5个采样周期，序列为1 */ 
	ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_239Cycles5);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 2,  ADC_SampleTime_239Cycles5);

   	//使能温度传感器和内部参考电压   
  //	ADC_TempSensorVrefintCmd(ENABLE); 
	
	/* Enable ADC1 DMA */
	ADC_DMACmd(ADC1, ENABLE);
	
	/* Enable ADC1 */
	ADC_Cmd(ADC1, ENABLE);
	
	/*复位校准寄存器 */   
	ADC_ResetCalibration(ADC1);
	/*等待校准寄存器复位完成 */
	while(ADC_GetResetCalibrationStatus(ADC1));
	
	/* ADC校准 */
	ADC_StartCalibration(ADC1);
	/* 等待校准完成*/
	while(ADC_GetCalibrationStatus(ADC1));
	
	/* 由于没有采用外部触发，所以使用软件触发ADC转换 */ 
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

/*
 * 函数名：ADC1_Init
 * 描述  ：无
 * 输入  ：无
 * 输出  ：无
 * 调用  ：外部调用
 */
void ADC1_Init(void)
{
	ADC1_GPIO_Config();
	ADC1_Mode_Config();
}
u16 Get_Adc(u8 ch,float value,float fliter_num)   
{
	unsigned int buffer_temp[3];
	float x = 0;
	buffer_temp[2] = ADC_ConvertedValue[2][ch];
	buffer_temp[1] = ADC_ConvertedValue[1][ch];
	buffer_temp[0] = ADC_ConvertedValue[0][ch];
	x = (buffer_temp[2] + buffer_temp[1] + buffer_temp[0])/3;
	x = x*fliter_num+(value*(1-fliter_num));
	return (u16) x;
}

/******************* (C) COPYRIGHT 2012 WildFire Team *****END OF FILE************/
