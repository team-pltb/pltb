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
stats "performance.txt" using 2
show variables all

#width=(STATS_max-STATS_min)/STATS_records

#hist(x,width)=width*floor(x/width)+width/2.0
#set boxwidth width*0.9

#plot "performance.txt" u (hist($2,width)):(1.0) smooth freq w boxes lc rgb"green" notitle

unset key
set xrange [0:STATS_records]
set yrange [-0.1:STATS_max]
set multiplot

#set parametric
#set trange [0:5]
#plot 000,t w l lc rgb"red" lw 3 notitle
#plot 001,t w l lc rgb"red" lw 3
#plot 032,t w l lc rgb"red" lw 3

#unset parametric

#set style histogram gap 4
set xtics rotate by -45
set xtics font "Times-Roman, 10"
set border linewidth 2.5
set xtics ("K=1" 0,"K=2" 1,"K=3" 31,"K=4" 186,"K=5" 201, "K=6" 202)

f(x) = a*x + b

fit f(x) "performance.txt" u 1:2 via a, b

plot "performance.txt" smooth freq w boxes fs solid lc rgb"blue" notitle
plot f(x)  w l lc rgb"red" notitle
