#!/bin/bash

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

BASE_FOLDER="eval/res/histograms"
DATA_FOLDER="$BASE_FOLDER/data"
HIST_FOLDER="$BASE_FOLDER/plots"
PLOT_FILE="eval/rf_histogram.plot"

FORMAT="pdf"
EXT=$FORMAT

mkdir -p $BASE_FOLDER
mkdir -p $HIST_FOLDER

for data in $DATA_FOLDER/*; do
	base=`basename $data`
	echo "Writing $base.$EXT"
	gnuplot -e "src='$data'" -e "trg='$HIST_FOLDER/$base.$EXT'" -e "format='$FORMAT'" $PLOT_FILE
	rm "tmp"
done
