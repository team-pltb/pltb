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
#include <pll/pll.h>
//include <pll/mem_alloc.h>
#include <pll/queue.h>
#include <pll/parsePartition.h>
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>

#include "pltb.h"

void configure_attr_defaults( pltb_config_t *config )
{
	config->attr_model_eval.rateHetModel     = PLL_GAMMA;
	config->attr_model_eval.fastScaling      = PLL_FALSE;
	config->attr_model_eval.saveMemory       = PLL_FALSE;
	config->attr_model_eval.useRecom         = PLL_FALSE;
	config->attr_model_eval.randomNumberSeed = 0x12345;
	config->attr_model_eval.numberOfThreads  = 1;

	config->attr_tree_search.rateHetModel     = PLL_GAMMA;
	config->attr_tree_search.fastScaling      = PLL_FALSE;
	config->attr_tree_search.saveMemory       = PLL_FALSE;
	config->attr_tree_search.useRecom         = PLL_FALSE;
	config->attr_tree_search.randomNumberSeed = 0x12345;
	config->attr_tree_search.numberOfThreads  = 1;
}

pllInstance *init_instance( pllInstanceAttr *attr )
{
	return pllCreateInstance(attr);
}

pllAlignmentData *read_alignment_data( char *dataset_file )
{
	assert(access(dataset_file, R_OK ) != -1);
	return pllParseAlignmentFile(PLL_FORMAT_PHYLIP, dataset_file);
}

partitionList *init_partitions( pllAlignmentData *data, pltb_base_freq_t base_freq_kind ) {
	//  DNAX => optimize base frequencies
	//  DNA  => empirical base frequencies
	char *prefix = NULL;
	switch (base_freq_kind) {
		default:
			return NULL;
		case EMPIRICAL:
			prefix = "DNA";
			break;
		case OPTIMIZED:
			prefix = "DNAX";
			break;
		case EQUAL:
			/* use empirical for now */
			prefix = "DNA";
			break;
	}
	char *conf = malloc(sizeof(char) * (unsigned long) snprintf(NULL, 0, "%s, p = 1 - %d",
	                                            prefix, data->sequenceLength) + 1);
	sprintf(conf, "%s, p = 1 - %d", prefix, data->sequenceLength);
	pllQueue *queue = pllPartitionParseString(conf);

	assert(pllPartitionsValidate(queue, data));

	partitionList *parts = pllPartitionsCommit(queue, data);

	if (base_freq_kind == EQUAL) {
		double equal_frequencies[] = {0.25, 0.25, 0.25, 0.25};
		memcpy(parts->partitionData[0]->frequencies, &equal_frequencies, sizeof(double) * 4);
		parts->dirty = PLL_TRUE;
	}

	pllQueuePartitionsDestroy(&queue);
	return parts;
}

void prepare_tree_string( pllInstance *inst, partitionList *parts ) {
	pllTreeToNewick(inst->tree_string, inst, parts, inst->start->back, PLL_TRUE, PLL_FALSE, 0, 0, 0, PLL_SUMMARIZE_LH, 0,0);
}

pllInstance *setup_instance( char *matrix, pllInstanceAttr *attr, pllAlignmentData *alignment_data, partitionList *parts )
{
	pllInstance *inst = init_instance(attr);
	assert(inst != NULL);
	pllTreeInitTopologyForAlignment(inst, alignment_data);
	pllLoadAlignment(inst, alignment_data, parts);
	pllComputeRandomizedStepwiseAdditionParsimonyTree(inst, parts);
	pllInitModel(inst, parts);
	pllSetSubstitutionRateMatrixSymmetries(matrix, parts, 0);
	return inst;
}

void calculate_model_ICs(pltb_model_stat_t *stat, pllAlignmentData* data, pllInstance* inst,
		unsigned model_param_count, pltb_config_t* config)
{
	unsigned n_branches = data->sequenceCount * 2 - 3;
	switch (config->base_freq_kind) {
		default:
			/* TODO error? */
		case EMPIRICAL:
		case EQUAL:
			calculate_ICs(&stat->ic[0], data, inst, model_param_count + 1 + n_branches);
			break;
		case OPTIMIZED:
			calculate_ICs(&stat->ic[0], data, inst, model_param_count + 4 + n_branches);
			break;
	}
}

void merge_into_result( pltb_result_t *result, pltb_model_stat_t *stat, unsigned index )
{
	for (unsigned i = 0; i < IC_MAX; i++) {
		if (stat->ic[i] < result->ic[i]) {
			result->ic[i] = stat->ic[i];
			result->matrix_index[i] = index;
		}
	}
}

void tree_search( pllInstance *inst, partitionList *parts )
{
	pllRaxmlSearchAlgorithm(inst, parts, PLL_TRUE);
}

void optimize_model_parameters( pllInstance *inst, partitionList *parts )
{
	pllOptimizeModelParameters(inst, parts, inst->likelihoodEpsilon);
}
