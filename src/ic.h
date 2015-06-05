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
#ifndef IC_H
#define IC_H

#include <pll/pll.h>

/* all information criteria included */
typedef enum { AIC, AICc_C, AICc_RC, BIC_C, BIC_RC, IC_MAX = BIC_RC + 1 } IC;

/* calculate one specific crterion value */
double calculate_IC( IC criterion, pllAlignmentData*, pllInstance*, unsigned );

/* calculate the values to all criteria available */
void calculate_ICs( double* dst, pllAlignmentData*, pllInstance*, unsigned );

#endif
