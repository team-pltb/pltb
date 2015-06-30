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
#ifndef PLTB_FRONTEND_H
#define PLTB_FRONTEND_H

#include <pll/pll.h>
#include "pltb.h"
#include "models.h"

#define OUTPUT_WIDTH 107

void fprint_progress_begin(FILE *f);

unsigned fprint_progress_step(FILE *f, unsigned last, unsigned curr, unsigned max);

void fprint_progress_end(FILE *f);

void fprint_eval_header(FILE *f);

void fprint_eval_row(FILE *f, model_space_t *model_space, pltb_model_stat_t *stat);

void fprint_eval_summary(FILE *f, model_space_t *model_space, pltb_model_stat_t (*stats)[], pltb_result_t *result);

void evaluate_result( model_space_t *model_space, pltb_result_t *result, pllAlignmentData *data, pltb_config_t *config);

char *get_IC_name_short(IC criterion);

char *get_IC_name_long(IC criterion);

#endif
