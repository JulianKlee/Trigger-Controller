#pragma once

#ifndef PIDCTRL_H
#define PIDCTRL_H

typedef struct pidctrl_parameters_T {
	/* proportional factor - must not be 0 or NAN */
	float Kp;
	/* proportional minimum limit - must be < 0; set to NAN or FLT_MIN for no limit */
	float p_min;
	/* proportional maximum limit - must be > 0; set to NAN or FLT_MAX for no limit */
	float p_max;

	/* derivative factor - set to NAN or 0 to disable */
	float Kd;
	/* derivative minimum limit - must be < 0; set to NAN or FLT_MIN for no limit */
	float d_min;
	/* derivative maximum limit - must be > 0; set to NAN or FLT_MAX for no limit */
	float d_max;

	/* integrative factor - set to NAN or 0 to disable */
	float Ki;
	/* integrative minimum limit - must be < 0; set to NAN or FLT_MIN for no limit */
	float i_min;
	/* integrative maximum limit - must be > 0; set to NAN or FLT_MAX for no limit */
	float i_max;
	/* integration decay - must be in the range [0..1[; set to NAN or 0 for standard behavior */
	float i_dcy;

#if 0
	/* accumulation minimum - must be < 0; set to NAN or FLT_MIN for no limit */
	float i_accum_min;
	/* accumulation maximum - must be > 0; set to NAN or FLT_MAX for no limit */
	float i_accum_max;
#endif
} pidctrl_parameters;

#define PIDCTRL_DEFAULT_PARAMETERS_NOCONTROL { 0, NAN, NAN,  0, NAN, NAN,  0, NAN, NAN, NAN }

#define PIDCTRL_FLAG_PROPORTIONAL (1 << 0)
#define PIDCTRL_FLAG_P_MIN (1 << 1)
#define PIDCTRL_FLAG_P_MAX (1 << 2)

#define PIDCTRL_FLAG_DERIVATIVE (1 << 3)
#define PIDCTRL_FLAG_D_MIN (1 << 4)
#define PIDCTRL_FLAG_D_MAX (1 << 5)

#define PIDCTRL_FLAG_INTEGRATIVE (1 << 6)
#define PIDCTRL_FLAG_I_MIN (1 << 7)
#define PIDCTRL_FLAG_I_MAX (1 << 8)
#define PIDCTRL_FLAG_I_DCY (1 << 9)

#if 0
#define PIDCTRL_FLAG_I_ACCUM_MIN (1 << 10)
#define PIDCTRL_FLAG_I_ACCUM_MAX (1 << 11)
#endif

/* descrete timestep PID regulator status */
typedef struct pidctrl_status_T {
	unsigned int flags;

	/* integrated error */
	float r_i;

	/* value of pv in previous timestep */
	float pv_prev;
} pidctrl_status;

int pidctrl_init(
	pidctrl_parameters const * parameters,
	pidctrl_status * status );

#define PIDCTRL_STEP_SATURATED_P (3 << 0)
#define PIDCTRL_STEP_SATURATED_P_MIN (1 << 0)
#define PIDCTRL_STEP_SATURATED_P_MAX (1 << 1)

#define PIDCTRL_STEP_SATURATED_D (3 << 2)
#define PIDCTRL_STEP_SATURATED_D_MIN (1 << 2)
#define PIDCTRL_STEP_SATURATED_D_MAX (1 << 3)

#define PIDCTRL_STEP_SATURATED_I (3 << 4)
#define PIDCTRL_STEP_SATURATED_I_MIN (1 << 4)
#define PIDCTRL_STEP_SATURATED_I_MAX (1 << 5)

#define PIDCTRL_STEP_CV_IS_NAN (1 << 6)

/* Compute a Control Value (CV) from current Process Value (PV) and Setpoint (SP)
 *
 * Process Value and Setpoint are passed separately instead of an error value
 * to allow instantanous changes in the setpoint without creating a large
 * derivative control value term.
 *
 *      err(t) = pv(t) - sp
 * d/dt err(t) = d/dt pv(t) - d/dt sp ; assume constant sp
 * d/dt err(t) = d/dt pv(t)
 *
 * The reasoning behind this is, that the setpoint is quasi-constant and changes
 * only as a user controlled parameter and does not depend on any process dynamics.
 *
 * returns  > 0 bitmask indicating control term saturation
 *         == 0 if control terms within limits
 *         = -1 if invalid parameters
 */
int pidctrl_step(
	pidctrl_parameters const * parameters,
	pidctrl_status * status,
	float T,
	float pv,
	float sp,
	float * cv );

#endif/*PIDCTRL_H*/
