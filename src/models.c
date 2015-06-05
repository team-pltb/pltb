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
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "models.h"

#define MAX_FREE_PARAMETER_COUNT 6
#define MAX_MATRIX_INDEX 203

const unsigned model_index_GTR = 202;

static unsigned rate_matrix_count[] = {1, 31, 90, 65, 15, 1};

static char* rate_matrices[] = {
/* K = 1 */
"000000",
/* K = 2 */
"011111","010000","001000","000100","000010",
"000001","001111","010111","011011","011101",
"011110","011000","010100","010010","010001",
"001100","001010","001001","000110","000101",
"000011","000111","001011","001101","001110",
"010011","010101","010110","011001","011010",
"011100",
/* K = 3 */
"012222","012111","011211","011121","011112",
"012000","010200","010020","010002","001200",
"001020","001002","000120","000102","000012",
"011222","012122","012212","012221","012211",
"012121","012112","011221","011212","011122",
"010222","012022","012202","012220","001222",
"001211","001121","001112","012011","012101",
"012110","010211","010121","010112","011201",
"011210","011021","011012","011120","011102",
"012200","012020","012002","010220","010202",
"010022","012100","012010","012001","011200",
"011020","011002","010210","010201","010120",
"010102","010021","010012","001220","001202",
"001022","001210","001201","001120","001102",
"001021","001012","000122","000121","000112",
"001122","001212","001221","010122","010212",
"010221","011022","011202","011220","012012",
"012021","012102","012120","012201","012210",
/* K = 4 */
"012333","012322","012232","012223","012311",
"012131","012113","011231","011213","011123",
"012300","012030","012003","010230","010203",
"010023","001230","001203","001023","000123",
"012233","012323","012332","012133","012313",
"012331","011233","011232","011223","012312",
"012321","012132","012123","012231","012213",
"012033","012303","012330","010233","010232",
"010223","012302","012320","012032","012023",
"012230","012203","001233","001232","001223",
"001231","001213","001123","012301","012310",
"012031","012013","012130","012103","010231",
"010213","010123","011230","011203","011023",
/* K = 5 */
"012344","012343","012334","012342","012324",
"012234","012341","012314","012134","011234",
"012340","012304","012034","010234","001234",
/* K = 6 */
"012345"
};

static unsigned index_offset( void *context, unsigned idx )
{
	return idx + *((unsigned*)context);
}

static unsigned index_select( void *context, unsigned idx )
{
	return ((unsigned*)context)[idx];
}

static void init_matrix_repr( char *repr, unsigned length )
{
	assert(length % 2 == 0);
	for (unsigned i = 1; i < length; i += 2) {
		repr[i] = ',';
	}
	repr[length - 1] = '\0';
}

static void init_matrix_repr_short( char *repr, unsigned length )
{
	repr[length - 1] = '\0';
}

static void format_matrix_repr( char *repr, char *digits, unsigned length )
{
	for (unsigned i = 0; i < length; i++) {
		repr[i * 2] = digits[i];
	}
}

static void format_matrix_repr_short( char *repr, char *digits, unsigned length )
{
	for (unsigned i = 0; i < length; i++) {
		repr[i] = digits[i];
	}
}

static unsigned default_index_translator( void *context, unsigned idx )
{
	(void)context; // unused.
	return idx;
}

static unsigned translate_index_to_K( unsigned index )
{
	for (unsigned K = 0; K < MAX_FREE_PARAMETER_COUNT; K++) {
		if (index < rate_matrix_count[K]) {
			return K + 1;
		}
		index -= rate_matrix_count[K];
	}
	/* TODO invalid index encountered */
	return 0;
}

void init_default_model_space( model_space_t *model_space )
{
	/* default index translator: identity */
	init_model_space(model_space, MAX_MATRIX_INDEX, NULL, &default_index_translator);
}

void init_range_model_space( model_space_t *model_space, unsigned lower, unsigned upper )
{
	// TODO: where do we clear the memory?
	unsigned *context = malloc(sizeof(unsigned));
	*context = lower;
	init_model_space(model_space, (unsigned)(upper - lower), context, &index_offset);
}

void init_selection_model_space( model_space_t *model_space, unsigned *indices, unsigned count )
{
	unsigned *indices_copy = malloc(sizeof(unsigned) * count);
	memcpy(indices_copy, indices, count * sizeof(unsigned));
	init_model_space(model_space, count, indices_copy, &index_select);
}

void init_model_space( model_space_t *model_space, unsigned count, void *context, index_translator *func )
{
	model_space->index_func = func;
	model_space->translator_context = context;

	model_space->matrix_count = count;
	model_space->matrix_index = ((unsigned)0) - 1;
	init_matrix_repr(model_space->matrix_repr, MODEL_MATRIX_REPRESENTATION_LENGTH);
	init_matrix_repr_short(model_space->matrix_repr_short, MODEL_MATRIX_REPRESENTATION_LENGTH_SHORT);
}

bool is_model_valid( model_space_t *model_space )
{
    return model_space->matrix_index < model_space->matrix_count;
}

unsigned absolute_model_index( model_space_t *model_space, unsigned index )
{
	return model_space->index_func(model_space->translator_context, index);
}

bool set_model( model_space_t *model_space, unsigned index )
{
	model_space->matrix_index = index;
	unsigned translated_index = absolute_model_index(model_space, index);
	model_space->K = translate_index_to_K(translated_index);
	model_space->free_parameter_count = model_space->K - 1;
	if (!is_model_valid(model_space)) {
		return false;
	}
	format_matrix_repr(model_space->matrix_repr, rate_matrices[translated_index],
			MAX_FREE_PARAMETER_COUNT);
	format_matrix_repr_short(model_space->matrix_repr_short, rate_matrices[translated_index],
			MAX_FREE_PARAMETER_COUNT);
	return true;
}

bool next_model( model_space_t *model_space )
{
	return set_model(model_space, model_space->matrix_index + 1);
}

void destroy_model_space( model_space_t *model_space )
{
	model_space->index_func = NULL;
	if (model_space->translator_context != NULL) {
		free(model_space->translator_context);
		model_space->translator_context = NULL;
	}
	model_space->matrix_count = 0;
}
