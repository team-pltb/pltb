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


def parse_trees(pltb_result_file):
    trees=[]

    # Read given output of pltb, yields e.g.:
    # trees = [("012345", ["AIC C", "BIC C", ...], TREE), ...]
    with open(pltb_result_file) as source:
        line_iter = iter(source);
        try:
            while True:
                line = line_iter.next();
                if (re.match('^Tree search.*$', line) != None):
                    break;
        except StopIteration:
            print("Error: No trees found.");
            exit(1);
        try:
            while True:
                line = line_iter.next();
                result = re.match('^# Model ([0-5]{6}) \[newick\] \(([a-zA-Z ,]*)\)$', line);
                if (result != None and result.group(1) != None and result.group(2) != None):
                    try:
                        trees.append((result.group(1), result.group(2).split(', '), line_iter.next()));
                    except StopIteration:
                        print("Error: " + result.group(1) + " has no tree.");
                        exit(2);
        except StopIteration:
            pass # fine

    # move GTR to front
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

def calculate_distances(raxml, trees):
    # work in temporary directory
    tmpdir = tempfile.mkdtemp(prefix = 'pltb-eval-');

    # plain textfile for trees to be analyzed
    tmp_tree_file_path = os.path.join(tmpdir, "tree_file");

    # write trees to file
    with open(tmp_tree_file_path, 'w') as tmp_tree_file:
        for (_, _, tree) in trees:
            tmp_tree_file.write(tree);

    try:
        # call raxml: -w - working directory, -n suffix, ...
        check_output(["%s -m GTRGAMMAX -f r -z %s -n run -w %s" % (raxml, tmp_tree_file_path, tmpdir)], shell=True);
        # RAxML_info.run and RAxML_RF-Distances.run should have been written
    except CalledProcessError as e:
        print("RAxML failed.");
        print(e.output);
        exit(1);
    else:
        # read distances file
        distance_file_path = os.path.join(tmpdir, 'RAxML_RF-Distances.run');
        with open(distance_file_path) as distance_file:
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

def parse_distances(distance_result):
    line_iter = iter(distance_result.split('\n'));
    distances = dict();
    try:
        while True:
            line = line_iter.next();
            result = re.match('^([0-9]) ([0-9]): ([0-9]+) ([0-9\.]+)$', line);
            if (result == None):
                break;
            distances[(int(result.group(1)), int(result.group(2)))] = (float(result.group(3)), float(result.group(4)));
    except StopIteration:
        pass # fine
    return distances;


def print_distances(raxml, results):
    for f in results:
        trees = parse_trees(f);
        print("Models: " + ', '.join(map(lambda (m,cs,_): m + " (" + ','.join(cs) + ")", trees)));
        if (len(trees) > 1):
            print(calculate_distances(raxml, trees), end="");
        elif (len(trees) == 1):
            print("Single tree, nothing to be done.");

def print_sorted_distances(raxml, results):
    all_distances = []
    for f in results:
        print("Processing %s" % (f));
        trees = parse_trees(f);
        distances = parse_distances(calculate_distances(raxml, trees));
        for ((id1, id2), (absolute, relative)) in distances.iteritems():
            all_distances.append((f, trees[id1][0], trees[id2][0], relative));
    all_distances = sorted(all_distances, key=itemgetter(3))
    for d in all_distances:
        print("{:<66}   {}   {}   {}".format(*d));
    print("{:<66} | {:^15} | {}".format("Result file", "Models", "Relative Distance"));
    print("Validation: We processed %d distances" % (len(all_distances)))

def insert_lazy(d, (ic1, ic2), value):
    if (ic1 <= ic2):
        key = (ic1, ic2);
    else:
        key = (ic2, ic1);
    if key in d:
        d[key].append(value);
    else:
        d[key] = [value];

def serialize_ic(ic):
    return ic.replace(' ', '').lower();

def assert_dir(path):
    if not os.path.exists(path):
        os.makedirs(path);

def print_hist(raxml, results):
    print("Processing %d results" % (len(results)));
    differences=dict()
    for f in results:
        trees = parse_trees(f);
        if 'extra' not in trees[0][1]:
            trees[0][1].append('extra');
        for tree in trees:
            for key in [(ic1, ic2) for ic1 in tree[1] for ic2 in tree[1] if ic1 < ic2]:
                insert_lazy(differences, key, 0.0);
        distances = parse_distances(calculate_distances(raxml, trees));
        for ((id1, id2), (absolute, relative)) in distances.iteritems():
            for ic1 in trees[id1][1]:
                for ic2 in trees[id2][1]:
                    insert_lazy(differences, (ic1, ic2), relative);
    assert_dir('eval/res/histograms/data');
    for ((ic1, ic2), relatives) in sorted(differences.iteritems()):
        print("# %s vs %s: %d" % (ic1, ic2, len(relatives)));
        with open('eval/res/histograms/data/%s-%s' % (serialize_ic(ic1), serialize_ic(ic2)), 'w') as target_file:
            target_file.write("\n".join(map(str, relatives)) + "\n");

parser = argparse.ArgumentParser(description='Calculate pairwise RF-distances given a pltb result.')
parser.add_argument('results', type=str, nargs='+',
                           help='the pltb results')
# TODO use choices for 'action': setting both the following arguments may have unintended effects
parser.add_argument('--sort', dest='action', action='store_const', const=print_sorted_distances, default=print_distances, help='print the sorted output over all distances')
parser.add_argument('--hist', dest='action', action='store_const', const=print_hist, help='generate histogram data in eval/res/histograms/data')
parser.add_argument('--raxml', dest='raxml', default='raxmlHPC-SSE3', help='RAxML binary for RF-distance calculation. default: raxmlHPC-SSE3')

args = parser.parse_args();

args.action(args.raxml, args.results);


exit(0);

