#!/usr/bin/python3.4
from lib.pltb_data import GTR_MODEL, Selector, TreeEntry
from itertools import dropwhile
import re

class ParseError(Exception):
    pass

# This function takes a file(name) containing the results of a PLTB run.
# A list of TreeEntries is returned.
#
# Each TreeEntry t contains:
# t.model: MODEL_STRING, e.g. "012345" (GTR)
# t.ics: [IC1, IC2, IC3, ...], e.g. ["AIC C", "BIC C", ...]
# t.newick_tree: ENCODED_TREE, the newick tree for this model
# The second parameter contains all ICs that chose this model string.
#
# The following invariant is assured (if requires_gtr is set):
# A TreeEntry with model GTR exists and becomes head of the returned list
def parse_trees_from_result_file(pltb_result_file, requires_gtr = True):
    with open(pltb_result_file) as source:
        trees = []
        source_lines = list(dropwhile(lambda l: not re.match('^Tree search.*$', l), source))
        if not source_lines:
            raise ParseError("Inconsistent result file " + pltb_result_file + ". No tree searches have been conducted.")
        for (head, tree) in zip(source_lines[1::2], map(lambda t: t.rstrip(), source_lines[2::2])):
            result = re.match('^# Model ([0-5]{6}) \[newick\] \(([a-zA-Z,\s-]*)\)$', head);
            model = result.group(1) # e.g. '012345'
            ics = list(map(Selector, result.group(2).split(', '))) # e.g. ['AIC']
            trees.append(TreeEntry(model, ics, tree))

    if not trees:
        raise ParseError("No tree found in " + pltb_result_file)

    if requires_gtr:
        gtrEntry = next(t for t in trees if t.model == GTR_MODEL);

        if not gtrEntry:
            raise ParseError("No tree found for GTR in " + pltb_result_file)

        trees.remove(gtrEntry)
        trees.insert(0, gtrEntry)

    return trees
