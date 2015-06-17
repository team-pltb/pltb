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
#ifndef PLTB_H
#define PLTB_H

#include <stdbool.h>
#include <pll/pll.h>

#include "ic.h"

typedef enum {
	/* fixed empirical values (set by pll) */
	EMPIRICAL,
	/* 1/4 each */
	EQUAL,
	/* implies 3 free parameters (4th frequency = sum - 1) */
	OPTIMIZED
} pltb_base_freq_t;

typedef struct {
	unsigned matrix_index[IC_MAX];
	double   ic[IC_MAX];
} pltb_result_t;

typedef struct {
	double likelihood;
	double ic[IC_MAX];
	double time_cpu;
	double time_real;
	unsigned matrix_index;
} pltb_model_stat_t;

typedef struct {
	/* implies a free parameter count of 3 */
	unsigned *extra_models;
	pllInstanceAttr attr_model_eval;
	pllInstanceAttr attr_tree_search;
	pltb_base_freq_t base_freq_kind;
	unsigned n_extra_models;
} pltb_config_t;

void configure_attr_defaults( pltb_config_t *config );

/**
 * Creates and initializes the pllInstance we work on.
 * Don't forget to destroy it after use.
 * @return A pllInstance with predefined attributes or NULL iff operation failed
 */
pllInstance *init_instance( pllInstanceAttr *attr );

/**
 * Retrieve the MSA (pllAlignmentData) from a file in PHYLIP format. Don't forget to destroy the data after use.
 * @param dataset_file Filename of the file to read & parse
 * @return A pllAlignmentData structure containing the parsed MSA
 */
pllAlignmentData *read_alignment_data( char *dataset_file );

/**
 * Create a partition configuration with one single partition containing all sequences. Don't forget to destroy the list after use.
 * @param data The MSA to be partitioned
 * @return A partitionList containing all partitions of the MSA
 * @todo discuss configuration of pllPartitionInfo
 */
partitionList *init_partitions( pllAlignmentData *data, pltb_base_freq_t base_freq_kind );

/**
 * Prints out the tree topology
 * @param inst the pllInstance containing the tree topology
 * @param parts the partitionList containing the partitions
 */
void prepare_tree_string( pllInstance *inst, partitionList *parts );

void calculate_model_ICs( pltb_model_stat_t *stat, pllAlignmentData*, pllInstance*, unsigned, pltb_config_t* );

void merge_into_result( pltb_result_t *local_result, pltb_model_stat_t *stat, unsigned index );

void tree_search( pllInstance *inst, partitionList *parts );

pllInstance *setup_instance( char *matrix, pllInstanceAttr *attr, pllAlignmentData *alignment_data, partitionList *parts );

void optimize_model_parameters( pllInstance *inst, partitionList *parts );

#endif
