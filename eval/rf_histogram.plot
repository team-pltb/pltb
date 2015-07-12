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

clear
reset
set key off

if (!exists("src")) src='extra-aic'
if (!exists("trg")) trg='profit.svg'
if (!exists("format")) format='svg'

bin_width = 1.0;
bin_number(x) = bin_width * floor(x/bin_width)

set table 'tmp'
plot src using (bin_number($1*100)):(1) smooth frequency with boxes
unset table

set xrange [-1:60]
set logscale y

set xlabel 'relative RF distance [%]'
set ylabel '# occurences [%]' offset 2

set style fill solid 1.0 noborder

set term format
# set term png size 1500,600
set output trg

set grid ytics lc rgb "#bbbbbb" lw 1 lt 0
set grid xtics lc rgb "#bbbbbb" lw 1 lt 0

plot 'tmp' u ($1):(100*$2/1476):(0.5) w boxes # , \
#     ''    u ($1):(100*$2/336*1.5):($2) with labels
