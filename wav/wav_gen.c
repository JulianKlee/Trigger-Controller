#include "wav_gen.h"
#include "wav_sync.h"

#include "incl/stm32f4xx.h"
#include "incl/stm32f4xx_rcc.h"
#include "incl/stm32f4xx_gpio.h"
#include "incl/stm32f4xx_dac.h"
#include "incl/stm32f4xx_dma.h"
#include "incl/misc.h"

#include "../portpin.h"

#include <math.h>
#include <float.h>

enum {
	rcc_enable_ahb1 = 
		  RCC_AHB1Periph_DMA1
		| RCC_AHB1Periph_GPIOA
		| RCC_AHB1Periph_GPIOB,

	rcc_enable_apb1 =
		RCC_APB1Periph_DAC,
};

portpin const dac_clk   = { GPIOB, 9, 0 };
portpin const dac_out_0 = { GPIOA, 4, 0 };
portpin const dac_out_1 = { GPIOA, 5, 0 };

static DMA_Stream_TypeDef *const dma_strm[2] = {DMA1_Stream5,  DMA1_Stream6};
static unsigned const dma_prio[2] = {DMA_Priority_VeryHigh, DMA_Priority_High};
static uint32_t const dac_chan[2] = {DAC_Channel_1, DAC_Channel_2};
static volatile void *const dac_addr[2] = {&DAC->DHR12R1, &DAC->DHR12R2};
static unsigned const dac_trig[2] = {DAC_Trigger_Ext_IT9, DAC_Trigger_T8_TRGO};

enum { WAV_GEN_LEN_MAX = 8192 };
static uint16_t wav_gen__wfmbuf[2][WAV_GEN_LEN_MAX];

struct wav_gen__waveform {
	unsigned type;
	float amp, off, pha;
	float aspect;
};

static size_t wav_gen__wfmlen[2] = {WAV_GEN_LEN_MAX, WAV_GEN_LEN_MAX};
static struct wav_gen__waveform wav_gen__wfmparms[2] = {
	{ WAV_WFM_SINE, 1.0f, 0.f, 0.f, NAN },
	{ WAV_WFM_TRIANGLE, 0.5f, 0.f, 0.f, 0.5f }
};

static inline
DAC_InitTypeDef wav_gen__mk_daccfg(
	unsigned trigger /* DAC_Trigger_Ext_IT9, DAC_Trigger_T8_TRGO */ )
{
	DAC_InitTypeDef const daccfg = {
		.DAC_Trigger        = trigger, 
		.DAC_WaveGeneration = DAC_WaveGeneration_None,
		.DAC_OutputBuffer   = DAC_OutputBuffer_Enable,
	};
	return daccfg;
}

