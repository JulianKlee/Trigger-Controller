#pragma once
#ifndef WAV_CLK_H
#define WAV_CLK_H

int wav_clk_cfg(
	float const ascan_kHz,
	float const bscan_kHz,
	unsigned *const out_samples,
	unsigned *const out_div );
int wav_clk_sync(void);
int wav_clk_bscan_output(unsigned enable);
int wav_clk_init(void);

#endif/*WAV_CLK_H*/
