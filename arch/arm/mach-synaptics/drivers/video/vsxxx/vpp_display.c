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

#include <linux/delay.h>
#include "OSAL_api.h"
#include <linux/types.h>
#include "tee_ca_vpp.h"
#include "soc-syna.h"
#include "com_type.h"
#include "vpp_api.h"
#include "vpp_cmd.h"
#include "vpp_cfg.h"
#include "avpll.h"
#include "vpp_priv.h"
#include "vbuf.h"
#include "vpp.h"
#include <irq_func.h>
#include <cpu_func.h>
#include <malloc.h>

#define WAIT_LOOP_COUNT 50

#define MP_BERLIN_INTR_ID(id)   ((id) + 32)

#if defined IRQ_dHubIntrAvio0
#define IRQ_DHUB_INTR_AVIO_0 IRQ_dHubIntrAvio0
#elif defined(IRQ_dHubIntrAvio0_0)
#define IRQ_DHUB_INTR_AVIO_0 IRQ_dHubIntrAvio0_0
#endif

#define RESINFO_PARAM_UPDATE(VAR)	m_resinfo_table[RES_DSI_CUSTOM].VAR = \
					pMipiConfigParams->infoparams.resInfo.VAR

typedef struct __SetClockFreq_Data_ {
	int pllsrc;
	int isInterlaced;
	int refresh_rate;
	int h_active;
	int v_active;
	int h_total;
	int v_total;
} SetClockFreq_Data;

static int g_pushframe_done;
static int frm_count;
static VPP_WIN_ATTR showlogo_attr = {0x00801080, 0xFFF, 1};
static int loop_isr_count;
static int isr_enabled;

VOID VPP_ISR_Handler_irq(VOID *param)
{
	int ret;

	loop_isr_count++;
	ret = VppIsrHandler(VPP_CC_MSG_TYPE_VPP, 0);
	if (ret)
		printf("vpp isr handler failed\n");

	if (g_pushframe_done) {
		frm_count++;
		g_pushframe_done = 0;
	}
}

void MV_VPP_Enable_IRQ(void)
{
	irq_install_handler(MP_BERLIN_INTR_ID(IRQ_DHUB_INTR_AVIO_0),
			    VPP_ISR_Handler_irq, NULL);
	isr_enabled = 1;
}

void MV_VPP_Disable_IRQ(void)
{
	irq_free_handler(MP_BERLIN_INTR_ID(IRQ_DHUB_INTR_AVIO_0));
}

int MV_VPPOBJ_Mipi_LoadInfoTable(struct vpp_config_params *vpp_config_param)
{
	VPP_MIPI_CONFIG_PARAMS *pMipiConfigParams;
	RESOLUTION_INFO *rescfg;
	VPLL_CONFIG pllcfg;

	pMipiConfigParams =  (((VPP_MIPI_CONFIG_PARAMS *)vpp_config_param->mipi_resinfo_params));
	if (!pMipiConfigParams) {
		printf("Invalid MIPI Config Params\n");
		return -EINVAL;
	}

	rescfg = &pMipiConfigParams->infoparams.resInfo;
	RESINFO_PARAM_UPDATE(active_width);
	RESINFO_PARAM_UPDATE(active_height);
	RESINFO_PARAM_UPDATE(hfrontporch);
	RESINFO_PARAM_UPDATE(hsyncwidth);
	RESINFO_PARAM_UPDATE(hbackporch);
	RESINFO_PARAM_UPDATE(vfrontporch);
	RESINFO_PARAM_UPDATE(vsyncwidth);
	RESINFO_PARAM_UPDATE(vbackporch);
	RESINFO_PARAM_UPDATE(type);
	RESINFO_PARAM_UPDATE(scan);
	RESINFO_PARAM_UPDATE(frame_rate);
	RESINFO_PARAM_UPDATE(freq);
	RESINFO_PARAM_UPDATE(flag_3d);
	RESINFO_PARAM_UPDATE(pts_per_cnt_4);

	m_resinfo_table[RES_DSI_CUSTOM].width = rescfg->active_width +
						rescfg->hfrontporch +
						rescfg->hsyncwidth +
						rescfg->hbackporch;

	m_resinfo_table[RES_DSI_CUSTOM].height = rescfg->active_height +
						 rescfg->vfrontporch +
						 rescfg->vsyncwidth +
						 rescfg->vbackporch;

	memset(&pllcfg, 0, sizeof(VPLL_CONFIG));
	/* Fill the PLL and config the clock */
	pllcfg.h_active = rescfg->active_width;
	pllcfg.v_active = rescfg->active_height;
	pllcfg.isInterlaced = rescfg->scan;
	pllcfg.frame_rate_enum = rescfg->frame_rate;
	pllcfg.h_total = m_resinfo_table[RES_DSI_CUSTOM].width;
	pllcfg.v_total = m_resinfo_table[RES_DSI_CUSTOM].height;

	if (AVPLL_GetClkgenparams(pMipiConfigParams->infoparams.resInfo.freq,
				  &pllcfg.Dm, &pllcfg.Dn,
				  &pllcfg.frac, &pllcfg.Dp) != MV_VPP_OK) {
		return MV_VPP_EUNSUPPORT;
	}

	AVPLL_Load_ResConfig(&pllcfg);

	return 0;
}

