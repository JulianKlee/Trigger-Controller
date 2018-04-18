#pragma once
#ifndef WAV_GEN_H
#define WAV_GEN_H

#include <stddef.h>
#include <stdint.h>

enum wav_wfm__type {
	WAV_WFM_UNDEFINED = 0,
	WAV_WFM_SINE      = 1,
	WAV_WFM_TRIANGLE  = 2,
};

int wav_gen_len(
	unsigned ch,
	size_t len );

int wav_gen_wfm(
	unsigned ch,
	unsigned type,
	float amp,
	float off,
	float pha,
	float aspect );

int wav_gen_wfm_qry(
	unsigned ch,
	unsigned *out_type,
	float *out_amp,
	float *out_off,
	float *out_pha,
	float *out_aspect );

int wav_gen_disarm(unsigned chs);
int wav_gen_arm(unsigned chs);

int wav_gen_init(void);

#endif/*WAV_GEN_H*/
