/*
 * sorted.c
 *
 *  Created on: Aug 25, 2017
 *      Author: kleej
 */
//C INCLUDE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

//CHIBIOS INCLUDES
#include "ch.h"
#include "hal.h"
//#include "test.h"
#include "shell.h"
#include "chprintf.h"
#include "usbcfg.h"

//GENERAL HAL LIBRARY INCLUDES
#include "incl\stm32f4xx.h"
#include "incl\stm32f4xx_rcc.h"
#include "incl\stm32f4xx_tim.h"
#include "incl\stm32f4xx_gpio.h"
#include "incl\stm32f4xx_dac.h"
#include "incl\stm32f4xx_dma.h"
#include "incl\stm32f4xx_exti.h"
#include "incl\stm32f4xx_syscfg.h"
#include "incl\misc.h"
#include "incl\stm32f4xx_adc.h"

//OWN INCLUDES
#include "waveforms.h"

#include "wav/wav_gen.h"
#include "wav/wav_mon.h"
#include "wav/wav_clk.h"
#include "wav/wav_sync.h"

#include "regulator.h"

//=======================global and statics==========================
uint16_t wave1[2048];
uint16_t wave2[2048];

uint16_t adcBuffer[2048];
uint16_t adcData[2048];

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

//=======================MCU Blinker Thread=============================
// check MCU Blinker thread
static THD_WORKING_AREA(waThread1, 64);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    systime_t time;

    time = serusbcfg.usbp->state == USB_ACTIVE ? 250 : 500;
    palClearPad(GPIOD, GPIOD_LED4);
    chThdSleepMilliseconds(time);
    palSetPad(GPIOD, GPIOD_LED4);
    chThdSleepMilliseconds(time);

  }
}

//===========================NVIC for C-Frame========================================

/* Triggered when the button is pressed or released. The LED5 is set to ON.*/
static void volume_trigger_isr(EXTDriver *extp, expchannel_t channel) {
	(void)extp;
	(void)channel;

	unsigned const lvl = palReadPad(GPIOD, 5);
	if( !lvl ){
		/* start condition */
		wav_clk_bscan_output(0);
	} else {
		wav_clk_bscan_output(1);
	}
}

//===========================Commands go here================================
static void cmd_waveform(BaseSequentialStream *chp, int argc, char *argv[]) {
	(void)argv;
	if (argc != 0) {
		chprintf(chp, "Usage: waveform\r\n");
		return;
	}

	for( size_t i = 0; i < wav_mon__wfmlen; ++i ){
		chprintf(chp, "%d\r\n", wav_mon__wfmbuf[i]);
	}
}

float f_ref_kHz = 425.f;
float f_glv_kHz = 1.f;

static
void wav_len_apply(void)
{
	unsigned samples, div;
	wav_clk_cfg(f_ref_kHz, f_glv_kHz, &samples, &div);
	wav_gen_len(0, samples*div);
	wav_mon_len(samples);
	wav_sync();
}

enum {
	INIT_RISE = 240,
	INIT_FALL = 4,
};

unsigned rise   = INIT_RISE;
unsigned fall   = INIT_FALL;

static
void wav_ylen_apply(void)
{
	unsigned sync = 0;
	static unsigned trilen = 0;
	if( trilen != (rise + fall) ){
		trilen = rise + fall;
		wav_gen_len(1, trilen);
		sync = 1;
	}
	wav_gen_wfm(1, WAV_WFM_TRIANGLE, NAN, NAN, NAN, rise);
	if( sync ){
		wav_sync();
	}
}

static
void cmd_reso_galvo_ref(BaseSequentialStream *chp, int argc, char *argv[])
{
	char *ep = NULL;
	if( 1 > argc ){
		return;
	}
	float const v = strtol(argv[0], &ep, 0) * 1e-3;

	if( ep && argv[0] != ep ){
		f_ref_kHz = v;
		wav_len_apply();
	}
}

static
void cmd_reso_galvo_frq(BaseSequentialStream *chp, int argc, char *argv[])
{
	char *ep = NULL;
	if( 1 > argc ){
		return;
	}
	float const v = strtol(argv[0], &ep, 0) * 1e-3;

	if( ep && argv[0] != ep ){
		f_glv_kHz = v;
		wav_len_apply();
	}
}

static
void cmd_reso_galvo_amp(BaseSequentialStream *chp, int argc, char *argv[])
{
	char *ep = NULL;
	if( 1 > argc ){
		return;
	}
	float amp = strtol(argv[0], &ep, 0) * 1e-3;

	if( ep && argv[0] != ep ){
		regulator_target(amp, NAN);
	}
}

static
void cmd_reso_galvo_pha(BaseSequentialStream *chp, int argc, char *argv[])
{
	char *ep = NULL;
	if( 1 > argc ){
		return;
	}
	float pha = strtol(argv[0], &ep, 0) * 1e-6;

	if( ep && argv[0] != ep ){
		regulator_target(NAN, pha);
	}
}


