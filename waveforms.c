/*
 * waveforms.c

 *
 *  Created on: Dec 10, 2016
 *      Author: kleej
 */
// GENERAL C LIB INCLUDES
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

//OWN FUNCTION INCLUDE
#include "waveforms.h"

//======================================================================
#define PI (3.141592653589793)
static float maximum_value = 4090;

//======================================================================

void sawfct(CFG_AWG* CFG) {
  if (CFG->DAC_CH1_mode == saw) {
    int samples_before_peak = CFG->DAC_CH1_waveform_sample
        * (CFG->DAC_CH1_waveform_spez / 100.0f);
    int samples_after_peak = CFG->DAC_CH1_waveform_sample - samples_before_peak;
    float slope_before_peak = maximum_value / samples_before_peak;
    float slope_after_peak = -(maximum_value / samples_after_peak);

    for (int i = 0; i < CFG->DAC_CH1_waveform_sample; i++) {
      if (i <= samples_before_peak) {
        wave1[i] = i * slope_before_peak;
      }
      else {
        wave1[i] = (unsigned)(maximum_value
            + (i - samples_before_peak) * slope_after_peak);
      }
    }
  }
  if (CFG->DAC_CH2_waveform == saw) {
    int samples_before_peak = CFG->DAC_CH2_waveform_sample
        * (CFG->DAC_CH2_waveform_spez / 100.0f);
    int samples_after_peak = CFG->DAC_CH2_waveform_sample - samples_before_peak;
    float slope_before_peak = maximum_value / samples_before_peak;
    float slope_after_peak = -(maximum_value / samples_after_peak);

    for (int i = 0; i < CFG->DAC_CH2_waveform_sample; i++) {
      if (i <= samples_before_peak) {
        wave2[i] = i * slope_before_peak;
      }
      else {
        wave2[i] = (unsigned)(maximum_value
            + (i - samples_before_peak) * slope_after_peak);
      }
    }
  }
}

//=======================================================================

void constfct(CFG_AWG* CFG) {
  if (CFG->DAC_CH1_waveform == dc) {
    for (int i = 0; i < CFG->DAC_CH1_waveform_sample; i++) {
      wave1[i] = maximum_value * CFG->DAC_CH1_waveform_spez / 100.0f;
    }
  }
  else if (CFG->DAC_CH2_waveform == dc) {
    for (int i = 0; i < CFG->DAC_CH2_waveform_sample; i++) {
      wave2[i] = maximum_value * CFG->DAC_CH2_waveform_spez / 100.0f;
    }
  }
}

void sinefct(CFG_AWG* CFG) {
  if (CFG->DAC_CH1_waveform == sine) {
    float dx = 2 * PI / CFG->DAC_CH1_waveform_sample;

    for (int i = 0; i < CFG->DAC_CH1_waveform_sample; i++) {
      wave1[i] = sinf(i * dx) * maximum_value / 2 + maximum_value / 2;
    }
  }
  else if (CFG->DAC_CH2_waveform == sine) {
    float dx = 2 * PI / CFG->DAC_CH2_waveform_sample;

    for (int i = 0; i < CFG->DAC_CH2_waveform_sample; i++) {
      wave2[i] = sinf(i * dx) * maximum_value / 2 + maximum_value / 2;
    }
  }
}

//=========================================================================

void rectfct(CFG_AWG* CFG) {
  if (CFG->DAC_CH1_waveform == rect) {
    int samples_before_peak = CFG->DAC_CH1_waveform_sample
        * (CFG->DAC_CH1_waveform_spez / 100.0f);

    for (int i = 0; i < CFG->DAC_CH1_waveform_sample; i++) {
      if (i <= samples_before_peak) {
        wave1[i] = maximum_value;
      }
      else {
        wave1[i] = 0;
      }
    }
  }
  else if (CFG->DAC_CH2_waveform == rect) {
    int samples_before_peak = CFG->DAC_CH2_waveform_sample
        * (CFG->DAC_CH2_waveform_spez / 100.0f);

    for (int i = 0; i < CFG->DAC_CH2_waveform_sample; i++) {
      if (i <= samples_before_peak) {
        wave2[i] = maximum_value;
      }
      else {
        wave2[i] = 0;
      }
    }
  }
}
