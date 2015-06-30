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
#include <stdlib.h>

#include "ic.h"
#include "pltb.h"
#include "models.h"

#include "pltb_frontend.h"

#define PRINT_HLINE(f) do {\
		fprintf(f, "-----------------------------------------------------------------------------------------------------------\n");\
	} while (0)
#define PRINT_HEADER(f) do {\
		fprintf(f, "   Model    |   Time [seconds]    |            |         I N F O R M A T I O N   C R I T E R I A\n");\
		fprintf(f, "------------|---------------------| Max Log_e  |-----------------------------------------------------------\n");\
		fprintf(f, " Symm.  | K |   CPU    |  REAL    | Likelihood |  %s      |  %s   |  %s   |  %s    |  %s \n",\
				get_IC_name_short(AIC), get_IC_name_short(AICc_C), get_IC_name_short(AICc_RC), get_IC_name_short(BIC_C), get_IC_name_short(BIC_RC));\
	} while (0)
#define PRINT_BODY_ROW(f, ...) do {\
		fprintf(f, " %s | %u | %8.3f | %8.3f | %10.8g | %9.8g | %9.8g | %9.8g | %9.8g | %9.8g\n", __VA_ARGS__);\
	} while (0)
#define PRINT_SUMMARY(f, cpu, real, models_array) do {\
		fprintf(f, " Overview   | %8.1f | %8.1f |            ", cpu, real);\
		for (unsigned i = 0; i < IC_MAX; i++) {\
			fprintf(f, "| -> %6s ", models_array[i]);\
		}\
		fprintf(f, "\n");\
	} while (0)
#define PRINT_TREE_SEARCH_HEADER() do {\
		printf("Tree search for best model(s)\n");\
	} while (0)
#define PRINT_TREE_SEARCH_PRETEXT_BEGIN(model) do {\
		printf("# Model %s [newick] (", model);\
	} while (0)
#define PRINT_TREE_SEARCH_PRETEXT_END() do {\
		printf(")\n");\
	} while (0)
#define PRINT_TREE_SEARCH_PRETEXT_IC(name) do {\
		printf("%s", name);\
	} while (0)
#define PRINT_TREE_SEARCH_PRETEXT_IC_SEP(name) do {\
		printf(", %s", name);\
	} while (0)
#define PRINT_TREE(repr) do {\
		printf("%s", repr);\
	} while (0)

static unsigned insert_unique_model(unsigned *models, unsigned len, unsigned model)
{
	for (unsigned i = 0; i < len; i++) {
		if (models[i] == model) {
			return len;
		}
	}
	models[len] = model;
	return len + 1;
}

static unsigned prepare_unique_model_tasks(unsigned *models, unsigned (*IC_models)[IC_MAX],
		unsigned *extra_models, unsigned n_extra_models)
{
	unsigned i = 0;
	for (unsigned j = 0; j < IC_MAX; j++) {
		i = insert_unique_model(models, i, (*IC_models)[j]);
	}
	for (unsigned j = 0; j < n_extra_models; j++) {
		i = insert_unique_model(models, i, extra_models[j]);
	}
	return i;
}

static void make_indices_absolute( model_space_t *model_space, unsigned (*IC_models)[IC_MAX] )
{
	for (unsigned i = 0; i < IC_MAX; i++) {
		(*IC_models)[i] = absolute_model_index(model_space, (*IC_models)[i]);
	}
}

