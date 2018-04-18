#include "wav_sync.h"
#include "wav_clk.h"
#include "wav_gen.h"
#include "wav_mon.h"

#include <stdint.h>
#include <stddef.h>

#include "incl/stm32f4xx.h"
#include "incl/stm32f4xx_rcc.h"
#include "incl/stm32f4xx_gpio.h"
#include "incl/misc.h"

#include "../portpin.h"

enum {
	rcc_enable_ahb1 = 
		RCC_AHB1Periph_GPIOC,
};

static portpin sync_out = { GPIOC, 13, 0 };

static
int wav_sync_gate(unsigned gate)
{
	GPIO_WriteBit(sync_out.port, (1 << sync_out.pin), gate ? Bit_RESET : Bit_SET);
	return 0;
}

int wav_sync(void)
{
	wav_sync_gate(0);
	wav_gen_arm( (1<<0) );
	wav_mon_arm();
	wav_clk_sync();
	wav_sync_gate(1);
	return 0;
}

int wav_sync_init(void)
{
	RCC_AHB1PeriphClockCmd(rcc_enable_ahb1, ENABLE);
	portpin_cfg_out(&sync_out);
	wav_sync_gate(0);
	return 0;
}
