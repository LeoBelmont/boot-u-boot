// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2016~2026 Synaptics Incorporated. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or
 * later as published by the Free Software Foundation.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND
 * SYNAPTICS EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES,
 * INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE, AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY
 * INTELLECTUAL PROPERTY RIGHTS. IN NO EVENT SHALL SYNAPTICS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, PUNITIVE, OR
 * CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED AND
 * BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF
 * COMPETENT JURISDICTION DOES NOT PERMIT THE DISCLAIMER OF DIRECT
 * DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS' TOTAL CUMULATIVE LIABILITY
 * TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S. DOLLARS.
 */

#include <dm.h>
#include <linux/arm-smccc.h>
#include <reset-uclass.h>

enum reset_type {
	SYNA_SMC_RESET,
	SYNA_SMC_ASSERT,
	SYNA_SMC_DEASSERT,
};

#define SYNA_SIP_RESET		0xC200000E

static int syna_smc_reset_assert(struct reset_ctl *rst)
{
	struct arm_smccc_res res;

	arm_smccc_smc(SYNA_SIP_RESET, SYNA_SMC_ASSERT, rst->id, 0,
		      0, 0, 0, 0, &res);

	return 0;
}

static int syna_reset_deassert(struct reset_ctl *rst)
{
	struct arm_smccc_res res;

	arm_smccc_smc(SYNA_SIP_RESET, SYNA_SMC_DEASSERT, rst->id, 0,
		      0, 0, 0, 0, &res);

	return 0;
}

static int syna_smc_reset_probe(struct udevice *dev)
{
	return 0;
}

static const struct udevice_id syna_smc_reset_ids[] = {
	{ .compatible = "syna,smc-reset" },
	{ }
};

struct reset_ops syna_smc_reset_ops = {
	.rst_assert = syna_smc_reset_assert,
	.rst_deassert = syna_reset_deassert,
};

U_BOOT_DRIVER(syna_smc_reset) = {
	.name		= "syna_smc_reset",
	.id		= UCLASS_RESET,
	.of_match	= syna_smc_reset_ids,
	.probe		= syna_smc_reset_probe,
	.ops		= &syna_smc_reset_ops,
};