static void AVPLL_SetClockFreq_Pack(int *p_packed_data, SetClockFreq_Data *p_data)
{
	p_packed_data[0] = p_data->pllsrc;
	p_packed_data[1] = p_data->isInterlaced << 8 | p_data->refresh_rate;
	p_packed_data[2] = p_data->h_active << 16 | p_data->v_active;
	p_packed_data[3] = p_data->h_total << 16 | p_data->v_total;
}

int MV_VPP_SetFormat(INT handle, INT cpcbID, VPP_DISP_OUT_PARAMS *pDispParams)
{
	SetClockFreq_Data clock_data;
	RESOLUTION_INFO *resinfo;
	int packed_data[4];
	UINT32  resID;

	resID = pDispParams->uiResId;
	if (pDispParams->uiDispId == VOUT_DSI)
		resinfo = &m_resinfo_table[RES_DSI_CUSTOM];
	else
		resinfo = &m_resinfo_table[resID];

	clock_data.pllsrc = 0;
	clock_data.isInterlaced = resinfo->scan;
	clock_data.refresh_rate = resinfo->frame_rate;
	clock_data.h_active = resinfo->active_width;
	clock_data.v_active = resinfo->active_height;
	clock_data.h_total = resinfo->width;
	clock_data.v_total = resinfo->height;
	AVPLL_SetClockFreq_Pack(&packed_data[0], &clock_data);

	if (cpcbID == CPCB_1)
		AVPLL_SetClockFreq(0, packed_data[1], packed_data[2], packed_data[3]);
	if (pDispParams->uiDispId == VOUT_DSI)
		AVPLL_SetClockFreq(1, packed_data[1], packed_data[2], packed_data[3]);

	return VppSetFormat(0, cpcbID, pDispParams);
}

