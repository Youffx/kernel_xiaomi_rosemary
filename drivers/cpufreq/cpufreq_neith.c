// SPDX-License-Identifier: GPL-2.0-only
/*
 * NEITH CPUFreq Governor
 *
 * Hybrid CPU frequency scaling governor combining:
 * - Conservative step-based scaling
 * - Scheduler-driven utilization (PELT)
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
 * ------------------------------------------------------------------
 * Overview:
 *
 * NEITH is a hybrid CPUFreq governor designed for mobile devices and
 * daily usage workloads. It merges the stability of the conservative
 * governor with the responsiveness of scheduler utilization signals
 * (Per-Entity Load Tracking / PELT).
 *
 * Design Goals:
 * - Smooth and predictable frequency transitions
 * - Fast reaction to sudden workload spikes
 * - Reduced UI stutter and frame drops
 * - Balanced power efficiency and performance
 *
 * Behavior:
 * - High sustained load:
 * → Gradual step-based frequency increase (conservative style)
 *
 * - Sudden utilization spike (PELT-driven):
 * → Immediate "emergency jump" toward target frequency
 *
 * - Low / decreasing load:
 * → Controlled and gradual frequency reduction
 *
 * - Anti-oscillation:
 * → Avoids aggressive up/down frequency bouncing
 *
 * Frequency Target Model:
 * f_target = (util + util/4) * f_max / 1024
 *
 * Notes:
 * - util is derived from PELT (scheduler utilization signal)
 * - The "+ util/4" term provides a mild boost (~1.25x) to improve
 * responsiveness without excessive power cost
 *
 * Author: Youffx <asa.jazal@gmail.com>
 * Created: May 1, 2026
 * ------------------------------------------------------------------
 */

#include <linux/slab.h>
#include <linux/sched/cpufreq.h>
#include <linux/sched.h>
#include "cpufreq_governor.h"
#include "cpufreq_neith.h"

struct neith_policy {
	struct policy_dbs_info policy_dbs;
	unsigned int requested_freq;
};

static inline struct neith_policy *to_neith(struct policy_dbs_info *p)
{
	return container_of(p, struct neith_policy, policy_dbs);
}

static unsigned long neith_get_util(int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	unsigned long util = cpu_util_cfs(rq);
	unsigned long max = arch_scale_cpu_capacity(NULL, cpu);

	if (!max)
		return 0;

	return (util * 1024) / max;
}

static unsigned int neith_update(struct cpufreq_policy *policy)
{
	struct policy_dbs_info *p = policy->governor_data;
	struct neith_policy *dbs = to_neith(p);
	struct dbs_data *dbs_data = p->dbs_data;
	struct neith_tuners *tuners = dbs_data->tuners;

	unsigned int load;
	unsigned int step;
	unsigned int cur;
	unsigned int next;
	unsigned long util;
	u64 target_pelt;

	load = dbs_update(policy);

	step = (tuners->freq_step * policy->cpuinfo.max_freq) / 100;
	if (step == 0)
		step = policy->cpuinfo.min_freq;

	cur = policy->cur;
	next = cur;

	util = neith_get_util(policy->cpu);

	target_pelt = (u64)policy->cpuinfo.max_freq * (util + (util >> 2));
	target_pelt >>= 10;
	
	if (target_pelt > policy->max)
		target_pelt = policy->max;

	if (load > dbs_data->up_threshold) {
		if (tuners->pelt_boost_enable && target_pelt > (cur + step)) {
			next = (unsigned int)target_pelt;
		} else {
			next = cur + step;
		}
	} else if (load < tuners->down_threshold) {
		if (cur > policy->min + step)
			next = cur - step;
		else
			next = policy->min;
	}

	if (next > policy->max)
		next = policy->max;

	if (next < policy->min)
		next = policy->min;

	if (next != cur) {
		__cpufreq_driver_target(policy, next,
			next > cur ? CPUFREQ_RELATION_H : CPUFREQ_RELATION_L);
		dbs->requested_freq = next;
	}

	return dbs_data->sampling_rate;
}

static ssize_t show_down_threshold(struct gov_attr_set *attr_set, char *buf)
{
	struct dbs_data *dbs = to_dbs_data(attr_set);
	struct neith_tuners *tuners = dbs->tuners;

	return sprintf(buf, "%u\n", tuners->down_threshold);
}

