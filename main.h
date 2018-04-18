/*
 * sorted.h
 *
 *  Created on: Aug 25, 2017
 *      Author: kleej
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#ifndef INCL_SORTED_H_
#define INCL_SORTED_H_
//=======================================================================

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)
#define TEST_WA_SIZE    THD_WORKING_AREA_SIZE(256)

#define DAC_DHR12R2_ADDRESS    0x40007414
#define DAC_DHR12R1_ADDRESS    0x40007408

extern uint16_t wave1[];
extern uint16_t wave2[];

extern uint16_t adcBuffer[];
extern uint16_t adcData[];
//====================================================================

typedef enum
{
  disable_galvo     =0,
  resonant          =1,
  non_resonant      =2
}galvo_type;

typedef enum
{
  DAC_mode_undef    =0,
  DAC_mode_fast     =1,
  DAC_mode_slow     =2,
}DAC_mode;

typedef enum
{
  disable_ADC       =0,
  enable_ADC        =1,
}ADC_mode;

typedef enum
{
  dc                =0,
  saw               =1,
  sine              =2,
  rect              =3
}waveform;

typedef enum
{
  non_PWM           =0,
  PWM               =1
}trigger_output_type;

//=============================================================================
typedef struct
{
  galvo_type            galvo;                      // resonant or not
  DAC_mode              DAC_CH1_mode;               // fast or slow
  DAC_mode              DAC_CH2_mode;
  ADC_mode              ADC_CH1_mode;               // enable or disable
  int                   ADC_CH1_size;
  trigger_output_type   trigger;                    // PWM or not PWM
  int                   PWM_duty;
  int                   PWM_offset;
  waveform              DAC_CH1_waveform;           // type of waveform
  waveform              DAC_CH2_waveform;
  float                 DAC_CH1_waveform_sample;    // number of samples
  float                 DAC_CH2_waveform_sample;
  float                 DAC_CH1_waveform_spez;      // SPEZ is the dutycycle(%) for rect and symmetry(%)
  float                 DAC_CH2_waveform_spez;      // for saw and amplitude (% of max) for dc


}CFG_AWG;





#endif /* INCL_SORTED_H_ */