int MV_VPPOBJ_SetFormat(struct vpp_config_params *vpp_config_param)
{
	int display_primary_HDMI_BitDepth = 2;
	VPP_DISP_OUT_PARAMS dispParams;
	int ret;

	dispParams.uiBitDepth = display_primary_HDMI_BitDepth;
	dispParams.uiColorFmt = OUTPUT_COLOR_FMT_RGB888;
	dispParams.iPixelRepeat = 1;

	switch (vpp_config_param->display_mode) {
	case VOUT_DISP_SINGLE_MODE_PRI:
		dispParams.uiDispId         = VOUT_HDMI;
		dispParams.uiDisplayMode    = VOUT_DISP_SINGLE_MODE_PRI;
		dispParams.uiResId          = vpp_config_param->disp1_res_id;
		return  MV_VPP_SetFormat(0, CPCB_1, &dispParams);
	case VOUT_DISP_SINGLE_MODE_SEC:
		dispParams.uiDispId         = VOUT_DSI;
		dispParams.uiDisplayMode    = VOUT_DISP_SINGLE_MODE_SEC;
		dispParams.uiResId          = vpp_config_param->disp1_res_id;
		return  MV_VPP_SetFormat(0, CPCB_1, &dispParams);
	case VOUT_DISP_DUAL_MODE_PIP:
		dispParams.uiDispId         = VOUT_HDMI;
		dispParams.uiDisplayMode    = VOUT_DISP_DUAL_MODE_PIP;
		dispParams.uiResId          = vpp_config_param->disp1_res_id;
		ret = MV_VPP_SetFormat(0, CPCB_1, &dispParams);
		if (ret)
			return ret;

		dispParams.uiDispId         = VOUT_DSI;
		dispParams.uiDisplayMode    = VOUT_DISP_DUAL_MODE_PIP;
		dispParams.uiResId          = vpp_config_param->disp2_res_id;
		return  MV_VPP_SetFormat(0, CPCB_2, &dispParams);
	default:
		printf("Unknown display mode\n");
		break;
	}

	return -EINVAL;
}

int MV_VPP_pushframe(VBUF_INFO *p_vpp_buf, INT plane_id, int width, int height)
{
	VPP_WIN showlogo_win;
	unsigned int size;
	int ret;

	if (isr_enabled)
		MV_VPP_Disable_IRQ();

	MV_VPPOBJ_SetPlaneMute(0, plane_id, 0);

	p_vpp_buf->m_active_left = 0;
	p_vpp_buf->m_active_top = 0;

	size = p_vpp_buf->m_buf_stride * p_vpp_buf->m_active_height;
	showlogo_win.x = 0;
	showlogo_win.y = 0;
	showlogo_win.width  = width;
	showlogo_win.height = height;

	/* Set the Display window based on resolution */
	MV_VPPOBJ_OpenDispWindow(0, plane_id, &showlogo_win, &showlogo_attr);

	/* Set the Reference window based on Frameinfo */
	showlogo_win.width  = p_vpp_buf->m_active_width;
	showlogo_win.height = p_vpp_buf->m_active_height;
	MV_VPPOBJ_SetRefWindow(0, plane_id, &showlogo_win);

	flush_dcache_range((uintptr_t)p_vpp_buf->m_pbuf_start,
			   (uintptr_t)(((char *)p_vpp_buf->m_pbuf_start) + size));

	MV_VPPOBJ_SetDisplayMode(0, plane_id, DISP_FRAME);
	build_frames(p_vpp_buf, p_vpp_buf->m_srcfmt, INPUT_BIT_DEPTH_8BIT,
		     p_vpp_buf->m_active_left, p_vpp_buf->m_active_top,
		     p_vpp_buf->m_active_width, p_vpp_buf->m_active_height,
		     1, 0, 0);

	ret = MV_VPPOBJ_DisplayFrame(0, plane_id, p_vpp_buf);
	if (ret != 0)
		printf("Diaplay frame failed\n");

	if (isr_enabled)
		MV_VPP_Enable_IRQ();

	return ret;
}

int MV_VPP_push_nullframe(int width, int height, int stride, INT plane_id)
{
	VBUF_INFO *vbuf;
	UINT32 size;
	void *pbuf;
	INT32 ret;

	pbuf = malloc(width * height * stride);
	if (!pbuf)
		return -ENOMEM;

	get_vbuf_info(&vbuf);

	vbuf->m_pbuf_start   = (ARCH_PTR_TYPE)pbuf;
	vbuf->m_disp_offset  = 0;
	vbuf->m_active_left  = 0;
	vbuf->m_active_top   = 0;
	vbuf->m_buf_stride = stride;
	vbuf->m_active_width = width;
	vbuf->m_active_height = height;
	vbuf->m_srcfmt = SRCFMT_XRGB32;
	size = vbuf->m_buf_stride * vbuf->m_active_height;
	flush_dcache_range((uintptr_t)pbuf, (uintptr_t)(((char *)pbuf) + size));

	ret = MV_VPP_pushframe(vbuf, plane_id, width, height);
	if (ret) {
		printf("Push frame failed\n");
		return ret;
	}

	g_pushframe_done++;

	return ret;
}

