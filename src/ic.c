/**
 * This file is part of PLTB.
 * Copyright (C) 2015 Michael Hoff, Stefan Orf and Benedikt Riehm
 *
 * PLTB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PLTB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PLTB.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <assert.h>
#include <math.h>
#include <pll/pll.h>

#include "ic.h"

/* Possible switch implementation: Use array of funcs.
 *
typedef double (calculate_IC_func) ( pllAlignmentData*, pllInstance* );

double calculate_AIC( pllAlignmentData*, pllInstance* );
double calculate_BIC( pllAlignmentData*, pllInstance* );

const calculate_IC_func *IC_funcs[IC_MAX] = {
&calculate_AIC, &calculate_BIC
};
*/

/**
 * Calculates the AIC
 * @param maxLogLikelihood is the maximum log-likelihood
 * @param freeParameters is the count of free parameters in the model
 * @return returns the AIC value
 */
static double calculateAIC(double maxLogLikelihood, unsigned freeParameters) {
	assert(maxLogLikelihood < 0);
	return -2 * maxLogLikelihood + 2 * (double)freeParameters;
}

/**
 * Calculates the corrected AIC (AICc)
 * @param max_log_likelihood is the maximum log-likelihood
 * @param free_params is the count of free parameters in the model
 * @return returns the AIC value
 */
static double calculateAICc(double max_log_likelihood, unsigned free_params, unsigned sample_size) {
	const double k = (double)free_params;
	const double n = (double)sample_size;
	return calculateAIC(max_log_likelihood, free_params) + ((2.f * k) * (k + 1.f)) / (n - k - 1.f);
}

/**
* Calculates the BIC
* @param maxLogLikelihood is the maximum log-likelihood
* @param freeParameters is the count of free parameters in the model
* @param numObservations is the number of observations
* @return returns the BIC value
*/
static double calculateBIC(double maxLogLikelihood, unsigned freeParameters, int numObservations) {
	assert(maxLogLikelihood < 0);
	return -2 * maxLogLikelihood + (double)freeParameters * log((double)numObservations);
}

void calculate_ICs( double* dst, pllAlignmentData *data, pllInstance *tree, unsigned parameter_count )
{
	for (unsigned i = 0; i < IC_MAX; i++) {
		dst[i] = calculate_IC((IC)i, data, tree, parameter_count);
	}
}

double calculate_IC( IC criterion, pllAlignmentData *data, pllInstance *tree, unsigned parameter_count )
{
	switch(criterion) {
		case AIC:
			return calculateAIC(tree->likelihood, parameter_count);
		case AICc_C:
			return calculateAICc(tree->likelihood, parameter_count, (unsigned int)data->sequenceLength);
		case AICc_RC:
			return calculateAICc(tree->likelihood, parameter_count, (unsigned int)(data->sequenceLength * data->sequenceCount));
		case BIC_C:
			return calculateBIC(tree->likelihood, parameter_count, data->sequenceLength);
		case BIC_RC:
			return calculateBIC(tree->likelihood, parameter_count,
					data->sequenceLength * data->sequenceCount);
		default:
		case IC_MAX:
			return 0.f;
	}
}