static ssize_t store_down_threshold(struct gov_attr_set *attr_set, const char *buf, size_t count)
{
	struct dbs_data *dbs = to_dbs_data(attr_set);
	struct neith_tuners *tuners = dbs->tuners;
	unsigned int val;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tuners->down_threshold = val;
	return count;
}

static ssize_t show_pelt_boost(struct gov_attr_set *attr_set, char *buf)
{
	struct dbs_data *dbs = to_dbs_data(attr_set);
	struct neith_tuners *tuners = dbs->tuners;

	return sprintf(buf, "%u\n", tuners->pelt_boost_enable);
}

static ssize_t store_pelt_boost(struct gov_attr_set *attr_set, const char *buf, size_t count)
{
	struct dbs_data *dbs = to_dbs_data(attr_set);
	struct neith_tuners *tuners = dbs->tuners;
	unsigned int val;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tuners->pelt_boost_enable = val;
	return count;
}

static ssize_t show_freq_step(struct gov_attr_set *attr_set, char *buf)
{
	struct dbs_data *dbs = to_dbs_data(attr_set);
	struct neith_tuners *tuners = dbs->tuners;

	return sprintf(buf, "%u\n", tuners->freq_step);
}

static ssize_t store_freq_step(struct gov_attr_set *attr_set, const char *buf, size_t count)
{
	struct dbs_data *dbs = to_dbs_data(attr_set);
	struct neith_tuners *tuners = dbs->tuners;
	unsigned int val;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tuners->freq_step = val;
	return count;
}

gov_show_one_common(up_threshold);
gov_show_one_common(sampling_rate);

gov_attr_rw(up_threshold);
gov_attr_rw(sampling_rate);
gov_attr_rw(down_threshold);
gov_attr_rw(pelt_boost);
gov_attr_rw(freq_step);

static struct attribute *neith_attributes[] = {
	&up_threshold.attr,
	&sampling_rate.attr,
	&down_threshold.attr,
	&pelt_boost.attr,
	&freq_step.attr,
	NULL
};

static struct policy_dbs_info *neith_alloc(void)
{
	struct neith_policy *p;

	p = kzalloc(sizeof(*p), GFP_KERNEL);
	return p ? &p->policy_dbs : NULL;
}

static void neith_free(struct policy_dbs_info *policy)
{
	kfree(to_neith(policy));
}

static int neith_init(struct dbs_data *dbs_data)
{
	struct neith_tuners *tuners;

	tuners = kzalloc(sizeof(*tuners), GFP_KERNEL);
	if (!tuners)
		return -ENOMEM;

	tuners->up_threshold = NEITH_DEF_UP_THRESHOLD;
	tuners->down_threshold = NEITH_DEF_DOWN_THRESHOLD;
	tuners->freq_step = NEITH_DEF_FREQ_STEP;
	tuners->pelt_boost_enable = 1;

	dbs_data->tuners = tuners;
	dbs_data->up_threshold = tuners->up_threshold;

	return 0;
}

static void neith_exit(struct dbs_data *dbs_data)
{
	kfree(dbs_data->tuners);
}

static void neith_start(struct cpufreq_policy *policy)
{
	struct neith_policy *p = to_neith(policy->governor_data);

	p->requested_freq = policy->cur;
}

static struct dbs_governor neith_gov = {
	.gov = CPUFREQ_DBS_GOVERNOR_INITIALIZER("neith"),
	.kobj_type = { .default_attrs = neith_attributes },
	.gov_dbs_update = neith_update,
	.alloc = neith_alloc,
	.free = neith_free,
	.init = neith_init,
	.exit = neith_exit,
	.start = neith_start,
};

static int __init neith_init_call(void)
{
	return cpufreq_register_governor(&neith_gov.gov);
}

static void __exit neith_exit_call(void)
{
	cpufreq_unregister_governor(&neith_gov.gov);
}

MODULE_AUTHOR("Youffx <asa.jazal@gmail.com>");
MODULE_DESCRIPTION("NEITH Hybrid CPUFreq Governor");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_NEITH
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return &neith_gov.gov;
}

fs_initcall(neith_init_call);
#else
module_init(neith_init_call);
module_exit(neith_exit_call);
#endif
