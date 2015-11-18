import sys
import os
import operator
import re
from subprocess import check_output, CalledProcessError, Popen
from operator import itemgetter, attrgetter, methodcaller
import tempfile

from lib.pltb_data import TreeEntry

# This function takes a list of parsed pltb tuples and the RAxML binary
# to calculate the RF distances (using RAxML) and to then return the exact output of RAxML.
def calculate_distances_internal(raxml, trees):
    # work in temporary directory (e.g. /tmp/pltb-eval-134hba2/)
    tmpdir = tempfile.mkdtemp(prefix = 'pltb-eval-');

    # RAxML requires all tree to be provided line-by-line in a single text-file.

    # create that temporary (tree-)file
    tmp_tree_file_path = os.path.join(tmpdir, "tree_file");

    # write trees to file line-by-line
    with open(tmp_tree_file_path, 'w') as tmp_tree_file:
        # iterate over the given tuples, extracting and writing only the last element (the newick tree string) for each entry
        for entry in trees:
            # writes single line (line feed is inserted automatically)
            tmp_tree_file.write(entry.newick_tree);

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

# This function takes the exact output of a RAxML RF-distance calculation run (see calculate_distances_with_raxml)
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
            line = next(line_iter);
            result = re.match('^([0-9]) ([0-9]): ([0-9]+) ([0-9\.]+)$', line);
            if (result == None):
                # if the match fails, we are done.
                break;
            # insert the parsed ids and distance values in the map: (id1, id2) -> (absolute, relative)
            distances[(int(result.group(1)), int(result.group(2)))] = (float(result.group(3)), float(result.group(4)));
    except StopIteration:
        pass # fine
    return distances;

def calc_distances(raxml, trees):
    if len(trees) > 1:
        return parse_distances(calculate_distances_internal(raxml, trees))
    elif trees:
        return {(0,0): (0,0)}
    else:
        raise Exception("Empty tree list given")