static inline
DMA_InitTypeDef wav_gen__mk_dmacfg(
	unsigned mode,
	unsigned prio,
	volatile void *const periph,
	void const *const mem,
	size_t const len )
{
	DMA_InitTypeDef const dmacfg = {
		.DMA_Channel            = DMA_Channel_7,
		.DMA_Priority           = prio, /* DMA_Priority_VeryHigh */
		.DMA_DIR                = DMA_DIR_MemoryToPeripheral,
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


static
int wav_gen_disarm_ch(
	unsigned ch )
{

	DAC_DMACmd(dac_chan[ch], DISABLE);
	DMA_DeInit(dma_strm[ch]);
	DAC_Cmd(dac_chan[ch], DISABLE);
	DAC_Cmd(dac_chan[ch], ENABLE);
	switch( ch ){
	case 0:
		DAC_SetChannel1Data(DAC_Align_12b_R, wav_gen__wfmbuf[0][0]);
		break;
	case 1:
		DAC_SetChannel2Data(DAC_Align_12b_R, wav_gen__wfmbuf[1][0]);
		break;
	default:
		break;
	}
	return 0;
}

static
int wav_gen_arm_ch(
	unsigned ch,
	unsigned mode /* DMA_Mode_Circular, DMA_Mode_Normal*/ )
{
	DMA_InitTypeDef dmacfg = wav_gen__mk_dmacfg(
			mode,
			dma_prio[ch],
			dac_addr[ch],
			wav_gen__wfmbuf[ch],
			wav_gen__wfmlen[ch] );

	DAC_DMACmd(dac_chan[ch], DISABLE);
	DAC_Cmd(dac_chan[ch], DISABLE);
	DMA_DeInit(dma_strm[ch]);
	DMA_Init(dma_strm[ch], &dmacfg);

	DMA_Cmd(dma_strm[ch], ENABLE);
	DAC_Cmd(dac_chan[ch], ENABLE);
	DAC_DMACmd(dac_chan[ch], ENABLE);

	return 0;
}

int wav_gen_disarm(unsigned chs)
{
	if( chs & (1 << 0) ){
		wav_gen_disarm_ch(0);
	}
	if( chs & (1 << 1) ){
		wav_gen_disarm_ch(1);
	}

	return 0;
}

int wav_gen_arm(unsigned chs)
{
	if( chs & (1 << 0) ){
		wav_gen_arm_ch(0, DMA_Mode_Circular);
	}
	if( chs & (1 << 1) ){
		wav_gen_arm_ch(1, DMA_Mode_Circular);
	}

	return 0;
}

static inline
float clampf(float const v, float const l, float const u){
	if( l > v ){ return l; }
	if( u < v ){ return u; }
	return v;
}

static
int wav_gen_wfm_sin(
	unsigned ch,
	float amp,
	float off,
	float pha )
{
	size_t const len = wav_gen__wfmlen[ch];
	float const k_tau = 1./(float)len;

	for(size_t i = 0; i < len; ++i){
		float const t = k_tau * i * 2. * M_PI;
		wav_gen__wfmbuf[ch][i] = (uint16_t)clampf((0.5f + 2047.f * (1. + amp*sinf(t + pha) + off)), 0.f, 4095.f);
	}
	return 0;
}

static
int wav_gen_wfm_triangle(
	unsigned ch,
	float amp,
	float off,
	float pha,
	float aspect )
{
	size_t const len = wav_gen__wfmlen[ch];

	size_t rise = aspect < 1.f ? (len*aspect) : aspect;
	if( len < rise ){ rise = len; }
	size_t const fall = len - rise;

	size_t i = 0;
	if( 0 < rise ){
		float const k_r = 1.f/(float)rise;
		for(; i < rise; ++i ){
			float const t = k_r * i;
			float const v = off + amp*(2.f*t - 1.f);
			wav_gen__wfmbuf[ch][i] = (uint16_t)clampf((0.5f + 2047.f*(1.f+v)), 0.f, 4095.f);
		}
	}
	if( 0 < fall ){
		float const k_f = 1.f/(float)fall;
		for(; i < len; ++i ){
			float const t = k_f * (i-rise);
			float const v = off + amp*(1.f - 2.f*t);
			wav_gen__wfmbuf[ch][i] = (uint16_t)clampf((0.5f + 2047.f*(1.f+v)), 0.f, 4095.f);
		}
	}

	return 0;
}

int wav_gen_wfm_qry(
	unsigned ch,
	unsigned *out_type,
	float *out_amp,
	float *out_off,
	float *out_pha,
	float *out_aspect )
{
	if( 1 < ch ){
		return -1;
	}
#define WFM_PARM_QRY(n) \
	if( out_##n ){ *out_##n = wav_gen__wfmparms[ch].n; }
	WFM_PARM_QRY(type);
	WFM_PARM_QRY(amp);
	WFM_PARM_QRY(off);
	WFM_PARM_QRY(pha);
	WFM_PARM_QRY(aspect);
#undef WFM_PARM_QRY
	return 0;
}

int wav_gen_wfm(
	unsigned ch,
	unsigned type,
	float amp,
	float off,
	float pha,
	float aspect )
{
	if( type ){
		wav_gen__wfmparms[ch].type = type;
	} else {
		type = wav_gen__wfmparms[ch].type;
	}

#define WFM_PARM_SYNC(n) \
	if( isfinite(n) ){ \
		wav_gen__wfmparms[ch].n = n; \
	} else { \
		n = wav_gen__wfmparms[ch].n; \
	}
	WFM_PARM_SYNC(amp);
	WFM_PARM_SYNC(off);
	WFM_PARM_SYNC(pha);
	WFM_PARM_SYNC(aspect);
#undef WFM_PARM_SYNC

	switch( type ){
	default: break;

	case WAV_WFM_SINE:
		 return wav_gen_wfm_sin(ch, amp, off, pha);

	case WAV_WFM_TRIANGLE:
		 return wav_gen_wfm_triangle(ch, amp, off, pha, aspect);
	}
	return -1;
}

int wav_gen_len(
	unsigned ch,
	size_t len )
{
	wav_gen__wfmlen[ch] = len;
	wav_gen_wfm(ch, 0, NAN, NAN, NAN, NAN);
	return 0;
}

int wav_gen_init(void)
{
	int rc = 0;

	RCC_AHB1PeriphClockCmd(rcc_enable_ahb1, ENABLE);
	RCC_APB1PeriphClockCmd(rcc_enable_apb1, ENABLE);

	portpin_cfg_in(&dac_clk);
	portpin_cfg_out(&dac_out_0);
	portpin_cfg_out(&dac_out_1);
	portpin_cfg_analog(&dac_out_0);
	portpin_cfg_analog(&dac_out_1);

	for( unsigned ch = 0; ch < 2; ++ch ){
		DAC_InitTypeDef daccfg = wav_gen__mk_daccfg( dac_trig[ch] );
		DAC_Init(dac_chan[ch], &daccfg);
	}

	return rc;
}
