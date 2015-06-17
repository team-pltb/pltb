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
#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include <pll/pll.h>
#include <stdlib.h>
#include <assert.h>
#include <float.h>
#include "mpi_backend.h"
#include "pltb_frontend.h"

#include "masterworker.h"

#ifdef __APPLE__
#include "time_mach.h"
#else
#include "time.h"
#endif

#define DEBUG_PROCESS_STATISTICS_OPEN_OUTPUT stdout
#define DEBUG_PROCESS_STATISTICS_CLOSE_OUTPUT(f)
//#define DEBUG_PROCESS_STATISTICS_OPEN_OUTPUT fopen("performance.txt", "w")
//#define DEBUG_PROCESS_STATISTICS_CLOSE_OUTPUT(f) fclose(f)

#define TASK_TAG 0
#define DONE_TAG 1
#define STOP_TAG 2

static MPI_Datatype mpi_task_type;
static MPI_Datatype mpi_result_type;
static MPI_Datatype mpi_model_stat_type;
static MPI_Op mpi_result_reduce_op;

static void master(int process_id, int n_workers,
		MPI_Comm root_comm, MPI_Comm inter_comm,
		pllAlignmentData *data, pltb_config_t *config,
		model_space_t *model_space, bool print_progress)
{
	FILE *out = DEBUG_PROCESS_STATISTICS_OPEN_OUTPUT;

	MPI_Status  status;

	MPI_Request requests [n_workers]; /* request handler */
	pltb_task_t tasks    [n_workers]; /* send buffer */

	pltb_model_stat_t stats[model_space->matrix_count];

	int send_index = 0;

	DBG_MASTER("Master[%d]: Issuing initial workload...\n", process_id);

	while (next_model(model_space)) {
		/* setup task */
		tasks[send_index].matrix_index         = model_space->matrix_index;
		tasks[send_index].free_parameter_count = model_space->free_parameter_count;

		DBG_MASTER("Master[%d] -> Worker[%02u]: Matrix #%03u with K = %u\n",
		           process_id, send_index + 1, model_space->matrix_index,
		           model_space->free_parameter_count);

		/* send task */
		MPI_Isend(&tasks[send_index], 1, mpi_task_type, send_index + 1,
		          TASK_TAG, root_comm, &requests[send_index]);

		/* break when all workers received exactly one task */
		if (++send_index == n_workers) break;
	}

	DBG_MASTER("Master[%d]: Switching to on demand work distribution...\n", process_id);

	unsigned finish_ctr = 0;
	unsigned progress   = 0;
	if (print_progress) { fprint_progress_begin(out); }

	while (next_model(model_space)) {

		/* wait for free send slot */
		MPI_Waitany(n_workers, requests, &send_index, MPI_STATUS_IGNORE);
		if (send_index == MPI_UNDEFINED) {
			fprintf(stderr, "Master[%d]: Internal error occured.\n", process_id);
			exit(1);
		}

		/* wait for worker to finish its task */
		pltb_model_stat_t stat;
		/* response contains task-specific evaluation information */
		MPI_Recv(&stat, 1, mpi_model_stat_type, MPI_ANY_SOURCE,
		         DONE_TAG, root_comm, &status);
		if (print_progress) { progress = fprint_progress_step(out, progress, ++finish_ctr, model_space->matrix_count); }
		stats[stat.matrix_index] = stat;

		/* setup new task */
		tasks[send_index].matrix_index         = model_space->matrix_index;
		tasks[send_index].free_parameter_count = model_space->free_parameter_count;

		DBG_MASTER("Master[%d] -> Worker[%02u]: Matrix #%03u with K = %u\n",
		           process_id, status.MPI_SOURCE,
		           model_space->matrix_index, model_space->free_parameter_count);

		/* send new task */
		MPI_Isend(&tasks[send_index], 1, mpi_task_type, status.MPI_SOURCE,
		          TASK_TAG, root_comm, &requests[send_index]);
	}

	DBG_MASTER("Master[%d]: Distribution complete. Sending shutdown signals...\n", process_id);

	int iterations = n_workers;
	while (iterations--) {

		/* wait for free send slot */
		MPI_Waitany(n_workers, requests, &send_index, MPI_STATUS_IGNORE);
		if (send_index == MPI_UNDEFINED) {
			fprintf(stderr, "Master[%d]: Internal error occured.\n", process_id);
			exit(1);
		}

		/* wait for worker to finish its task */
		pltb_model_stat_t stat;
		/* response contains task-specific evaluation information */
		MPI_Recv(&stat, 1, mpi_model_stat_type, MPI_ANY_SOURCE,
		         DONE_TAG, root_comm, &status);
		if (print_progress) { progress = fprint_progress_step(out, progress, ++finish_ctr, model_space->matrix_count); }
		stats[stat.matrix_index] = stat;

		DBG_MASTER("Master[%d] -> Worker[%02d]: Switch to reduction mode!\n", process_id, status.MPI_SOURCE);
		/* issue transfer of result to master (per reduce) */
		MPI_Send(NULL, 0, mpi_task_type, status.MPI_SOURCE,
		         STOP_TAG, root_comm);
	}
	if (print_progress) { fprint_progress_end(out); }
	DBG_MASTER("Master[%d]: Waiting for all workers to finish their work and fold their results...\n", process_id);

	pltb_result_t result;

	/* * * *
	 * Use the intercommunicator between the master communicator and the worker
	 * communicator to isse a result reduce operation. This collective operation
	 * will reduce all workers results (their local maxima) to an aggregated result
	 * (global maximum) which is received by only the root process.
	 * * * */
	MPI_Reduce(NULL, &result, n_workers, mpi_result_type,
	           mpi_result_reduce_op, MPI_ROOT, inter_comm);

	/* all workers will die now */


	fprint_eval_header(out);
	for (unsigned i = 0; i < model_space->matrix_count; i++) {
		fprint_eval_row(out, model_space, &stats[i]);
	}
	fprint_eval_summary(out, model_space, &stats, &result);
	DEBUG_PROCESS_STATISTICS_CLOSE_OUTPUT(out);

	evaluate_result(model_space, &result, data, config);
}

