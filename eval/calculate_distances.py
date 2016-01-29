#!/usr/bin/python3.4

# This file is part of PLTB.
# Copyright (C) 2015 Michael Hoff, Stefan Orf and Benedikt Riehm
#
# PLTB is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# PLTB is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with PLTB.  If not, see <http://www.gnu.org/licenses/>.

from __future__ import print_function

import sys
import os
from subprocess import check_output, CalledProcessError, Popen
from operator import itemgetter, attrgetter, methodcaller
import tempfile
import argparse

from functools import reduce
from itertools import product
import operator

from lib.pltb_data import GTR_MODEL, GTR_SELECTOR_LABEL, Selector, TreeEntry
from lib.pltb_result_parser import parse_trees_from_result_file
from lib.distance_calculator import calc_distances

def parse_trees_and_calc_distances(raxml, result_file):
    trees = parse_trees_from_result_file(result_file)
    distances = calc_distances(raxml, trees)
    return (trees, distances)

### VARIANCE

def calc_mean(values):
    if not values:
        raise Exception("Empty list has no mean")
    return sum(values) / len(values)

def calc_variance(values, ref = None):
    if not values:
        raise Exception("Empty list has no variance")
    if ref == None:
        ref = calc_mean(values)
    return sum(map(lambda v: pow(v - ref, 2), values)) / len(values)

def create_relative_distance_variance_for_file(raxml, result_file):
    (_, distances) = parse_trees_and_calc_distances(raxml, result_file)
    return calc_variance([rel_distance for (_,rel_distance) in distances.values()])

def create_wrapped_relative_distance_variance_for_file(raxml, result_file):
    return (result_file, create_relative_distance_variance_for_file(raxml, result_file))

def create_relative_distance_variances(raxml, results):
    return [create_wrapped_relative_distance_variance_for_file(raxml, f) for f in results]

### MODEL-PAIRWISE DISTANCES

def create_model_pairwise_relative_distances_for_file(raxml, result_file):
    trees, distances = parse_trees_and_calc_distances(raxml, result_file)
    return [(result_file, trees[i].model, trees[j].model, rel_dist) for (i,j), (_,rel_dist) in distances.items()]

# [ (result_file, model_A, model_B, relative_distance), ... ]
# for all result files, all pairwise distances
def create_model_pairwise_relative_distances(raxml, results):
    return reduce(operator.concat, [create_model_pairwise_relative_distances_for_file(raxml, f) for f in results], [])

### IC-PAIRWISE DISTANCES

def create_empty_ic_list_matrix():
    return create_initialized_dict(product(list(Selector), list(Selector)), lambda: [])

def create_initialized_dict(keys, init):
    matrix = dict()
    for k in keys:
        matrix[k] = init()
    return matrix

def create_relative_distance_matrix_for_ics_for_file(raxml, result_file):
    trees, distances = parse_trees_and_calc_distances(raxml, result_file)

    if Selector.GTR not in trees[0].ics:
        trees[0].ics.append(Selector.GTR)

    matrix = create_empty_ic_list_matrix()
    for (i,j), (_, rel_dist) in distances.items():
        for ic1, ic2 in product(trees[i].ics, trees[j].ics):
            matrix[(ic1, ic2)].append(rel_dist)
            # ensure symmetry
            if ic1 != ic2:
                matrix[(ic2, ic1)].append(rel_dist)
    return matrix

def merge_list_matrix(left_matrix, right_matrix):
    matrix = create_empty_ic_list_matrix()
    for key, values in left_matrix.items():
        matrix[key].extend(values)
    for key, values in right_matrix.items():
        matrix[key].extend(values)
    return matrix

def create_relative_distance_matrix_for_ics(raxml, results):
    return reduce(merge_list_matrix, map(lambda f: create_relative_distance_matrix_for_ics_for_file(raxml, f), results), create_empty_ic_list_matrix())


### PRINTING AND FORMATTING FUNCTIONS (API)

def print_sorted_model_pairwise_distances(raxml, results, descending = True):
    # sort the whole list using the third tuple element (the distance)
    all_distances = sorted(create_model_pairwise_relative_distances(raxml, results), key=itemgetter(3), reverse = descending)
    print("{:<80} | {:^15} | {}".format("Result file", "Models", "Relative Distance"))
    for (result_file, modelA, modelB, rel_dist) in all_distances:
        print("{:<80}   {}   {}   {}".format(result_file, modelA, modelB, rel_dist))


