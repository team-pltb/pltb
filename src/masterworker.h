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
#include <stdbool.h>
#include <mpi.h>

#include "debug.h"
#include "pltb.h"
#include "models.h"

/* 0     -> no debug messages from workers
 * n > 0 -> workers with process_id in 1..n will report
 */
#if DEBUG
    #define DEBUG_WORKER 0
#endif
#if DEBUG_WORKER
	#define DBG_WORKER(fmt, p, ...) do { if (p <= DEBUG_WORKER) DBG(fmt, p, ##__VA_ARGS__); } while(0)
#else
	#define DBG_WORKER(fmt, p, ...)
#endif

/* 0     -> no debug messages from master
 * n > 0 -> debug messages from master
 */
#define DEBUG_MASTER 0
#if DEBUG_MASTER
	#define DBG_MASTER(fmt, p, ...) do { DBG(fmt, p, ##__VA_ARGS__); } while(0)
#else
	#define DBG_MASTER(fmt, p, ...)
#endif

int run_master_worker( int process_id, MPI_Comm root_comm, char *dataset_file, pltb_config_t *config, model_space_t *model_space, bool print_progress );
