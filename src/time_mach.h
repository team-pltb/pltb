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
#ifndef TIME_MACH_H
#define TIME_MACH_H

#include <time.h>
#include <sys/time.h>
#include <mach/clock.h>
#include <mach/mach.h>

typedef struct {
	/* for cpu time measurement */
	clock_t clock_before, clock_after;

	/* for real time measurement */
	clock_serv_t cs_before, cs_after;
	mach_timespec_t mts_before, mts_after;
	struct timespec spec_before, spec_after;
} timer_instance_t;

#define TIME_STRUCT_INIT(var) timer_instance_t var

#define TIME_START(var) var.clock_before = clock(); \
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &var.cs_before); \
	clock_get_time(var.cs_before, &var.mts_before); \
	mach_port_deallocate(mach_task_self(), var.cs_before); \
	var.spec_before.tv_sec = var.mts_before.tv_sec; \
	var.spec_before.tv_nsec = var.mts_before.tv_nsec;
#define TIME_END(var) var.clock_after = clock(); \
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &var.cs_after); \
	clock_get_time(var.cs_after, &var.mts_after); \
	mach_port_deallocate(mach_task_self(), var.cs_after); \
	var.spec_after.tv_sec = var.mts_after.tv_sec; \
	var.spec_after.tv_nsec = var.mts_after.tv_nsec;
#define TIME_CPU(var) ((var.clock_after - var.clock_before) / (double)CLOCKS_PER_SEC)
#define TIME_REAL(var) ((var.spec_after.tv_sec - var.spec_before.tv_sec) \
	+ (var.spec_after.tv_nsec - var.spec_before.tv_nsec) / 1000000000.0)

#endif
