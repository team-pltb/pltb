#!/usr/bin/python

# This file is part of PLTB.
# Copyright (C) 2015 Michael Hoff, Stefan Orf and Benedikt Riehm
#
# PLTB is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# PLTB is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with PLTB.  If not, see <http://www.gnu.org/licenses/>.

from __future__ import print_function

import sys
import re
import os
import operator
from subprocess import check_output, CalledProcessError, Popen
from operator import itemgetter, attrgetter, methodcaller
import tempfile
import argparse

# This function takes a file(name) containing the results of a PLTB run.
# A list of tuples of the following form is returned:
# [(MODEL_STRING, [IC1, IC2, IC3, ...], ENCODED_TREE), ...]
#
# Each tuple contains...
# 0: MODEL_STRING, e.g. "012345" (GTR)
# 1: [IC1, IC2, IC3, ...], e.g. ["AIC C", "BIC C", ...]
# 2: ENCODED_TREE, the newick tree for this model
# The second parameter contains all ICs that chose this model string.
#
# Thus, this returned list is basically the parsed "tree search" output of PLL.
#
# The following invariant is assured:
# If a tuple with the GTR model exists, it is the first tuple of the returned list.
def parse_trees(pltb_result_file):
    # start with an empty list of tuples
    trees=[]

    # read file line by line
    with open(pltb_result_file) as source:
        line_iter = iter(source);
        try:
            while True:
                # search for "Tree search..." to start parsing trees
                line = line_iter.next();
                if (re.match('^Tree search.*$', line) != None):
                    break;
        except StopIteration:
            # last line reached and never found a tree
            print("Error: No trees found.");
            exit(1);
        # "Tree search..." line found. start parsing trees
        try:
            while True:
                line = line_iter.next();
                result = re.match('^# Model ([0-5]{6}) \[newick\] \(([a-zA-Z ,]*)\)$', line);
                if (result != None and result.group(1) != None and result.group(2) != None):
                    try:
                        # add a tuple, note the line_iter.next(), which takes the next line and saves it as the newick tree
                        # thus always two lines are consumed here.
                        trees.append((result.group(1), result.group(2).split(', '), line_iter.next()));
                    except StopIteration:
                        print("Error: " + result.group(1) + " has no tree.");
                        exit(2);
        except StopIteration:
            pass # fine

    # move GTR to front of list (if contained)
    for i in range(len(trees)):
        if (trees[i][0] == "012345"): # GTR
            trees.insert(0, trees.pop(i));
            break;

    if (len(trees) == 0):
        print("Error: No tree found or GTR not contained.");
        exit(1);
    if (trees[0][0] != "012345"):
        print("Warning: GTR not included.");

    return trees;

# This function takes a list of parsed pltb tuples (see parse_trees) and the RAxML binary
# to calculate the RF distances (using RAxML) and to then return the exact output of RAxML.
def calculate_distances(raxml, trees):
    # work in temporary directory (e.g. /tmp/pltb-eval-134hba2/)
    tmpdir = tempfile.mkdtemp(prefix = 'pltb-eval-');

    # RAxML requires all tree to be provided line-by-line in a single text-file.

    # create that temporary (tree-)file
    tmp_tree_file_path = os.path.join(tmpdir, "tree_file");

    # write trees to file line-by-line
    with open(tmp_tree_file_path, 'w') as tmp_tree_file:
        # iterate over the given tuples, extracting and writing only the last element (the newick tree string) for each entry
        for (_, _, tree) in trees:
            # writes single line (line feed is inserted automatically)
            tmp_tree_file.write(tree);

    try:
        # call raxml: -w - working directory (our temporary dir), -n (obligatory run-) suffix, ...
        # for the other arguments please refer to the RAxML documentation
        check_output(["%s -m GTRGAMMAX -f r -z %s -n run -w %s" % (raxml, tmp_tree_file_path, tmpdir)], shell=True);
        # RAxML outputs two files in the temporary folder: RAxML_info.run and RAxML_RF-Distances.run
    except CalledProcessError as e:
        print("RAxML failed.");
        print(e.output);
        exit(1);
    else:
        # call successful

        # read distances file
        distance_file_path = os.path.join(tmpdir, 'RAxML_RF-Distances.run');
        with open(distance_file_path) as distance_file:
            # distance_result will contain the exact output of RAxML regarding the RF-distance calculation
            distance_result = distance_file.read();
        try:
            # remove distances file
            os.remove(distance_file_path);
        except IOError as e:
            print("Error: Unable to remove temporary file " + distance_file_path);
            exit(1);
        finally:
            # remove info file
            os.remove(os.path.join(tmpdir, 'RAxML_info.run'));
        # all RAxML files removed
    finally:
        # remove our tree file
        os.remove(tmp_tree_file_path)
        # remove the temporary directory
        os.rmdir(tmpdir);

    return distance_result;

