#pragma once
#ifndef PORTPIN_H
#define PORTPIN_H

#include <stdint.h>

typedef struct portpin_s {
	void *port;
	uint32_t pin;
	unsigned af;
} const portpin;

void portpin_cfg_in(portpin *pp);
void portpin_cfg_out(portpin *pp);
void portpin_cfg_analog(portpin *pp);

#endif/*PORTPIN_H*/
