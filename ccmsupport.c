#include <ch.h>
#include <hal.h>

#include "ccmsupport.h"

// This is for basic testing of CCM. Could be removed but OTOH, we don't 
// care about a 16 bytes of CCM anyway...
static _ccm_data_ uint32 test_ccm_data[2]={13,42};
static _ccm_bss_  uint32 test_ccm_bss[2];

#if STM32_WW_ENABLE_CCM_RAM
// Provided by linker script. 
extern uint32_t _ccm_textdata;
extern uint32_t _textdata;  // FIXME: NOt needed
extern uint32_t _ccm_data_start, _ccm_data_end;
extern uint32_t _ccm_bss_start, _ccm_bss_end;
#endif

#define fill32(start, end, filler) {                                        \
  uint32_t *p1 = start;                                                     \
  uint32_t *p2 = end;                                                       \
  while (p1 < p2)                                                           \
    *p1++ = filler;                                                         \
}

// Called from crt0.c.
void __late_init(void)
{
	/* DATA segment initialization.*/
	{
		uint32_t *tp, *dp;
		
		tp = &_ccm_textdata;
		dp = &_ccm_data_start;
		while (dp < &_ccm_data_end)
		*dp++ = *tp++;
	}
	
	fill32(&_ccm_bss_start, &_ccm_bss_end, 0);
}