# This function takes the exact output of a RAxML RF-distance calculation run (see calculate_distances)
# and returns a map with the following structure:
# (index1, index2) -> (absolute_distance, relative_distance)
#
# This map has to be interpreted in the context of a pltb result list (see parse_trees).
# In such a list, each index represents a tree.
# With the map returned by this function each index-combination, thus every tree-combination,
# gets assigned a relative and an absolute distance value (as calculated by RAxML)
#
# Arbitrary example:
# trees = [ ("000000", ["AIC"], "..."), ("012345", ["BIC"], "...") ]
# distances[(0,1)] = (10.0, 0.15)
# Thus, the tree for "000000" and the tree for "012345" differ by 15% and an absolute amount of 10.
def parse_distances(distance_result):
    # iterator for the lines in distance_result
    line_iter = iter(distance_result.split('\n'));
    # start with an empty map
    distances = dict();
    try:
        while True:
            line = line_iter.next();
            result = re.match('^([0-9]) ([0-9]): ([0-9]+) ([0-9\.]+)$', line);
            if (result == None):
                # if the match fails, we are done.
                break;
            # insert the parsed ids and distance values in the map: (id1, id2) -> (absolute, relative)
            distances[(int(result.group(1)), int(result.group(2)))] = (float(result.group(3)), float(result.group(4)));
    except StopIteration:
        pass # fine
    return distances;

# This function takes the RAxML binary, an list of (PLTB) result files
# and prints a sorted list over all found relative distances.
# Each printed distance value is associated with the corresponding result file
# it came from and the two models which's comparison lead to this value.
def print_sorted_distances(raxml, results):
    # this list will contain all distances in the following tuple format:
    # [ (pltb_result_file, model_A, model_B, relative_distance), ... ]
    all_distances = []
    for f in results:
        print("Processing %s" % (f));
        # read the trees from the pltb result file
        trees = parse_trees(f);
        # calculate and parse the distances
        distances = parse_distances(calculate_distances(raxml, trees));
        # for each pairwise-distance between two ids (the ids refer to the trees-list)
        for ((id1, id2), (absolute, relative)) in distances.iteritems():
            # insert an entry in the format described above
            # note the lookup in the trees list, first with the index
            # and then with 0, which means the first tuple entry (hence, the model string. see parse_trees)
            all_distances.append((f, trees[id1][0], trees[id2][0], relative));
    # sort the whole list using the third tuple element (the distance)
    all_distances = sorted(all_distances, key=itemgetter(3))
    for d in all_distances:
        print("{:<66}   {}   {}   {}".format(*d));
    print("{:<66} | {:^15} | {}".format("Result file", "Models", "Relative Distance"));
    print("Validation: We processed %d distances" % (len(all_distances)))

# This function takes a dictionary (a map), two information criterium labels and a (distance) value.
# The dictionary will now map the tuple containing both labels to a list containing various distances.
#
# The tuple (the key) containing both labels is first normalized using canonical ordering.
# Except that, if ic1 or ic2 is 'extra' it has to be the second key entry.
# This is important, as the inserted distance value has commutative semantics.
#
# The list for the values is created lazily on demand.
def insert_lazy(d, (ic1, ic2), value):
    if ((ic1 >= ic2) and (ic1 != 'extra')) or (ic2 == 'extra'):
        key = (ic2, ic1);
    else:
        key = (ic1, ic2);
    if key in d:
        d[key].append(value);
    else:
        d[key] = [value];

# Prepare the given IC label for printing.
# Current semantics: Delete whitespaces and convert to lowercase.
def serialize_ic(ic):
    return ic.replace(' ', '').lower();

# Make sure the given path exists.
# Creates all folders on the path if required.
def assert_dir(path):
    if not os.path.exists(path):
        os.makedirs(path);