/* ====== y ====== */

static
void cmd_y_scan_rise(BaseSequentialStream *chp, int argc, char *argv[])
{
	char *ep = NULL;
	if( 1 > argc ){
		return;
	}
	unsigned const v = strtol(argv[0], &ep, 0);

	if( ep && argv[0] != ep ){
		rise = v;
		wav_ylen_apply();
	}
}

static
void cmd_y_scan_fall(BaseSequentialStream *chp, int argc, char *argv[])
{
	char *ep = NULL;
	if( 1 > argc ){
		return;
	}
	unsigned const v = strtol(argv[0], &ep, 0);

	if( ep && argv[0] != ep ){
		fall = v;
		wav_ylen_apply();
	}
}

static
void cmd_y_scan_amp(BaseSequentialStream *chp, int argc, char *argv[])
{
	char *ep = NULL;
	if( 1 > argc ){
		return;
	}
	float amp = strtol(argv[0], &ep, 0) * 1e-3;

	if( ep && argv[0] != ep ){
		wav_gen_wfm(1, 0, amp, NAN, NAN, NAN);
	}
}

static
void cmd_y_scan_off(BaseSequentialStream *chp, int argc, char *argv[])
{
	char *ep = NULL;
	if( 1 > argc ){
		return;
	}
	float off = strtol(argv[0], &ep, 0) * 1e-3;

	if( ep && argv[0] != ep ){
		wav_gen_wfm(1, 0, NAN, off, NAN, NAN);
	}
}

//===========================Shell config=================================

static const ShellCommand commands[] = {
	{"waveform", cmd_waveform},
	{"rgr", cmd_reso_galvo_ref},
	{"rgf", cmd_reso_galvo_frq},
	{"rga", cmd_reso_galvo_amp},
	{"rgp", cmd_reso_galvo_pha},

	{"ysr", cmd_y_scan_rise},
	{"ysf", cmd_y_scan_fall},
	{"ysa", cmd_y_scan_amp},
	{"yso", cmd_y_scan_off},

	{NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
	(BaseSequentialStream *)&SDU1,
	commands
};

//============================External Interrupt Config==============================

static const EXTConfig extcfg = {
//	.channels[0 ... EXT_MAX_CHANNELS - 1] = {EXT_CH_MODE_DISABLED, NULL},
// Add all external interrupt sources here.
	.channels[ 5] = {EXT_CH_MODE_BOTH_EDGES  | EXT_MODE_GPIOD | EXT_CH_MODE_AUTOSTART, volume_trigger_isr}, //NVIC EXTI
	.channels[ 9] = {EXT_CH_MODE_RISING_EDGE | EXT_MODE_GPIOB | EXT_CH_MODE_AUTOSTART, NULL},   // DAC EXTI
	.channels[11] = {EXT_CH_MODE_RISING_EDGE | EXT_MODE_GPIOE | EXT_CH_MODE_AUTOSTART, NULL},   // ADC EXTI
};

//=====================================================================

int main(void)
{

	thread_t *shelltp = NULL;

	/*
	* System initializations.
	* - HAL initialization, this also initializes the configured device drivers
	*   and performs the board-specific initializations.
	* - Kernel initialization, the main() function becomes a thread and the
	*   RTOS is active.
	*/
	halInit();
	chSysInit();

	//create the MCU check blinker thread
	chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

#if 1
	extStart(&EXTD1, &extcfg);
#endif

#if 1
	wav_sync_init();
	wav_clk_init();
	wav_gen_init();
	wav_mon_init();

#endif

	wav_len_apply();
	wav_ylen_apply();
	
	wav_sync();

	/*
	* Initializes a serial-over-USB CDC driver.
	*/
	sduObjectInit(&SDU1);
	sduStart(&SDU1, &serusbcfg);

	/*
	* Activates the USB driver and then the USB bus pull-up on D+.
	* Note, a delay is inserted in order to not have to disconnect the cable
	* after a reset.
	*/
	usbDisconnectBus(serusbcfg.usbp);
	chThdSleepMilliseconds(500);
	usbStart(serusbcfg.usbp, &usbcfg);
	usbConnectBus(serusbcfg.usbp);
	//==========================================================
	chThdSleepMilliseconds(500);

	regulator_start();

	//==========================================================================
	/*
	* Shell manager initialization.
	*/

	shellInit();

	// extChannelEnable(&EXTD1, 4);

	/*
	* Normal main() thread activity, in this demo it does nothing except
	* sleeping in a loop and check the button state.
	*/
	for(;;){
		if( SDU1.config->usbp->state == USB_ACTIVE ){
			shelltp = chThdCreateFromHeap(
				NULL, SHELL_WA_SIZE,
				"shell",
				NORMALPRIO + 1,
				shellThread,
				(void*)&shell_cfg1 );
			chThdWait(shelltp);
		}
		chThdSleepMilliseconds(100);
	}
	return 0;
}
