#define _GNU_SOURCE
#include <ch.h>
#include <chvt.h>
#include <hal.h>

#include "regulator.h"

#include <math.h>

#include "pidctrl.h"
#include "wav/wav_mon.h"
#include "wav/wav_gen.h"

enum {
	REG_CONTROLLER_INTERVAL_MSEC = 100,
	REG_CONTROLLER_MIN_dT_ms =  70,
	REG_CONTROLLER_MAX_dT_ms = 500,
};

struct RegControllerPID {
	pidctrl_parameters parameters;
	pidctrl_status     state;
	float              target;
};

static struct RegControllerPID phase = {
	{
		.Kp = 0.25f, .p_min = -0.5f*M_PI, .p_max = 0.5f*M_PI,
		.Kd = 0,    .d_min = NAN,   .d_max = NAN,
		.Ki = 1.5f, .i_min = -2.2f*M_PI, .i_max = 2.2f*M_PI
	},
	{0, 0, 0},
	NAN
};
static struct RegControllerPID amplitude = {
	{
		.Kp = 0.02f, .p_min = -0.1f, .p_max =  0.1f,
		.Kd = 0,     .d_min =   NAN, .d_max =   NAN,
		.Ki = 2.00f, .i_min =  0.0f, .i_max =  2.0f,
	},
	{0, 0, 0},
	NAN
};

static
int reg_controller_step_amplitude_phase(float const dT)
{
	float measured_amplitude;
	float measured_phase;
	if( wav_mon_eval(&measured_amplitude, &measured_phase) ){
		return -1;
	}

	float cv_amplitude, cv_phase;
	if( wav_gen_wfm_qry(0, NULL, &cv_amplitude, NULL, &cv_phase, NULL) ){
		return -2;
	}
	float const old_cv_amplitude = cv_amplitude;
	float const old_cv_phase = cv_phase;

	if( isfinite(amplitude.target) ){
		pidctrl_step(
			&amplitude.parameters,
			&amplitude.state,
			dT,
			measured_amplitude,
			amplitude.target,
			&cv_amplitude);
	}

	if( isfinite(phase.target) ){
		pidctrl_step(
			&phase.parameters,
			&phase.state,
			dT,
			measured_phase,
			phase.target,
			&cv_phase);

		if( ( 1.05f*M_PI < phase.state.r_i) ) {
			phase.state.r_i =
				-M_PI + fmodf( phase.state.r_i, M_PI);
		} else
		if( (-1.05f*M_PI > phase.state.r_i) ) {
			phase.state.r_i =
				 M_PI - fmodf(-phase.state.r_i, M_PI);
		}

		/* cyclic adjustment happens after phase cv is read from the
		 * PID => the delta is unaffected and consistent even in case
		 * of cyclic wraparound. */

	}

	if( old_cv_amplitude == cv_amplitude ) {
		cv_amplitude = NAN; /* no change */
	}

	if( old_cv_phase == cv_phase ) {
		cv_phase = NAN; /* no change */
	}

	// Absolutely vital to not set a new amplitude unless it really
	// changes because uploading a new wfm takes 5 msec.
	// FIXME: For amplitude we could actually only update if it changes
	//   by more than some value >0.
	if( isfinite(cv_amplitude)
	 || isfinite(cv_phase)
	){
		if( wav_gen_wfm(0, 0, cv_amplitude, NAN, cv_phase, NAN) ){
			return -3;
		}
	}

	return 0;
}

int regulator_target(
	float amp,
	float pha)
{
	if( isfinite(amp)
	 && 0.f <= amp
	){
		amplitude.target = amp;
	}
	if( isfinite(pha) ){
		phase.target = fmod(pha, 2*M_PI);
	}
	return 0;
}

static THD_WORKING_AREA(waRSRegulator, 4096);
static THD_FUNCTION(RSRegulator, arg){
	(void)arg;
	chRegSetThreadName("rsregulator");
	for(;;){
		chThdSleepMilliseconds(REG_CONTROLLER_INTERVAL_MSEC);
		reg_controller_step_amplitude_phase(REG_CONTROLLER_INTERVAL_MSEC*1e-3f);
	}
}

int regulator_start(void)
{
	phase.target = 0.0f;
	amplitude.target = 0.5f;
	pidctrl_init(&phase.parameters, &phase.state);
	pidctrl_init(&amplitude.parameters, &amplitude.state);

	chThdCreateStatic(waRSRegulator, sizeof(waRSRegulator), NORMALPRIO, RSRegulator, NULL);
	return 0;
}
