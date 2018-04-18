#include "pidctrl.h"

#include <math.h>
#include <float.h>

int pidctrl_init(
	pidctrl_parameters const * const parameters,
	pidctrl_status * const status )
{
	status->r_i     = 0;
	status->pv_prev = 0;
	status->flags   = 0;

	if( !parameters
	 || !status ) {
		return -1;
	}

	/* fnord: FLT_MIN is defined as the smallest positive number
	 * representible by a float. Use -FLT_MAX for the negative end
	 * of the representable value range. 
	 * ---
	 *  Whoever gave the C Standard comitte the drugs when they defined
	 *  that, I want to have a word with you. You name something like
	 *  that FLT_SMALLEST_POS or similar. -- dw */

	if( isfinite(parameters->Kp)
	 && fabsf(parameters->Kp) > FLT_EPSILON ) {
		if( (isfinite(parameters->p_min) && (parameters->p_min >= 0))
		 || (isfinite(parameters->p_max) && (parameters->p_max <= 0))
		 || (  isfinite(parameters->p_min)
		    && isfinite(parameters->p_max)
		    && (parameters->p_max <= parameters->p_min))
		) {
			return -1;
		}

		if( isfinite(parameters->p_min) ) {
			status->flags |= PIDCTRL_FLAG_P_MIN;
		}
		if( isfinite(parameters->p_max) ) {
			status->flags |= PIDCTRL_FLAG_P_MAX;
		}
		status->flags |= PIDCTRL_FLAG_PROPORTIONAL;
	}

	if( isfinite(parameters->Kd)
	 && fabsf(parameters->Kd) > FLT_EPSILON ) {
		if( (isfinite(parameters->d_min) && (parameters->d_min >= 0))
		 || (isfinite(parameters->d_max) && (parameters->d_max <= 0))
		 || (  isfinite(parameters->d_min)
		    && isfinite(parameters->d_max)
		    && (parameters->d_max <= parameters->d_min))
		) {
			return -1;
		}

		if( isfinite(parameters->d_min) ) {
			status->flags |= PIDCTRL_FLAG_D_MIN;
		}
		if( isfinite(parameters->d_max) ) {
			status->flags |= PIDCTRL_FLAG_D_MAX;
		}
		status->flags |= PIDCTRL_FLAG_DERIVATIVE;
	}

	if( isfinite(parameters->Ki)
	 && fabsf(parameters->Ki) > FLT_EPSILON ) {
		if( isfinite(parameters->i_min) ) {
			status->flags |= PIDCTRL_FLAG_I_MIN;
		}
		if( isfinite(parameters->i_max) ) {
			status->flags |= PIDCTRL_FLAG_I_MAX;
		}
		if( isfinite(parameters->i_dcy)
		 && (FLT_EPSILON < parameters->i_dcy)
		 && (1 > parameters->i_dcy) ) {
			status->flags |= PIDCTRL_FLAG_I_DCY;
		}
		status->flags |= PIDCTRL_FLAG_INTEGRATIVE;
	}

	return 0;
}

int pidctrl_step(
	pidctrl_parameters const * const parameters,
	pidctrl_status * const status,
	float const dT,
	float const pv,
	float const sp,
	float * const cv )
{
	if( !(parameters)
	 || !(status)
	 || !(cv) ) {
		return -1;
	}

	int ret = 0;
	*cv = 0;

	float const err = sp - pv;

	if( status->flags & PIDCTRL_FLAG_PROPORTIONAL ) {
		float r_p = parameters->Kp * err;
		if( (status->flags & PIDCTRL_FLAG_P_MIN)
		 && (parameters->p_min > r_p) ) {
			r_p = parameters->p_min;
			ret |= PIDCTRL_STEP_SATURATED_P_MIN;
		}
		if( (status->flags & PIDCTRL_FLAG_P_MAX)
		 && (parameters->p_max < r_p) ) {
			r_p = parameters->p_max;
			ret |= PIDCTRL_STEP_SATURATED_P_MAX;
		}
		*cv += r_p;
	}

	if( status->flags & PIDCTRL_FLAG_DERIVATIVE ) {
		/*      err(t) = pv(t) - sp
		 * d/dt err(t) = d/dt pv(t) - d/dt sp ; assume constant sp
		 * d/dt err(t) = d/dt pv(t)
		 */
		float const deriv = pv - status->pv_prev;
		status->pv_prev = pv;

		float r_d = parameters->Kd * deriv / dT;
		if( (status->flags & PIDCTRL_FLAG_D_MIN)
		 && (parameters->d_min > r_d) ) {
			r_d = parameters->d_min;
			ret |= PIDCTRL_STEP_SATURATED_D_MIN;
		}
		if( (status->flags & PIDCTRL_FLAG_D_MAX)
		 && (parameters->d_max < r_d) ) {
			r_d = parameters->d_max;
			ret |= PIDCTRL_STEP_SATURATED_D_MAX;
		}
		*cv += r_d;
	}

	if( status->flags & PIDCTRL_FLAG_INTEGRATIVE ) {
		if( !isfinite(status->r_i) ) {
			status->r_i = 0.0f;
		}

		if( status->flags & PIDCTRL_FLAG_I_DCY ) {
			status->r_i -=
				status->r_i
				* parameters->i_dcy;
		}
		status->r_i += parameters->Ki * err * dT;

		if( (status->flags & PIDCTRL_FLAG_I_MIN)
		 && (parameters->i_min > status->r_i) ) {
			status->r_i = parameters->i_min;
			ret |= PIDCTRL_STEP_SATURATED_I_MIN;
		}
		if( (status->flags & PIDCTRL_FLAG_I_MAX)
		 && (parameters->i_max < status->r_i) ) {
			status->r_i = parameters->i_max;
			ret |= PIDCTRL_STEP_SATURATED_I_MAX;
		}
	}
	/* Even if I error is disabled, r_i may be set to a nonzero value
	 * for control variable (CV) bias. */
	*cv += status->r_i;

	return ret | (isfinite(*cv) ? 0 : PIDCTRL_STEP_CV_IS_NAN);
}
