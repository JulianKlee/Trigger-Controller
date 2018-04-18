#include <stdint.h>
#include <stddef.h>
#include <math.h>

#include "wav_clk.h"
#include "wav_gen.h"

#include "incl/stm32f4xx.h"
#include "incl/stm32f4xx_rcc.h"
#include "incl/stm32f4xx_tim.h"
#include "incl/stm32f4xx_gpio.h"
#include "incl/stm32f4xx_exti.h"
#include "incl/misc.h"

#include "../portpin.h"

enum {
	rcc_enable_ahb1 = 
		  RCC_AHB1Periph_GPIOA
		| RCC_AHB1Periph_GPIOB
		| RCC_AHB1Periph_GPIOC
		| RCC_AHB1Periph_GPIOD
		| RCC_AHB1Periph_GPIOE,

	rcc_enable_apb1 =
		RCC_APB1Periph_TIM3,
	
	rcc_enable_apb2 = 
		  RCC_APB2Periph_TIM1
		| RCC_APB2Periph_TIM8,
};

static float const pll_vco_center_kHz  = 6250.f;
static float const pll_ref_kHz_min     =  300.f;
static float const pll_ref_kHz_max     =  550.f;
static float const pll_ref_default_kHz =  425.f;

static portpin pll_vco = { GPIOD, 2, GPIO_AF_TIM3 };
static portpin pll_cmp = { GPIOA, 6, GPIO_AF_TIM3 };
static portpin bscan_clk = { GPIOA, 0, GPIO_AF_TIM8 };
static portpin bscan_out = { GPIOC, 6, GPIO_AF_TIM8 };
static portpin btrig_clk = { GPIOE, 7, GPIO_AF_TIM1 };
static portpin btrig_out = { GPIOA, 8, GPIO_AF_TIM1 };

static portpin brecl_out_p = { GPIOC,  7, GPIO_AF_TIM8 };
static portpin brecl_out_n = { GPIOB, 14, GPIO_AF_TIM8 };

static unsigned wav_clk__pll_div = 0;

//===========================Timer divider====================================

static
void wav_clk_pll_init(void)
{
	TIM_TimeBaseInitTypeDef const timcfg = {
		.TIM_Prescaler     = 0,
		.TIM_Period        = 2,
		.TIM_CounterMode   = TIM_CounterMode_Up,
		.TIM_ClockDivision = TIM_CKD_DIV1,
	};
	TIM_OCInitTypeDef const tim_oc_cfg = {
		.TIM_OCMode      = TIM_OCMode_PWM1,
		.TIM_OutputState = TIM_OutputState_Enable,
		.TIM_OCPolarity  = TIM_OCPolarity_High,
		.TIM_OCIdleState = TIM_OCIdleState_Reset,
	};

	portpin_cfg_in(&pll_vco);
	portpin_cfg_out(&pll_cmp);

	TIM_TimeBaseInit(TIM3, &timcfg);
	TIM_UpdateDisableConfig(TIM3, DISABLE);
	TIM_OC1Init(TIM3, &tim_oc_cfg);
	TIM_OC1FastConfig(TIM3, TIM_OCFast_Enable);
	TIM_ETRClockMode2Config(TIM3,
		TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted, 0);
	TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_OC1);
	TIM_Cmd(TIM3, ENABLE);
}

static
void wav_clk_bscan_init(void)
{
	TIM_TimeBaseInitTypeDef const timcfg = {
		.TIM_Prescaler     = 0,
		.TIM_Period        = 2,
		.TIM_CounterMode   = TIM_CounterMode_Up,
		.TIM_ClockDivision = TIM_CKD_DIV1,
	};
	TIM_BDTRInitTypeDef const tim_bdtr_cfg = {
		0,
	};
	TIM_OCInitTypeDef const tim_oc_cfg = {
		.TIM_OCMode      = TIM_OCMode_PWM1,
		.TIM_OutputState = TIM_OutputState_Enable,
		.TIM_OCPolarity  = TIM_OCPolarity_High,
		.TIM_OCIdleState = TIM_OCIdleState_Reset,
	};

	portpin_cfg_in(&bscan_clk);
	portpin_cfg_out(&bscan_out);

	portpin_cfg_out(&brecl_out_p);
	portpin_cfg_out(&brecl_out_n);

	TIM_TimeBaseInit(TIM8, &timcfg);
	TIM_UpdateDisableConfig(TIM8, DISABLE);
	TIM_BDTRConfig(TIM8, &tim_bdtr_cfg);

	TIM_OC1Init(TIM8, &tim_oc_cfg);
	TIM_OC2Init(TIM8, &tim_oc_cfg);
#if 0
	TIM_OC1FastConfig(TIM8, TIM_OCFast_Enable);
	TIM_OC2FastConfig(TIM8, TIM_OCFast_Enable);
#endif

	TIM_ETRClockMode2Config(TIM8,
		TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted, 0);

	TIM_SelectOutputTrigger(TIM8, TIM_TRGOSource_OC1);
	
	TIM_OC2PolarityConfig(TIM8, TIM_OCPolarity_High);
	TIM_OC2NPolarityConfig(TIM8, TIM_OCNPolarity_High);
	TIM_CCxCmd(TIM8, TIM_Channel_2, TIM_CCx_Enable);
	TIM_CCxNCmd(TIM8, TIM_Channel_2, TIM_CCxN_Enable);
	TIM_CtrlPWMOutputs(TIM8, ENABLE);

	TIM_Cmd(TIM8, ENABLE);
}

