#define _GNU_SOURCE
#include "wav_mon.h"

#include "ch.h"
#include "hal.h"

#include "incl\stm32f4xx.h"
#include "incl\stm32f4xx_rcc.h"
#include "incl\stm32f4xx_gpio.h"
#include "incl\stm32f4xx_adc.h"
#include "incl\stm32f4xx_dma.h"
#include "incl\misc.h"

#include "portpin.h"

#include <math.h>

enum {
	rcc_enable_ahb1 = 
		  RCC_AHB1Periph_DMA2
		| RCC_AHB1Periph_GPIOA
		| RCC_AHB1Periph_GPIOE,

	rcc_enable_apb2 =
		RCC_APB2Periph_ADC1,
};

uint16_t wav_mon__wfmbuf[WAV_MON_LEN_MAX];
size_t wav_mon__wfmlen = WAV_MON_LEN_MAX;

portpin const adc_clk = { GPIOE, 4, 0 };
portpin const adc_in  = { GPIOA, 1, GPIO_Mode_IN };

static DMA_Stream_TypeDef *const dma_strm = DMA2_Stream0;
static unsigned const dma_prio = DMA_Priority_High;
static volatile void *const adc_addr = &ADC1->DR;

static
int wav_mon_adc_init(void)
{
	ADC_CommonInitTypeDef const adccfg = {
		.ADC_Mode             = ADC_Mode_Independent,
		.ADC_Prescaler        = ADC_Prescaler_Div2,
		.ADC_DMAAccessMode    = ADC_DMAAccessMode_Disabled,
		.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles,
	};
	ADC_InitTypeDef const adc1cfg = {
		.ADC_Resolution           = ADC_Resolution_12b,
		.ADC_ScanConvMode         = DISABLE,
		.ADC_ContinuousConvMode   = DISABLE,
		.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising,
		.ADC_ExternalTrigConv     = ADC_ExternalTrigConv_Ext_IT11,
		.ADC_DataAlign            = ADC_DataAlign_Right,
		.ADC_NbrOfConversion      = 1,
	};

	ADC_CommonInit(&adccfg);
	ADC_Init(ADC1, &adc1cfg);
	ADC_RegularChannelConfig(ADC1, 1, 1, ADC_SampleTime_3Cycles);

	return 0;
}

static inline
DMA_InitTypeDef wav_mon__mk_dmacfg(
	unsigned mode,
	unsigned prio,
	volatile void *const periph,
	void const *const mem,
	size_t const len )
{
	DMA_InitTypeDef const dmacfg = {
		.DMA_Channel            = DMA_Channel_0,
		.DMA_Priority           = prio,
		.DMA_DIR                = DMA_DIR_PeripheralToMemory,
		.DMA_Mode               = mode, 
		.DMA_FIFOMode           = DMA_FIFOMode_Enable,
		.DMA_FIFOThreshold      = DMA_FIFOThreshold_HalfFull,

		.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord,
		.DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
		.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single,
		.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord,
		.DMA_MemoryInc          = DMA_MemoryInc_Enable,
		.DMA_MemoryBurst        = DMA_MemoryBurst_Single,

		.DMA_Memory0BaseAddr    = (uintptr_t)mem,
		.DMA_PeripheralBaseAddr = (uintptr_t)periph,
		.DMA_BufferSize         = len,
	};
	return dmacfg;
}

int wav_mon_len(size_t len){
	wav_mon__wfmlen = len;
	return 0;
};

int wav_mon_arm(void)
{
	DMA_InitTypeDef dmacfg = wav_mon__mk_dmacfg(
		DMA_Mode_Circular, dma_prio,
		adc_addr,
		wav_mon__wfmbuf, wav_mon__wfmlen );

	ADC_DMACmd(ADC1, DISABLE);
	ADC_Cmd(ADC1, DISABLE);
	DMA_DeInit(dma_strm);
	DMA_Init(dma_strm, &dmacfg);

	DMA_Cmd(dma_strm, ENABLE);
	ADC_Cmd(ADC1, ENABLE);
	ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);
	ADC_DMACmd(ADC1, ENABLE);

	return 0;
}

int wav_mon_init(void)
{
	int rc = 0;

	RCC_AHB1PeriphClockCmd(rcc_enable_ahb1, ENABLE);
	RCC_APB2PeriphClockCmd(rcc_enable_apb2, ENABLE);

	portpin_cfg_in(&adc_clk);
	portpin_cfg_analog(&adc_in);

	wav_mon_adc_init();

	return rc;
}


#if 1
static inline
float autocomplement_phase(
	
{
//Intelectual property of Wolfgang Draxinger
}
#endif

int wav_mon_eval(
	float *amplitude,
	float *phase )
{
	float avgspl = 0;
	for(unsigned i = 0; i < wav_mon__wfmlen; ++i) {
		avgspl += wav_mon__wfmbuf[i];
	}
	avgspl /= (float)(wav_mon__wfmlen);

	float sqsum = 0.;
	float phi_iq[2] = {0.f, 0.f};
	float const t = 2.f*(float)M_PI / (float)wav_mon__wfmlen;
	for(unsigned i = 0; i < wav_mon__wfmlen; ++i) {
		float const v = -(wav_mon__wfmbuf[i] - avgspl);
		sqsum += v*v;

		float s,c;
		sincosf(i*t, &s, &c);

		phi_iq[0] += v * s;
		phi_iq[1] += v * c;
	}
	float const rms =
		  sqrtf(sqsum / (float)wav_mon__wfmlen)
		/ ((1024.f)*0.7f);

	if( amplitude ){
		*amplitude = rms;
	}
	if( phase ){
		*phase = atan2f(phi_iq[1], phi_iq[0]);
#if 1
		/* crazy anticorrelation phase hack. If this works,
		 * I have no mathematically founded theory why,
		 * just accept that it does what it does :) -- dw */
		*phase +=
			autocomplement_phase( );
#endif

	}

	return 0;
}
