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

#include "hal_vpp_wrap.h"
#include "vpp_api.h"
#include "vpp.h"

int MV_VPP_Init(struct berlin_fb_priv *priv)
{
	int ret;

	ret = wrap_MV_VPP_InitVPPS(priv);
	if (ret) {
		printf("VPP init failed = %d\n", ret);
		return ret;
	}

	ret = wrap_MV_VPP_Create();
	if (ret) {
		printf("VPP create failed = %d\n", ret);
		return ret;
	}

	ret = wrap_MV_VPP_Reset();
	if (ret) {
		printf("VPP Reset failed = %d\n", ret);
		return ret;
	}

	ret = wrap_MV_VPP_Config();
	if (ret) {
		printf("VPP Config failed = %d\n", ret);
		return ret;
	}

	if (IS_MODE_MIPI(priv->vpp_config_param.display_mode)) {
		ret = wrap_MV_VPP_Mipi_LoadInfoTable(priv);
		if (ret) {
			printf("MIPI Load table info failed - %d\n", ret);
			return ret;
		}

		ret = wrap_MV_VPP_Mipi_LoadConfig(priv);
		if (ret) {
			printf("MIPI Load info params failed - %d\n", ret);
			return ret;
		}
	}

	ret = wrap_MV_VPP_Set_Format(priv);
	if (ret) {
		printf("VPP set format failed = %d\n", ret);
		return ret;
	}

	ret = wrap_MV_VPP_SetHdmiTxControl();
	if (ret) {
		printf("VPP set HDMI TX failed = %d\n", ret);
		return ret;
	}

	return ret;
}

int MV_VPP_Config_Display(struct berlin_fb_priv *priv)
{
	int ret;

	ret = wrap_MV_VPP_Config_Display(priv);
	if (ret) {
		printf("VPP Display config failed = %d\n", ret);
	}

	return ret;
}

void MV_VPP_Enable_Interrupt(struct berlin_fb_priv *priv)
{
	wrap_MV_VPP_Enable_Interrupt(priv);
}

int MV_VPP_Display_Frame(struct berlin_fb_priv *priv, VBUF_INFO *pVppBuf,
			 int display)
{
	int ret;

	ret = wrap_MV_VPP_Display_Frame(priv, pVppBuf, display);
	if (ret) {
		printf("VPP Display %d Frame failed = %d - \n", display, ret);
	}

	return ret;
}

void MV_VPP_Stop(void)
{
	wrap_MV_VPP_Stop();
}

void MV_VPP_DeInit(struct berlin_fb_priv *priv)
{
	wrap_MV_VPP_DeInit(priv);
}
