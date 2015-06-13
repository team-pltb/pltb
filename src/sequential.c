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
#include <float.h>
#include "debug.h"
#include "pltb.h"
#include "pltb_frontend.h"

#include "sequential.h"

#ifdef __APPLE__
#include "time_mach.h"
#else
#include "time.h"
#endif

#define DEBUG_PROCESS_STATISTICS_OPEN_OUTPUT stdout
#define DEBUG_PROCESS_STATISTICS_CLOSE_OUTPUT(f)
//#define DEBUG_PROCESS_STATISTICS_OPEN_OUTPUT fopen("performance.txt", "w")
//#define DEBUG_PROCESS_STATISTICS_CLOSE_OUTPUT(f) fclose(f)

int run_sequential( char *dataset_file, pltb_config_t *config, model_space_t *model_space )
{
	FILE *out = DEBUG_PROCESS_STATISTICS_OPEN_OUTPUT;
	pltb_model_stat_t stats[model_space->matrix_count];

	TIME_STRUCT_INIT(timer);

	fprint_eval_header(out);

	pllAlignmentData *data = read_alignment_data(dataset_file);

	pltb_result_t result;

	for(unsigned i = 0; i < IC_MAX; i++) {
		result.ic[i] = FLT_MAX;
	}

	while (next_model(model_space)) {
		partitionList *parts = init_partitions(data, config->base_freq_kind);
		pllInstance *inst = setup_instance(model_space->matrix_repr, &config->attr_model_eval, data, parts);

		pltb_model_stat_t *stat = &stats[model_space->matrix_index];
		stat->matrix_index = model_space->matrix_index;
		TIME_START(timer);

		optimize_model_parameters(inst, parts);

		TIME_END(timer);
		stat->time_cpu  = TIME_CPU(timer);
		stat->time_real = TIME_REAL(timer);

		stat->likelihood = inst->likelihood;
		calculate_model_ICs(stat, data, inst, model_space->free_parameter_count, config);
		merge_into_result(&result, stat, model_space->matrix_index);

		fprint_eval_row(out, model_space, stat);

		pllPartitionsDestroy(inst, &parts);
		pllDestroyInstance(inst);
	}
	fprint_eval_summary(out, model_space, &stats, &result);
	DEBUG_PROCESS_STATISTICS_CLOSE_OUTPUT(out);

	evaluate_result(model_space, &result, data, config);

	pllAlignmentDataDestroy(data);
	return 0;
}
