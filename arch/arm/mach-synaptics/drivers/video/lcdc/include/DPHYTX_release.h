/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2016~2025 Synaptics Incorporated. All rights reserved.
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

#ifndef DPHYTX_RELEASE_H
#define DPHYTX_RELEASE_H

#include "ctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _DOCC_H_BITOPS_
#define _DOCC_H_BITOPS_ () {}

#define _bSETMASK_(b)				\
({						\
	typeof(b) __b = (b);			\
	(__b < 32) ? (1 << (__b & 31)) : 0;	\
})

/* N-bit set mask from lsb to msb */
#define _NSETMASK_(msb, lsb)\
({							\
	typeof(msb) __msb = (msb);			\
	typeof(lsb) __lsb = (lsb);			\
	_bSETMASK_(__msb + 1) - _bSETMASK_(__lsb);	\
})

/* Single-bit clear mask */
#define _bCLRMASK_(b)				\
({						\
	typeof(b) __b = (b);			\
	~_bSETMASK_(__b);			\
})

/* N-bit clear mask */
#define _NCLRMASK_(msb, lsb)			\
({						\
	typeof(msb) __msb = (msb);		\
	typeof(lsb) __lsb = (lsb);		\
	~_NSETMASK_(__msb, __lsb);		\
})

/* Bitfield get */
#define _BFGET_(r, msb, lsb)			\
({						\
	typeof(r)   __r   = (r);		\
	typeof(msb) __msb = (msb);		\
	typeof(lsb) __lsb = (lsb);		\
	(_NSETMASK_(__msb - __lsb, 0) &		\
	 (__r >> __lsb));			\
})

/* Bitfield set */
#define _BFSET_(r, msb, lsb, v)			\
do {						\
	typeof(r)   __r   = (r);		\
	typeof(msb) __msb = (msb);		\
	typeof(lsb) __lsb = (lsb);		\
	typeof(v)   __v   = (v);		\
	(__r) &= _NCLRMASK_(__msb, __lsb);	\
	(__r) |= _NSETMASK_(__msb, __lsb) &	\
		(__v << __lsb);			\
} while (0)
#endif

#define R_DPHYTX_DPHY_CTL0                     (0x0000)
#define R_DPHYTX_DPHY_CTL1                     (0x0004)
#define R_DPHYTX_DPHY_CTL2                     (0x0008)
#define R_DPHYTX_DPHY_CTL3                     (0x000C)
#define R_DPHYTX_DPHY_CTL4                     (0x0010)
#define R_DPHYTX_DPHY_CTL5                     (0x0014)
#define R_DPHYTX_DPHY_CTL6                     (0x0018)
#define R_DPHYTX_DPHY_CTL7                     (0x001C)
#define R_DPHYTX_DPHY_CTL8                     (0x0020)
#define R_DPHYTX_DPHY_RB0                      (0x0024)
#define R_DPHYTX_DPHY_RB1                      (0x0028)
#define R_DPHYTX_DPHY_RB2                      (0x002C)
#define R_DPHYTX_DPHY_RB3                      (0x0030)
#define R_DPHYTX_DPHY_PLL0                     (0x0034)
#define R_DPHYTX_DPHY_PLL1                     (0x0038)
#define R_DPHYTX_DPHY_PLL2                     (0x003C)
#define R_DPHYTX_DPHY_PLLRB0                   (0x0040)
#define R_DPHYTX_DPHY_PLLRB1                   (0x0044)

//uDPHY_CTL0_BiuCtrlPhyEn
#define MIPI_DPHY_BIUCTRLPHYEN 0
#define MIPI_DPHY_BIUCTRLPHYEN_WIDTH 0

//uDPHY_CTL1_shutdownz
#define MIPI_DPHY_SHUTDOWNZ 0
#define MIPI_DPHY_SHUTDOWNZ_WIDTH 0

//uDPHY_CTL1_rstz
#define MIPI_DPHY_RSTZ 1
#define MIPI_DPHY_RSTZ_WIDTH 1

//uDPHY_CTL_enable
#define MIPI_DPHY_CTL_ENABLE 15
#define MIPI_DPHY_CTL_ENABLE_WIDTH 12

