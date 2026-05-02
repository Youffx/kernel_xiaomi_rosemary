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
 * Description:
 * NEITH is a hybrid CPUFreq governor designed for balanced daily usage
 * on mobile devices. It blends the smooth and stable frequency scaling
 * characteristics of the conservative governor with the responsiveness
 * of scheduler-driven utilization signals (Per-Entity Load Tracking / PELT).
 *
 * The goal is to achieve:
 * - Efficient power usage during light workloads
 * - Fast ramp-up under sudden load
 * - Reduced frequency oscillation
 * - Better real-world responsiveness compared to traditional governors
 *
 * Author: Youffx <asa.jazal@gmail.com>
 * Date: May 1, 2026
 */

#ifndef _CPUFREQ_NEITH_H
#define _CPUFREQ_NEITH_H

#include <linux/cpufreq.h>

#define NEITH_DEF_UP_THRESHOLD      80
#define NEITH_DEF_DOWN_THRESHOLD    40
#define NEITH_DEF_FREQ_STEP         5

struct neith_tuners {
	unsigned int up_threshold;
	unsigned int down_threshold;
	unsigned int freq_step;
	unsigned int pelt_boost_enable;
};

#endif
