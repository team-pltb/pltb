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
#ifndef MODELS_H
#define MODELS_H

#include <stdbool.h>

#define MODEL_MATRIX_REPRESENTATION_LENGTH 12
#define MODEL_MATRIX_REPRESENTATION_LENGTH_SHORT 7

typedef unsigned (index_translator)( void *context, unsigned idx );

typedef struct {
	char matrix_repr[MODEL_MATRIX_REPRESENTATION_LENGTH];
	char matrix_repr_short[MODEL_MATRIX_REPRESENTATION_LENGTH_SHORT];
	unsigned free_parameter_count;
	unsigned K;
	unsigned matrix_index;
	unsigned matrix_count;
	void *translator_context;
	index_translator *index_func;
} model_space_t;

extern const unsigned model_index_GTR;

void init_default_model_space( model_space_t *model_space );

void init_range_model_space( model_space_t *model_space, unsigned lower, unsigned upper );

void init_selection_model_space( model_space_t *model_space, unsigned *indices, unsigned count );

void init_model_space( model_space_t *model_space, unsigned count, void *context, index_translator *func );

bool is_model_valid( model_space_t *model_space );

unsigned absolute_model_index( model_space_t *model_space, unsigned index );

bool set_model( model_space_t *model_space, unsigned index );

bool next_model( model_space_t *model_space );

void destroy_model_space( model_space_t *model_space );

#endif
