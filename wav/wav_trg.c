#include "wav_trg.h"

static
int wav_trg_pin_init(void)
{
}

static
int wav_trg_cfg()
{

  //tim1
  {
    TIM_TimeBaseInitStructure.TIM_Prescaler = 0;
    TIM_TimeBaseInitStructure.TIM_Period = CFG->DAC_CH2_waveform_sample-1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);
    TIM_ETRClockMode2Config(TIM1, TIM_ExtTRGPSC_OFF,
    TIM_ExtTRGPolarity_NonInverted,
                            0);

    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
    TIM_OCInitStructure.TIM_Pulse = CFG->DAC_CH2_waveform_sample/4;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);
    //TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_OC1);

    TIM1->CR2 = (TIM8->CR2 & ~(TIM_CR2_MMS_0 * 0b111)) | TIM_CR2_MMS_0 * 0b100;
    TIM1->SMCR |= TIM_SMCR_ECE;
    TIM1->SMCR |= TIM_SMCR_SMS_0 * 0x7;
    TIM1->SMCR |= TIM_SMCR_TS_0 * 0x7;
    TIM_Cmd(TIM1,ENABLE);

    TIM_CtrlPWMOutputs(TIM1,ENABLE);
  }


  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_OCInitTypeDef TIM_OCInitStructure;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = CFG->DAC_CH1_waveform_sample;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
  TIM_ETRClockMode2Config(TIM3, TIM_ExtTRGPSC_OFF,
  TIM_ExtTRGPolarity_NonInverted,
                          0);
  /* PWM1 Mode configuration: Channel1 */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = 0;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OC1Init(TIM3, &TIM_OCInitStructure);
  TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
  /* TIM3 enable counter */
  TIM3->CR1 = 0;
  TIM3->CR1 |= TIM_CR1_CMS_0 * 0x1;
  TIM3->CR2 = 0;
  TIM3->SMCR |= TIM_SMCR_ECE;
  TIM3->SMCR |= TIM_SMCR_SMS_0 * 0x7;
  TIM3->SMCR |= TIM_SMCR_TS_0 * 0x7;
  TIM3->DIER = 0;
  TIM3->CCMR1 = TIM_CCMR1_OC1M_0 * 0x6;
  TIM3->CCMR1 |= TIM_CCMR1_IC1F_0 * 0x3;
  TIM3->CCER |= 1;
  TIM_Cmd(TIM3, ENABLE);
}

int wav_trg_init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
}