void evaluate_result(model_space_t *relative_model_space, pltb_result_t *result, pllAlignmentData *data, pltb_config_t *config)
{
	pllInstance *tree;

	/* TODO: inplace modifications. ugly! */
	make_indices_absolute(relative_model_space, &result->matrix_index);

	unsigned *models  = malloc(sizeof(unsigned) * (IC_MAX + config->n_extra_models));
	unsigned n_models = prepare_unique_model_tasks(models, &result->matrix_index, config->extra_models, config->n_extra_models);

	model_space_t model_space;
	init_selection_model_space(&model_space, models, n_models);

	PRINT_TREE_SEARCH_HEADER();
	while (next_model(&model_space)) {
		PRINT_TREE_SEARCH_PRETEXT_BEGIN(model_space.matrix_repr_short);
		bool first = true;
		for (unsigned i = 0; i < IC_MAX; i++) {
			if (result->matrix_index[i] == models[model_space.matrix_index]) {
				if (first) {
					PRINT_TREE_SEARCH_PRETEXT_IC(get_IC_name_short(i));
					first = false;
				} else {
					PRINT_TREE_SEARCH_PRETEXT_IC_SEP(get_IC_name_short(i));
				}
			}
		}
		if (first) {
			PRINT_TREE_SEARCH_PRETEXT_IC("extra");
		}
		PRINT_TREE_SEARCH_PRETEXT_END();

		partitionList *parts = init_partitions(data, config->base_freq_kind);
		/* do the actual work */
		tree = setup_instance(model_space.matrix_repr, &config->attr_tree_search, data, parts);
		tree_search(tree, parts);
		prepare_tree_string(tree, parts);
		PRINT_TREE(tree->tree_string);
		pllPartitionsDestroy(tree, &parts);
		pllDestroyInstance(tree);
	}

	destroy_model_space(&model_space);
}

char *get_IC_name_short(IC criterion)
{
	switch (criterion) {
		case AIC:
			return "AIC";
		case AICc_C:
			return "AICc-S";
		case AICc_RC:
			return "AICc-M";
		case BIC_C:
			return "BIC-S";
		case BIC_RC:
			return "BIC-M";
		default:
		case IC_MAX:
			return "";
	}
}

char *get_IC_name_long(IC criterion)
{
	return get_IC_name_short(criterion);
}

void fprint_progress_begin(FILE *f)
{
		setbuf(f, NULL);
		fprintf(f, "/ 0%% ----------------------------------- Evaluating likelihoods ------------------------------------ 100%% \\");
		fprintf(f, "\n\\");
}

unsigned fprint_progress_step(FILE *f, unsigned last, unsigned curr, unsigned max)
{
	unsigned next = (curr * (OUTPUT_WIDTH - 2)) / max;
	for (; last < next; last++) fprintf(f, "#");
	return next;
}

void fprint_progress_end(FILE *f)
{
	fprintf(f, "/\n");
}

void fprint_eval_header(FILE *f)
{
	PRINT_HLINE(f);
	PRINT_HEADER(f);
	PRINT_HLINE(f);
}

void fprint_eval_row(FILE *f, model_space_t *model_space, pltb_model_stat_t *stat)
{
	set_model(model_space, stat->matrix_index);
	PRINT_BODY_ROW(f, model_space->matrix_repr_short, model_space->K,
	        stat->time_cpu, stat->time_real, stat->likelihood,
			stat->ic[AIC], stat->ic[AICc_C], stat->ic[AICc_RC], stat->ic[BIC_C], stat->ic[BIC_RC]);
}

void fprint_eval_summary(FILE *f, model_space_t *model_space, pltb_model_stat_t (*stats)[], pltb_result_t *result)
{
	double overall_time_cpu  = 0.0;
	double overall_time_real = 0.0;
	for (unsigned i = 0; i < model_space->matrix_count; i++) {
		overall_time_cpu += (*stats)[i].time_cpu;
		overall_time_real += (*stats)[i].time_real;
	}
	char chosen_models[IC_MAX][MODEL_MATRIX_REPRESENTATION_LENGTH_SHORT];
	for (unsigned i = 0; i < IC_MAX; i++) {
		set_model(model_space, result->matrix_index[i]);
		memcpy(&chosen_models[i], &model_space->matrix_repr_short, MODEL_MATRIX_REPRESENTATION_LENGTH * sizeof(char));
	}
	PRINT_HLINE(f);
	PRINT_SUMMARY(f, overall_time_cpu, overall_time_real, chosen_models);
	PRINT_HLINE(f);
}