# This function takes the RAxML binary and a list of PLTB result files
# to write files in eval/res/histograms/data for histogram generation.
def print_hist_ic(raxml, results):
    print("Processing %d results" % (len(results)));
    # empty map which will contain mappings of the following format:
    # (ic1, ic2) -> [distance_value1, distance_value2, ...]
    differences=dict()
    for f in results:
        # take a single result file f

        # parse the contained trees
        trees = parse_trees(f);

        # virtually insert a 'extra' label if GTR was chosen by another model also.
        # thus, a tree labeled 'extra' always exists.
        if 'extra' not in trees[0][1]:
            # it is assumed here, that the first tree is always corresponding to GTR.
            trees[0][1].append('extra');

        # for all IC labels which chose one model together insert the distance 0.0 in the map
        for tree in trees:
            for key in [(ic1, ic2) for ic1 in tree[1] for ic2 in tree[1] if ic1 < ic2]:
                insert_lazy(differences, key, 0.0);

        # now calculate and parse the distances regarding the given tree-list
        distances = parse_distances(calculate_distances(raxml, trees));
        # for each mapping
        for ((id1, id2), (absolute, relative)) in distances.iteritems():
            # for each IC label for the tree of the first index
            for ic1 in trees[id1][1]:
                # for each IC label for the tree of the second index
                for ic2 in trees[id2][1]:
                    # insert the distance into the map for both ICs
                    insert_lazy(differences, (ic1, ic2), relative);
    assert_dir('eval/res/histograms/data');

    # write each map entry to a file
    for ((ic1, ic2), relatives) in sorted(differences.iteritems()):
        print("# %s vs %s: %d" % (ic1, ic2, len(relatives)));
        with open('eval/res/histograms/data/%s-%s' % (serialize_ic(ic1), serialize_ic(ic2)), 'w') as target_file:
            target_file.write("\n".join(map(str, relatives)) + "\n");

def print_hist_model(raxml, results):
    print("Processing %d results" % (len(results)));
    print("Counting models chosen by several ICs per dataset only once");
    models=dict()
    for f in results:
        # take a single result file f

        # parse the contained trees
        trees = parse_trees(f);
        for tree in trees:
            model = tree[0]
            if model == "012345" and "extra" in tree[1]:
                continue
            if model not in models:
                models[model] = 0
            models[model] += 1

    assert_dir('eval/res/histograms/model_data');

    # write each map entry to a file
    with open('eval/res/histograms/model_data/overall', 'w') as target_file:
        target_file.write("\n".join(map(lambda entry: "%s %d" % entry, models.iteritems())) + "\n");

def print_hist_model_per_ic(raxml, results):
    print("Processing %d results" % (len(results)));

    ic_models=dict()
    for f in results:

        trees = parse_trees(f);
        for tree in trees:
            model = tree[0]
            for ic in tree[1]:
                if model == "012345" and ic == "extra":
                    continue
                if ic not in ic_models:
                    ic_models[ic] = dict()
                if model not in ic_models[ic]:
                    ic_models[ic][model] = 0
                ic_models[ic][model] += 1

    assert_dir('eval/res/histograms/model_data');

    for (ic, models) in ic_models.iteritems():
        with open('eval/res/histograms/model_data/%s' % (serialize_ic(ic)), 'w') as target_file:
            target_file.write("\n".join(map(lambda entry: "%s %d" % entry, models.iteritems())) + "\n");


parser = argparse.ArgumentParser(description='Calculate pairwise RF-distances given a pltb result.')

actions=dict({'print-sorted': print_sorted_distances, 'hist-ic-ic': print_hist_ic, 'hist-model': print_hist_model, 'hist-model-per-ic': print_hist_model_per_ic})

parser = argparse.ArgumentParser(description='Calculate pairwise RF-distances given a pltb result.')
parser.add_argument('action', choices=actions, help='the operation to conduct');
parser.add_argument('results', type=str, nargs='+', help='the pltb results')
parser.add_argument('--raxml', dest='raxml', default='raxmlHPC-SSE3', help='RAxML binary for RF-distance calculation. default: raxmlHPC-SSE3')

args = parser.parse_args();

actions[args.action](args.raxml, args.results);

exit(0);
