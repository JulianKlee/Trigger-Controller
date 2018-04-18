#include "portpin.h"

#include "incl/stm32f4xx.h"
#include "incl/stm32f4xx_gpio.h"

void portpin_cfg_in(portpin *pp)
{
	GPIO_InitTypeDef const cfg = {
		.GPIO_Pin   = 1 << pp->pin,
		.GPIO_Mode  = pp->af ? GPIO_Mode_AF : GPIO_Mode_IN,
		.GPIO_PuPd  = GPIO_PuPd_DOWN,
	};
	GPIO_PinAFConfig(pp->port, pp->pin, pp->af);
	GPIO_Init(pp->port, &cfg);
}

void portpin_cfg_out(portpin *pp)
{
	GPIO_InitTypeDef const cfg = {
		.GPIO_Pin   = 1 << pp->pin,
		.GPIO_Speed = GPIO_Speed_25MHz,
		.GPIO_OType = GPIO_OType_PP,
		.GPIO_PuPd  = GPIO_PuPd_NOPULL,
		.GPIO_Mode  = pp->af ? GPIO_Mode_AF : GPIO_Mode_OUT,
	};
	GPIO_PinAFConfig(pp->port, pp->pin, pp->af);
	GPIO_Init(pp->port, &cfg);
}

void portpin_cfg_analog(portpin *pp)
{
	GPIO_InitTypeDef const cfg = {
		.GPIO_Pin   = 1 << pp->pin,
		.GPIO_Speed = GPIO_Speed_2MHz,
		.GPIO_OType = GPIO_OType_OD,
		.GPIO_Mode  = GPIO_Mode_AN,
		.GPIO_PuPd  = GPIO_PuPd_NOPULL,
	};
	GPIO_PinAFConfig(pp->port, pp->pin, 0);
	GPIO_Init(pp->port, &cfg);
}
