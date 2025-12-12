// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2016~2024 Synaptics Incorporated. All rights reserved.
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

#include "includes.h"
#include "dsih_core.h"
#include "DPHYTX_release.h"

void mipi_dphy_BiuCtrlPHYEn(dphy_t *phy, int en)
{
	u32 ctrl;

	ioread32(phy->base + R_DPHYTX_DPHY_CTL0, &ctrl);
	MIPI_FIELD_SET(ctrl, MIPI_DPHY_BIUCTRLPHYEN, (en & 0x1));
	iowrite32(phy->base + R_DPHYTX_DPHY_CTL0, ctrl);
}

void mipi_dphy_shutdown(dphy_t *phy, int shutdown)
{
	u32 ctrl;

	ioread32(phy->base + R_DPHYTX_DPHY_CTL1, &ctrl);
	MIPI_FIELD_SET(ctrl, MIPI_DPHY_SHUTDOWNZ, (shutdown & 0x1));
	iowrite32(phy->base + R_DPHYTX_DPHY_CTL1, ctrl);
}

void mipi_dphy_resetz(dphy_t *phy, int resetz)
{
	u32 ctrl;

	ioread32(phy->base + R_DPHYTX_DPHY_CTL1, &ctrl);
	MIPI_FIELD_SET(ctrl, MIPI_DPHY_RSTZ, (resetz & 0x1));
	iowrite32(phy->base + R_DPHYTX_DPHY_CTL1, ctrl);
}

void mipi_dphy_enable_lanes(dphy_t *phy, int lanes)
{
	u32 ctrl;

	ioread32(phy->base + R_DPHYTX_DPHY_CTL1, &ctrl);

	ctrl &= MIPI_FIELD_CLR_MASK(MIPI_DPHY_CTL_ENABLE);

	MIPI_FIELD_SET(ctrl, MIPI_DPHY_CTL_ENABLE, (lanes & 0xF));

	iowrite32(phy->base + R_DPHYTX_DPHY_CTL1, ctrl);
}

void mipi_dphy_EnableClkBIU(dphy_t *phy, int en)
{
	u32 ctrl;

	ioread32(phy->base + R_DPHYTX_DPHY_CTL1, &ctrl);
	MIPI_FIELD_SET(ctrl, MIPI_DPHY_ENABLECLK_BIU, (en & 0x1));
	iowrite32(phy->base + R_DPHYTX_DPHY_CTL1, ctrl);
}

void mipi_dphy_CfgClkFreqRange(dphy_t *phy, int range)
{
	u32 ctrl;

	ioread32(phy->base + R_DPHYTX_DPHY_CTL1, &ctrl);
	MIPI_FIELD_SET(ctrl, MIPI_DPHY_ENABLECLK_FREQUENCY_RANGE, (range & 0x3F));
	iowrite32(phy->base + R_DPHYTX_DPHY_CTL1, ctrl);
}

void mipi_dphy_pll_shadow_control_en(dphy_t *phy, int en)
{
	u32 ctrl;

	ioread32(phy->base + R_DPHYTX_DPHY_PLL2, &ctrl);
	MIPI_FIELD_SET(ctrl, MIPI_DPHY_PLL2_PLL_SHADOW_CONTROL, (en & 0x1));
	iowrite32(phy->base + R_DPHYTX_DPHY_PLL2, ctrl);
}

void mipi_dphy_pll_clksel(dphy_t *phy, int clksel)
{
	u32 ctrl;

	ioread32(phy->base + R_DPHYTX_DPHY_PLL2, &ctrl);
	MIPI_FIELD_SET(ctrl, MIPI_PLL2_CLKSEL, (clksel & 0x3));
	iowrite32(phy->base + R_DPHYTX_DPHY_PLL2, ctrl);
}

void mipi_dphy_stopstate_wait(dphy_t *phy, int lanes)
{
	u32 rb0, cond;
	unsigned int wait = 0xF00;

	lanes = (1 << lanes) - 1;
	cond = 0;
	MIPI_FIELD_SET(cond, MIPI_DPHY_RB0_STOPSTATECLK, 1);
	MIPI_FIELD_SET(cond, MIPI_RB0_STOPSTATEDATA, (lanes & 0xF));

	do {
		ioread32(phy->base + R_DPHYTX_DPHY_RB0, &rb0);
		rb0 = (rb0 & 0x3D000);
		if (!wait)
			break;
		wait = wait - 1;
	} while (rb0 != cond);
}