//uDPHY_CTL1_enableclkBIU
#define MIPI_DPHY_ENABLECLK_BIU 4
#define MIPI_DPHY_ENABLECLK_BIU_WIDTH 4

//uDPHY_CTL1_cfgclkfreqrange
#define MIPI_DPHY_ENABLECLK_FREQUENCY_RANGE 29
#define MIPI_DPHY_ENABLECLK_FREQUENCY_RANGE_WIDTH 24

//uDPHY_PLL2_pll_shadow_control
#define MIPI_DPHY_PLL2_PLL_SHADOW_CONTROL 12
#define MIPI_DPHY_PLL2_PLL_SHADOW_CONTROL_WIDTH 12

//uDPHY_PLL2_clksel
#define MIPI_PLL2_CLKSEL 10
#define MIPI_PLL2_CLKSEL_WIDTH 9

//uDPHY_RB0_stopstateclk
#define MIPI_DPHY_RB0_STOPSTATECLK 12
#define MIPI_DPHY_RB0_STOPSTATECLK_WIDTH 12

//uDPHY_RB0_stopstatedata
#define MIPI_RB0_STOPSTATEDATA 17
#define MIPI_RB0_STOPSTATEDATA_WIDTH 14

#define MIPI_FIELD_SET(VAR, FIELD, VAL) \
_BFSET_(VAR, FIELD, FIELD##_WIDTH, VAL)

#define MIPI_FIELD_CLR_MASK(FIELD) \
_NCLRMASK_(FIELD, FIELD##_WIDTH)

struct dphy_tx_dev;

u32 dphy_tx_get_version(struct dphy_tx_dev *dev);
void dphy_tx_power_control(struct dphy_tx_dev *dev, int enable);
int dphy_tx_get_power_status(struct dphy_tx_dev *dev);
void dphy_tx_reset(struct dphy_tx_dev *dev, int reset);
void dphy_tx_shutdown(struct dphy_tx_dev *dev, int shutdown);

void dphy_tx_set_hs_freq_range(struct dphy_tx_dev *dev, uint8_t freq_range);
uint8_t dphy_tx_get_hs_freq_range(struct dphy_tx_dev *dev);
void dphy_tx_set_cfg_clk_freq_range(struct dphy_tx_dev *dev, uint8_t freq_range);
void dphy_tx_enable_continuous_mode(struct dphy_tx_dev *dev, int enable);
void dphy_tx_enable_bist(struct dphy_tx_dev *dev, int enable);

void dphy_tx_enable_lane(struct dphy_tx_dev *dev, uint8_t lane, int enable);
void dphy_tx_force_tx_stop_mode(struct dphy_tx_dev *dev, uint8_t lane, int force);
void dphy_tx_set_turn_disable(struct dphy_tx_dev *dev, uint8_t lane, int disable);
void dphy_tx_force_rx_mode(struct dphy_tx_dev *dev, uint8_t lane, int force);
void dphy_tx_set_base_dir(struct dphy_tx_dev *dev, uint8_t lane, int direction);

void dphy_tx_request_hs_clk(struct dphy_tx_dev *dev, int request);
void dphy_tx_request_hs_data(struct dphy_tx_dev *dev, uint8_t lane, int request);
void dphy_tx_set_hs_data(struct dphy_tx_dev *dev, uint8_t lane, uint8_t data);
uint8_t dphy_tx_get_hs_data(struct dphy_tx_dev *dev, uint8_t lane);

void dphy_tx_set_escape_data(struct dphy_tx_dev *dev, uint8_t lane, uint8_t data);
uint8_t dphy_tx_get_escape_data(struct dphy_tx_dev *dev, uint8_t lane);
void dphy_tx_set_escape_trigger(struct dphy_tx_dev *dev, uint8_t lane, uint8_t trigger);
void dphy_tx_request_escape(struct dphy_tx_dev *dev, uint8_t lane, int request);
void dphy_tx_set_escape_lpdt(struct dphy_tx_dev *dev, uint8_t lane, int enable);
void dphy_tx_set_escape_valid(struct dphy_tx_dev *dev, uint8_t lane, int valid);

void dphy_tx_set_ulps_clk(struct dphy_tx_dev *dev, int enable);
void dphy_tx_exit_ulps_clk(struct dphy_tx_dev *dev, int exit);
void dphy_tx_set_ulps_escape(struct dphy_tx_dev *dev, uint8_t lane, int enable);
void dphy_tx_exit_ulps_escape(struct dphy_tx_dev *dev, uint8_t lane, int exit);

int dphy_tx_get_pll_lock_status(struct dphy_tx_dev *dev);
int dphy_tx_get_stop_state_clk(struct dphy_tx_dev *dev);
int dphy_tx_get_stop_state_data(struct dphy_tx_dev *dev, uint8_t lane);
int dphy_tx_get_ulps_active_clk(struct dphy_tx_dev *dev);
int dphy_tx_get_ulps_active_data(struct dphy_tx_dev *dev, uint8_t lane);
int dphy_tx_get_hs_ready(struct dphy_tx_dev *dev, uint8_t lane);
int dphy_tx_get_escape_ready(struct dphy_tx_dev *dev, uint8_t lane);

int dphy_tx_get_control_error(struct dphy_tx_dev *dev, uint8_t lane);
int dphy_tx_get_contention_lp0_error(struct dphy_tx_dev *dev, uint8_t lane);
int dphy_tx_get_contention_lp1_error(struct dphy_tx_dev *dev, uint8_t lane);
int dphy_tx_get_escape_error(struct dphy_tx_dev *dev, uint8_t lane);
int dphy_tx_get_sync_escape_error(struct dphy_tx_dev *dev, uint8_t lane);

void dphy_tx_update_pll(struct dphy_tx_dev *dev);
void dphy_tx_set_pll_n_divider(struct dphy_tx_dev *dev, uint8_t n);
uint8_t dphy_tx_get_pll_n_divider(struct dphy_tx_dev *dev);
void dphy_tx_set_pll_m_multiplier(struct dphy_tx_dev *dev, uint16_t m);
uint16_t dphy_tx_get_pll_m_multiplier(struct dphy_tx_dev *dev);
void dphy_tx_set_pll_vco_control(struct dphy_tx_dev *dev, uint8_t vco_ctrl);
void dphy_tx_set_pll_prop_control(struct dphy_tx_dev *dev, uint8_t prop_ctrl);
void dphy_tx_set_pll_int_control(struct dphy_tx_dev *dev, uint8_t int_ctrl);
void dphy_tx_set_pll_gmp_control(struct dphy_tx_dev *dev, uint8_t gmp_ctrl);
void dphy_tx_set_pll_cpbias_control(struct dphy_tx_dev *dev, uint8_t cpbias_ctrl);
void dphy_tx_set_pll_clk_select(struct dphy_tx_dev *dev, uint8_t clk_sel);
void dphy_tx_force_pll_lock(struct dphy_tx_dev *dev, int force);
void dphy_tx_enable_pll_shadow_control(struct dphy_tx_dev *dev, int enable);
void dphy_tx_clear_pll_shadow(struct dphy_tx_dev *dev);
void dphy_tx_enable_gp_clk(struct dphy_tx_dev *dev, int enable);

void dphy_tx_set_skew_cal_hs(struct dphy_tx_dev *dev, int enable);
void dphy_tx_set_blank_clk_sel(struct dphy_tx_dev *dev, int select);

void dphy_tx_turn_request(struct dphy_tx_dev *dev, uint8_t lane, int request);

void dphy_tx_write_reg(struct dphy_tx_dev *dev, u32 reg_offset, u32 value);
u32 dphy_tx_read_reg(struct dphy_tx_dev *dev, u32 reg_offset);
void dphy_tx_write_reg_field(struct dphy_tx_dev *dev, u32 reg_offset,
			     u32 mask, u32 shift, u32 value);
u32 dphy_tx_read_reg_field(struct dphy_tx_dev *dev, u32 reg_offset,
			   u32 mask, u32 shift);

int dphy_tx_init(struct dphy_tx_dev *dev);
void dphy_tx_deinit(struct dphy_tx_dev *dev);
int dphy_tx_configure(struct dphy_tx_dev *dev, void *config);
int dphy_tx_start(struct dphy_tx_dev *dev);
void dphy_tx_stop(struct dphy_tx_dev *dev);

#ifdef __cplusplus
}
#endif

#endif /* DPHYTX_RELEASE_H */