static int MV_VPPOBJ_Open_Plane(int plane_id, RESOLUTION_INFO *resinfo,
				VPP_WIN *win, VPP_WIN_ATTR *attr)
{
	int ret;

	ret = MV_VPPOBJ_OpenDispWindow(0, plane_id, win, attr);
	if (ret != MV_VPP_OK)
		return ret;
	MV_VPPOBJ_SetRefWindow(0, plane_id, win);

	MV_VPPOBJ_SetDisplayMode(0, plane_id, DISP_FRAME);

	ret = create_global_desc_array(SRCFMT_XRGB32, INPUT_BIT_DEPTH_8BIT,
				       resinfo->active_width,
				       resinfo->active_height);
	if (ret != MV_VPP_OK)
		return ret;

	ret = MV_VPP_push_nullframe(resinfo->active_width, 0, 0, plane_id);
	if (ret != MV_VPP_OK) {
		printf("NULL frame failed - Plane_id = %d\n", plane_id);
		return ret;
	}

	return ret;
}

int MV_VPPOBJ_Config_Display(struct vpp_config_params *vpp_config_param)
{
	RESOLUTION_INFO resinfo;
	VPP_WIN showlogo_win;
	int ret = 0;

	resinfo = m_resinfo_table[vpp_config_param->disp1_res_id];

	showlogo_win.x = 0;
	showlogo_win.y = 0;
	showlogo_win.width  = resinfo.active_width;
	showlogo_win.height = resinfo.active_height;

	if (IS_ENABLED(CONFIG_TARGET_DOLPHIN)) {
		if (!IS_MODE_DUAL(vpp_config_param->display_mode)) {
			ret = MV_VPPOBJ_Open_Plane(PLANE_PIP, &resinfo, &showlogo_win,
						   &showlogo_attr);
			if (ret != MV_VPP_OK)
				return ret;
		}
	}

	ret = MV_VPPOBJ_Open_Plane(PLANE_GFX1, &resinfo, &showlogo_win, &showlogo_attr);
	if (ret != MV_VPP_OK)
		return ret;

	return ret;
}

int syna_get_display_modeinfo(struct berlin_fb_priv *priv, int *width,
			      int *height, int display, avio_fastlogo_info *dispinfo)
{
	RESOLUTION_INFO resinfo;

	if (display == DISPLAY_1) {
		resinfo = m_resinfo_table[priv->vpp_config_param.disp1_res_id];
		dispinfo->u.cpcb0_res_id = priv->vpp_config_param.disp1_res_id;
	} else if (IS_MODE_DUAL(priv->vpp_config_param.display_mode) &&
			 (display == DISPLAY_2)) {
		resinfo = m_resinfo_table[priv->vpp_config_param.disp2_res_id];
		dispinfo->u.cpcb1_res_id = priv->vpp_config_param.disp2_res_id;
	} else {
		return -EINVAL;
	}

	*width = resinfo.active_width;
	*height = resinfo.active_height;

	return 0;
}

void MV_VPPOBJ_StopDisplay(void)
{
	loop_isr_count = 0;

	while (loop_isr_count < WAIT_LOOP_COUNT)
		mdelay(2);
}

void MV_VPPOBJ_DestroyDisplay(void)
{
	MV_VPPOBJ_StopDisplay();

	MV_VPP_Disable_IRQ();

	MV_VPPOBJ_Stop(0);
}
