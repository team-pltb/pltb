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
#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG 1

#if DEBUG
    #define DBG(fmt, ...) do { printf(fmt, ##__VA_ARGS__); } while(0)
#else
    #define DBG(fmt, ...)
#endif

#define DEBUG_PROCESS_STATISTICS_NO_OUTPUT 0
#define DEBUG_PROCESS_STATISTICS_TO_STDOUT 1
#define DEBUG_PROCESS_STATISTICS_TO_FILE 2

#define DEBUG_PROCESS_STATISTICS DEBUG_PROCESS_STATISTICS_TO_STDOUT

#endif
