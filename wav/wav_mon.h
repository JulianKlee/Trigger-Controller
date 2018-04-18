#pragma once
#ifndef WAV_MON_H
#define WAV_MON_H

#include <stddef.h>
#include <stdint.h>

enum { WAV_MON_LEN_MAX = 2048 };
extern uint16_t wav_mon__wfmbuf[];
extern size_t wav_mon__wfmlen;

int wav_mon_init(void);
int wav_mon_arm(void);
int wav_mon_len(size_t len);
int wav_mon_eval(float *amplitude, float *phase);

#endif/*WAV_MON_H*/