static
void wav_clk_btrig_init(void)
{
	TIM_TimeBaseInitTypeDef const timcfg = {
		.TIM_Prescaler     = 0,
		.TIM_Period        = 2,
		.TIM_CounterMode   = TIM_CounterMode_Up,
		.TIM_ClockDivision = TIM_CKD_DIV1,
	};
	TIM_BDTRInitTypeDef const tim_bdtr_cfg = {
		0
	};
	TIM_OCInitTypeDef const tim_oc_cfg = {
		.TIM_OCMode      = TIM_OCMode_PWM1,
		.TIM_OutputState = TIM_OutputState_Enable,
		.TIM_OCPolarity  = TIM_OCPolarity_High,
		.TIM_OCIdleState = TIM_OCIdleState_Reset,
	};

	portpin_cfg_in(&btrig_clk);
	portpin_cfg_out(&btrig_out);

	TIM_TimeBaseInit(TIM1, &timcfg);
	TIM_UpdateDisableConfig(TIM1, DISABLE);
	TIM_BDTRConfig(TIM1, &tim_bdtr_cfg);
	TIM_OC1Init(TIM1, &tim_oc_cfg);
	TIM_OC1FastConfig(TIM1, TIM_OCFast_Enable);
	TIM_ETRClockMode2Config(TIM1,
		TIM_ExtTRGPSC_OFF, TIM_ExtTRGPolarity_NonInverted, 0);
	TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_OC1);
	TIM_Cmd(TIM1, ENABLE);
}

static
int wav_clk_pll_cfg(float const ref_kHz, unsigned *const out_div)
{
	int rc= 0;
	if( isfinite(ref_kHz)
	 && (0 <= ref_kHz)
	){
		if( pll_ref_kHz_min <= ref_kHz
		 || pll_ref_kHz_max >= ref_kHz
		){
			wav_clk__pll_div = (unsigned)(0.5f + pll_vco_center_kHz / ref_kHz);
			TIM_SetCompare1(TIM3, wav_clk__pll_div/2);
			TIM_SetAutoreload(TIM3, wav_clk__pll_div-1);
		} else {
			rc= -1;
		}
	}
	if( out_div ){
		*out_div = wav_clk__pll_div;
	}
	return rc;
}

static unsigned wav_clk_bscan__width = 2;
static unsigned wav_clk_bscan__gate  = 1;

static inline
void wav_clk_tim1_load(void)
{
	TIM_SetCounter(TIM1,
		wav_clk_bscan__gate );
}

static
int wav_clk_bscan_cfg(unsigned const spls, unsigned const div)
{
	unsigned const bspls = spls*div-1;
	wav_clk_bscan__width = bspls;
	wav_clk_bscan__gate  = div;

	TIM_Cmd(TIM1, DISABLE);
	TIM_Cmd(TIM8, DISABLE);

	TIM_SetAutoreload(TIM1, bspls);
	TIM_SetCompare1(TIM1, wav_clk_bscan__gate - 1);
	wav_clk_tim1_load();

	TIM_SetCounter(TIM8, 0);
	TIM_SetAutoreload(TIM8, bspls);
	TIM_SetCompare1(TIM8, bspls/2);
	TIM_SetCompare2(TIM8, wav_clk_bscan__gate*10);

	TIM_OC2PolarityConfig(TIM8, TIM_OCPolarity_High);
	TIM_OC2NPolarityConfig(TIM8, TIM_OCNPolarity_High);
	TIM_CCxCmd(TIM8, TIM_Channel_2, TIM_CCx_Enable);
	TIM_CCxNCmd(TIM8, TIM_Channel_2, TIM_CCxN_Enable);
	TIM_CtrlPWMOutputs(TIM8, ENABLE);

	TIM_Cmd(TIM8, ENABLE);
	TIM_Cmd(TIM1, ENABLE);
	return 0;
}

