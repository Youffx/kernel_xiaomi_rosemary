// SPDX-License-Identifier: GPL-2.0-only
/*
 * NEITH CPUFreq Governor
 *
 * Hybrid CPU frequency scaling governor:
 * Conservative-based policy combined with scheduler utilization (PELT).
 *
 * Copyright (C) 2026 Youffx <asa.jazal@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Youffx <asa.jazal@gmail.com>
 * Date: May 1, 2026
 */

#ifndef _CPUFREQ_NEITH_H
#define _CPUFREQ_NEITH_H

#include <linux/cpufreq.h>

#define NEITH_DEF_UP_THRESHOLD		82
#define NEITH_DEF_DOWN_THRESHOLD	38

#define NEITH_DEF_FREQ_STEP		5

#define NEITH_UTIL_HIGH		850
#define NEITH_UTIL_MID		650
#define NEITH_UTIL_LOW		200

#define NEITH_UP_RATE_LIMIT_US		50000
#define NEITH_DOWN_RATE_LIMIT_US	20000

#define NEITH_HYSTERESIS_PERCENT	2

struct neith_tuners {
	unsigned int up_threshold;
	unsigned int down_threshold;
	unsigned int freq_step;
	unsigned int pelt_boost_enable;
};

#endif