def print_sorted_datasetwise_variances(raxml, results, descending = True):
    variances = sorted(create_relative_distance_variances(raxml, results), key=itemgetter(1), reverse = descending)
    print("{:<80} | {}".format("Result file", "Variance"))
    for (result_file, variance) in variances:
        print("{:<80}   {}".format(result_file, variance))

# Make sure the given path exists.
# Creates all folders on the path if required.
def assert_dir(path):
    if not os.path.exists(path):
        os.makedirs(path)

# This function takes the RAxML binary and a list of PLTB result files
# to write files in eval/res/histograms/data for histogram generation.
def write_relative_distances_for_selector_pairs(raxml, results):
    differences = create_relative_distance_matrix_for_ics(raxml, results)
    unique_ic_pairs = [(ic1, ic2) for ic1, ic2 in differences.keys() if ic1 < ic2]

    assert_dir('eval/res/histograms/data')
    print("Writing all distances for each selector combination in a separate data file...")
    for ic1, ic2 in unique_ic_pairs:
        relatives = differences[(ic1, ic2)]
        filename = 'eval/res/histograms/data/%s-%s' % (ic1.serialize(), ic2.serialize())
        write_formatted_items(filename, "%f", relatives, lambda _: print("... {:^6} entries".format(len(relatives))))

def write_formatted_items(filename, formatstr, items, prewrite = None):
    print("Writing {}".format(filename))
    with open(filename, 'w') as target_file:
        if prewrite:
            prewrite(target_file)
        target_file.write("\n".join(map(lambda item: formatstr % item, items)) + "\n")

def write_model_selection_histogram(raxml, results):
    print("Counting each model selection (ignoring GTR if forced by 'extra')")

    models = dict()
    for f in results:
        for tree in parse_trees_from_result_file(f):
            if tree.ics == [Selector.GTR]:
                continue
            if tree.model not in models:
                models[tree.model] = 0
            models[tree.model] += 1

    assert_dir('eval/res/histograms/model_data')
    write_formatted_items('eval/res/histograms/model_data/overall', "%s %d", models.items())

def write_model_selection_histogram_per_ic(raxml, results):
    # ic_models: ic -> (model -> count)
    ic_models = dict()
    for f in results:
        for tree in parse_trees_from_result_file(f):
            if tree.ics == [Selector.GTR]:
                continue
            for ic in tree.ics:
                if ic not in ic_models:
                    ic_models[ic] = dict()
                if tree.model not in ic_models[ic]:
                    ic_models[ic][tree.model] = 0
                ic_models[ic][tree.model] += 1

    assert_dir('eval/res/histograms/model_data')

    for ic, models in ic_models.items():
        write_formatted_items('eval/res/histograms/model_data/{}'.format(ic.serialize()), "%s %d", models.items())

    ics = sorted(ic_models.keys())

    combined = dict()
    for ic, models in ic_models.items():
        for model, count in models.items():
            if model not in combined:
                combined[model] = create_initialized_dict(ics, lambda: 0)
            combined[model][ic] = count

    write_formatted_items('eval/res/histograms/model_data/combined', "%s",
                          [model + " " + " ".join(map(lambda ic: str(ic_counts[ic]), ics)) for model, ic_counts in sorted(combined.items())],
                          lambda f: f.write("Model " + " ".join(map(lambda ic: "\"%s\"" % (ic), ics)) + "\n"))

parser = argparse.ArgumentParser(description='Calculate pairwise RF-distances given a pltb result.')

actions = dict(
    { 'model-pairwise-distances': print_sorted_model_pairwise_distances
     , 'ic-pairwise-distances': write_relative_distances_for_selector_pairs
     , 'model-selection': write_model_selection_histogram
     , 'model-selection-per-ic': write_model_selection_histogram_per_ic
     , 'dataset-variances': print_sorted_datasetwise_variances
     }
)

parser = argparse.ArgumentParser(description='Calculate pairwise RF-distances given a pltb result.')
parser.add_argument('action', choices=actions, help='the operation to conduct')
parser.add_argument('results', type=str, nargs='+', help='the pltb results')
parser.add_argument('--raxml', dest='raxml', default='raxmlHPC-SSE3', help='RAxML binary for RF-distance calculation. default: raxmlHPC-SSE3')

args = parser.parse_args()

actions[args.action](args.raxml, args.results)

exit(0)