static void worker(int process_id, int master_id,
			MPI_Comm root_comm, MPI_Comm inter_comm,
			pllAlignmentData *data, pltb_config_t *config,
			model_space_t *model_space)
{
	MPI_Status status;

	pltb_result_t    result;
	pltb_task_t      task;

	for (unsigned i = 0; i < IC_MAX; i++) {
		result.ic[i] = FLT_MAX;
	}

	TIME_STRUCT_INIT(timer);
	pltb_model_stat_t stat;

	while (true) {
		/* receive task (or STOP command) from master */
		MPI_Recv(&task, 1, mpi_task_type, master_id, MPI_ANY_TAG, root_comm, &status);

		if (status.MPI_TAG == STOP_TAG) break;

		assert(status.MPI_TAG == TASK_TAG);
		DBG_WORKER("Worker[%02d]: Received order to process matrix #%u\n",
					process_id, task.matrix_index);

		set_model(model_space, task.matrix_index);

		partitionList *parts = init_partitions(data, config->base_freq_kind);

		pllInstance *inst = setup_instance(model_space->matrix_repr, &config->attr_model_eval, data, parts);

		/* initiate time measuring */
		stat.matrix_index = task.matrix_index;
		TIME_START(timer);

		/* the time intensive work.. */
		optimize_model_parameters(inst, parts);

		/* measure and store time */
		TIME_END(timer);
		stat.time_cpu  = TIME_CPU(timer);
		stat.time_real = TIME_REAL(timer);

		stat.likelihood = inst->likelihood;
		calculate_model_ICs(&stat, data, inst, model_space->free_parameter_count, config);
		merge_into_result(&result, &stat, model_space->matrix_index);

		/* clean up */
		pllPartitionsDestroy(inst, &parts);
		pllDestroyInstance(inst);

		/* reply with DONE tag and the meta information */
		MPI_Send(&stat, 1, mpi_model_stat_type, master_id, DONE_TAG, root_comm);
	}

	DBG_WORKER("Worker[%02d]: Stop signal received. Proceeding with reduction process...\n", process_id);

	MPI_Reduce(&result, NULL, 1, mpi_result_type, mpi_result_reduce_op, master_id, inter_comm);

	DBG_WORKER("Worker[%02d]: Result transmitted to reduction process. Exiting.\n", process_id);
}

/**
 * The core function of pltb.
 * @param dataset_file the file containing the sequences
 */
int run_master_worker(int process_id, MPI_Comm root_comm, char *dataset_file,
		pltb_config_t *config, model_space_t *model_space, bool print_progress)
{
	assert(strcmp(dataset_file, "") != 0);

	const int master_id = 0;

	int n_processes;
	int n_workers;

	MPI_Comm_size(root_comm, &n_processes);

	if (n_processes < 2) {
		/* we are alone in this world */
		printf("Single process detected. Quitting.\n");
		return 1;
	}

	if (n_processes - (int)model_space->matrix_count > 1) {
		if (process_id == master_id) {
			printf("Too many processes attached. Quitting.\n");
		}
		return 1;
	}

	n_workers = n_processes - 1;

	MPI_Comm local_comm;
	MPI_Comm inter_comm;

	/* master: local_leader = 0, remote_leader = 1
	 * worker: local_leader = 1, remote_leader = 0
	 */
	int local_leader  = master_id == process_id ? 0 : 1;
	int remote_leader = 1 - local_leader;

	/* split communicator by local_leader (as color, @see doc).
	 * local_leader = 0 => master communicator (one process only)
	 * local_leader = 1 => worker communicator (all other processes)
	 * allocating operation => free local_comm after use
	 */
	MPI_Comm_split(root_comm, local_leader, 0, &local_comm);

	/* intercommunicator between local_comm and remote_group
	 * using the given root_comm as peer communicator.
	 * allocating operation => free inter_comm after use
	 */
	MPI_Intercomm_create(local_comm, 0, root_comm, remote_leader, 0, &inter_comm);

	/* construct MPI meta types.
	 * allocating operation => free types after use */
	init_MPI_Task_type(&mpi_task_type);
	init_MPI_Result_type(&mpi_result_type);

	/* tell MPI about the newly created types.
	 * TODO is there a way to 'uncommit' these types? */
	MPI_Type_commit(&mpi_task_type);
	MPI_Type_commit(&mpi_result_type);

	init_MPI_Model_stat_type(&mpi_model_stat_type);
	MPI_Type_commit(&mpi_model_stat_type);

	/* create operation wrapper.
	 * allocating operation => free op after use */
	MPI_Op_create(result_reduce, true, &mpi_result_reduce_op);

	pllAlignmentData *data  = read_alignment_data(dataset_file);

	if (process_id == master_id) {
		// master
		master(process_id, n_workers, root_comm, inter_comm, data, config, model_space, print_progress);
	} else {
		// worker
		worker(process_id, master_id, root_comm, inter_comm, data, config, model_space);
	}

	pllAlignmentDataDestroy(data);

	MPI_Type_free(&mpi_task_type);
	MPI_Type_free(&mpi_result_type);
	MPI_Comm_free(&local_comm);
	MPI_Comm_free(&inter_comm);
	MPI_Op_free(&mpi_result_reduce_op);
	return 0;
}