int wav_clk_sync(void)
{
	TIM_Cmd(TIM3, DISABLE);
	TIM_Cmd(TIM1, DISABLE);
	TIM_Cmd(TIM8, DISABLE);

	TIM_SetCounter(TIM3, 0);

	/* TIM_SetCounter(TIM8, (10*wav_clk_bscan__width)/16); */
	TIM_SetCounter(TIM8, wav_clk_bscan__gate*8);
	TIM_SetAutoreload(TIM8, wav_clk_bscan__width);
	TIM_SetCompare1(TIM8, wav_clk_bscan__width/2);
	TIM_SetCompare2(TIM8, wav_clk_bscan__gate*10);
	/* to delay the TIM1 gate pulse, the counter must be preloaded
	 * with a "negative" value. The unsigned overflow threshold for
	 * the counter is wav_clk_bscan__width, which is what must be
	 * subtracted from. */
	wav_clk_tim1_load();

	TIM_OC2PolarityConfig(TIM8, TIM_OCPolarity_High);
	TIM_OC2NPolarityConfig(TIM8, TIM_OCNPolarity_High);
	TIM_CCxCmd(TIM8, TIM_Channel_2, TIM_CCx_Enable);
	TIM_CCxNCmd(TIM8, TIM_Channel_2, TIM_CCxN_Enable);
	TIM_CtrlPWMOutputs(TIM8, ENABLE);

	TIM_Cmd(TIM8, ENABLE);
	TIM_Cmd(TIM1, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
	return 0;
}

int wav_clk_bscan_output(
	unsigned enable )
{
	if( !enable ){
		TIM_CtrlPWMOutputs(TIM1, DISABLE);
		wav_gen_disarm( (1<<1) );
	} else {
		while( TIM_GetCounter(TIM8) >= (wav_clk_bscan__width / 2)
		    || TIM_GetCounter(TIM1) >= (wav_clk_bscan__width - 1)
		    || TIM_GetCounter(TIM1) <= (wav_clk_bscan__gate/3 + 1)
		);
		TIM_CtrlPWMOutputs(TIM1, ENABLE);
		wav_gen_arm( (1<<1) );
	}
	return 0;
}

int wav_clk_cfg(
	float const ascan_kHz,
	float const bscan_kHz,
	unsigned *const out_samples,
	unsigned *const out_div )
{
	unsigned div;
	float const pll_ref_kHz = isfinite(ascan_kHz) ? ascan_kHz : pll_ref_default_kHz;
	float const x_kHz = bscan_kHz;
	/* XXX: to obtain a stable alignment between the bscan_clk timer,
	 * the ascan output signal and the ascan clock signal, all fast
	 * division factors have to be evaluated from the slowst scan clock
	 * i.e. the bscan clock. Hence calculate the amount of clock divisions
	 * between bscan trigger (gates) and divide the divisions (i.e.
	 * multiply) the frequencies from there.
	 *
	 * Example: We want to scan with a resonant scanner in both directions.
	 * Hence the bscan trigger frequency is twice the bscan scanning
	 * frequency. Hence apply a factor of 2 on the bscan trigger frequency.
	 * The amount of samples in the bscan waveform buffer is then twice
	 * the value of bscan clock-to-trigger division. */
	unsigned const x_spls = ((unsigned)(0.5f + pll_ref_kHz / (2.f*x_kHz)));

	wav_clk_pll_cfg(pll_ref_kHz, &div);
	wav_clk_bscan_cfg(x_spls, div);

	if( out_samples ){ *out_samples = 2*x_spls; }
	if( out_div ){ *out_div = div; }

	return 0;
}

int wav_clk_init(void)
{
	unsigned pll_div;
	RCC_AHB1PeriphClockCmd(rcc_enable_ahb1, ENABLE);
	RCC_APB1PeriphClockCmd(rcc_enable_apb1, ENABLE);
	RCC_APB2PeriphClockCmd(rcc_enable_apb2, ENABLE);

	wav_clk_pll_init();
	wav_clk_bscan_init();
	wav_clk_btrig_init();

	wav_clk_pll_cfg(pll_ref_default_kHz, &pll_div);
	wav_clk_bscan_cfg(2, pll_div);
	return 0;
}
