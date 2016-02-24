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

# Script for processing whole dataset folders with pltb.
# $1: folder with datasets (without trailing slash), e.g. eval/res/datasets/testDatasets
# $2: folder where the results shall be stored in
# Naming pattern: $1/somename -> $2/somename-0xSEED[-opt].result (SEED is replaced by e.g. 12345)

function process {
	# expect $1: pltb
	# expect $2: dataset
	# expect $3: result base path
	hex_seeds=( "12345" "54321" "00000" "11111" "22222" "33333" "44444" "55555" "66666" "77777" "88888" "99999" "AAAAA" "BBBBB" "CCCCC" "DDDDD" "EEEEE" "FFFFF" );

	# configuration for the cluster we used
	processes=13
	threads_per_process=4
	threads_for_search=47

	for seed in ${hex_seeds[@]}; do
		mpi_part="mpirun -np $processes"
		pltb_param="-f $2 -n $threads_per_process -s $threads_for_search -r 0x$seed -g"
		pltb_part="$1 $pltb_param"
		cmd="$mpi_part $pltb_part > $3-0x$seed.result"
		echo "$cmd"
		eval $cmd
		cmd="$mpi_part $pltb_part -b > $3-0x$seed-opt.result"
		echo "$cmd"
		eval $cmd
	done
}

pltb="./pltb.out"

if [[ -f "$pltb" ]]; then
	datFolder=$1
	resFolder=$2
	for datFile in $datFolder/*; do
		datName=$(basename "$datFile")
		echo "Processing $datFile -> $resFolder/$dataName";
		process $pltb $datFile "$resFolder/$datName";
	done;
else
	echo "Error: Binary $pltb does not exist."
fi
