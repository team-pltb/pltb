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
#include <stddef.h>

#include "mpi_backend.h"

void result_reduce(void *in, void *inout, int *len, MPI_Datatype *datatype) {
	(void)datatype; // unused!
	unsigned iterations = (unsigned int) *len;
	while (iterations--) {
		pltb_result_t *a = (pltb_result_t*)in;
		pltb_result_t *b = (pltb_result_t*)inout;
		for (unsigned i = 0; i < IC_MAX; i++) {
			if (a->ic[i] < b->ic[i]) {
				b->ic[i] = a->ic[i];
				b->matrix_index[i] = a->matrix_index[i];
			}
		}
	}
}

int init_MPI_Task_type(MPI_Datatype *task_type) {
	static int          block_lengths[2] = { 1, 1 };
	static MPI_Aint     offsets[2]       = { offsetof(pltb_task_t, matrix_index),
	                                         offsetof(pltb_task_t, free_parameter_count)
	                                       };
	static MPI_Datatype member_types[2]  = { MPI_UNSIGNED, MPI_UNSIGNED };
	return MPI_Type_struct(2, block_lengths, offsets, member_types, task_type);
}

int init_MPI_Result_type(MPI_Datatype *result_type) {
	static int          block_lengths[2] = { IC_MAX, IC_MAX };
	static MPI_Aint     offsets[2]       = { offsetof(pltb_result_t, ic),
	                                         offsetof(pltb_result_t, matrix_index)
	                                       };
	static MPI_Datatype member_types[2]  = { MPI_DOUBLE, MPI_UNSIGNED };
	return MPI_Type_struct(2, block_lengths, offsets, member_types, result_type);
}

int init_MPI_Model_stat_type( MPI_Datatype *result_type ) {
	static int block_lengths[5]      = { 1, 1, IC_MAX, 1, 1 };
	static MPI_Aint offsets[5]       = { offsetof(pltb_model_stat_t, matrix_index),
	                                     offsetof(pltb_model_stat_t, likelihood),
	                                     offsetof(pltb_model_stat_t, ic),
	                                     offsetof(pltb_model_stat_t, time_cpu),
	                                     offsetof(pltb_model_stat_t, time_real)
	                                   };
	MPI_Datatype member_types[5]     = { MPI_INT,
	                                     MPI_DOUBLE,
	                                     MPI_DOUBLE,
	                                     MPI_DOUBLE,
	                                     MPI_DOUBLE
	                                   };
	return MPI_Type_struct(5, block_lengths, offsets, member_types, result_type);
}
