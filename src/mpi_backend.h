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
#ifndef MPI_BACKEND_H
#define MPI_BACKEND_H

#include <mpi.h>

#include "pltb.h"

typedef struct {
    unsigned matrix_index;
    unsigned free_parameter_count;
} pltb_task_t;

void result_reduce( void*, void*, int*, MPI_Datatype* );

int init_MPI_Task_type( MPI_Datatype* );
int init_MPI_Result_type( MPI_Datatype* );
int init_MPI_Model_stat_type( MPI_Datatype* result_type );

#endif
