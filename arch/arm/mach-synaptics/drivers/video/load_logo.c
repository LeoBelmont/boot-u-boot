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

#include <linux/types.h>
#include <cpu_func.h>
#include <asm/cache.h>
#include <stdlib.h>
#include "vpp_api.h"
#include "vbuf.h"
#include "part_efi.h"
#include "vdec_com.h"
#include "mmc.h"
#include "misc_syna.h"
#include "OSAL_api.h"
#include "fastboot_syna.h"
#include "vpp_priv.h"
#include "tee_client.h"
#include "mem_init.h"

#ifdef CONFIG_GENX_ENABLE
#include "genimg.h"
#endif

#define LOGO_NAME			"fastlogo"
#define LOGO_A_NAME			"fastlogo_a"
#define LOGO_B_NAME			"fastlogo_b"
#define MAX_PARTITION		32
#define MAX_LOGO_NAMES		3
#define MAX_PARTITION_NAME_SIZE	8

#define MAX_PAGESIZE            8192
#define IMAGE_TYPE_FAST_LOGO    0x27
#define MIN_FASTLOGO_IMG_SIZE   (0.5 * 1024 * 1024)
#define MAX_FASTLOGO_IMG_SIZE   (64 * 1024 * 1024)

#ifndef CONFIG_GENX_ENABLE
#define GENX_IMAGE_HEADER_FASTLOGO_SIZE		0
#else
#define GENX_IMAGE_HEADER_FASTLOGO_SIZE		(336 + PREPEND_IMAGE_INFO_SIZE)
#endif

#define LOGO_HEADER_SIZE	(1024 + GENX_IMAGE_HEADER_FASTLOGO_SIZE)

struct fl_logo_info {
	unsigned long addr;
	unsigned int width;
	unsigned int height;
	unsigned int stride;
};

/* Global array to store multiple logo info (one per display) */
static struct fl_logo_info g_logo_info[MAX_NUM_DISPLAY];
static int g_logo_info_cnt;

struct pt_info {
	__le64 part;
	__le64 start_lba;
	__le64 cnt;
	char partition_name[PARTNAME_SZ + 1];
};

typedef struct disk_partition disk_partition_t;

typedef struct {
	unsigned int offset;
	unsigned int width;
	unsigned int height;
	unsigned int stride;
} fastlogo_info_t;

typedef struct {
	unsigned int versionNum;
	unsigned int logo_num;
	fastlogo_info_t info[];
} fastlogo_header_t;

static unsigned int syna_get_blksize(void)
{
	struct blk_desc *dev_desc;
	int mmc_dev = get_mmc_boot_dev();
	struct mmc *mmc = find_mmc_device(mmc_dev);

	if (!mmc) {
		printf("invalid mmc device\n");
		return -1;
	}

	dev_desc = blk_get_dev("mmc", mmc_dev);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("invalid mmc device\n");
		return -1;
	}

	return (unsigned int)dev_desc->blksz;
}

void *syna_emmc_read_from_offset(const char *partition_name, unsigned int offset,
				 unsigned int size, void *buff,
				 FASTLOGO_INFO *fastlogo_display_info)
{
	struct blk_desc *dev_desc;
	disk_partition_t info;
	lbaint_t start_blk, blk_cnt;
	int mmc_dev = get_mmc_boot_dev();
	struct mmc *mmc = find_mmc_device(mmc_dev);
	int part_type;

	if (!mmc) {
		printf("invalid mmc device\n");
		return NULL;
	}

	dev_desc = blk_get_dev("mmc", mmc_dev);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("invalid mmc device\n");
		return NULL;
	}

	part_type = get_mmc_part_by_name(mmc_dev, partition_name);
	blk_dselect_hwpart(dev_desc, part_type);
	if (part_get_info_by_name(dev_desc, partition_name, &info) == -1) {
		printf("cannot find partition: '%s'\n", partition_name);
		return NULL;
	}

	fastlogo_display_info->hw_partition = part_type;
	fastlogo_display_info->sw_partition = f_mmc_get_part_index(mmc_dev, partition_name);
	fastlogo_display_info->devnum = dev_desc->devnum;

	start_blk = info.start + (offset / dev_desc->blksz);
	blk_cnt = size / dev_desc->blksz + 2;
	blk_dread(dev_desc, start_blk, blk_cnt, buff);

	return (buff + (offset % dev_desc->blksz));
}

static fastlogo_info_t *check_validate_logo(int width, int height, UINT8 *pHEADER)
{
	fastlogo_header_t *fl_header_info = (fastlogo_header_t *)pHEADER;
	int i;

	for (i = 0; i < fl_header_info->logo_num; i++) {
		debug("number of logo %d w[%d] H[%d] O[%d] st[%d]\n",
		      fl_header_info->logo_num,
		      fl_header_info->info[i].width,
		      fl_header_info->info[i].height,
		      fl_header_info->info[i].offset,
		      fl_header_info->info[i].stride);

#ifdef VPP_SUPPORT_SCALAR
		if (fl_header_info->info[i].width > 0 &&
		    fl_header_info->info[i].width <= width &&
		    fl_header_info->info[i].height > 0 &&
		    fl_header_info->info[i].height <= height) {
			return &fl_header_info->info[i];
		}
#else
		if (fl_header_info->info[i].width == width &&
		    fl_header_info->info[i].height == height) {
			return &fl_header_info->info[i];
		}
#endif
	}

	return NULL;
}

int syna_load_logo_info(int width, int height, VBUF_INFO *p_vpp_buf, FASTLOGO_INFO *fl_info)
{
	int ret = -1;
	unsigned char *buff = NULL;
	unsigned char *img_buff = NULL;
	unsigned int block_size;
	unsigned int pad_size;
	int total_header_size;

	struct img_header *img_hdr;
	u32 read_size, img_size, logo_size;

	struct img_info *img_info;
	fastlogo_info_t *fl_header;
	UINT8 *read_buffer, *logo_buffer, *header, *logo_header = NULL;
	int ab_mode;
	const char *pt_name;

	if (!(IS_ENABLED(CONFIG_MMC))) {
		//Only support fastlogo on emmc image, fail for SPI/RAM
		printf("fastlogo: Not supported!!!!!!!!\n");
		return -1;
	}

	ab_mode = get_current_slot();
	if (ab_mode != BOOTSEL_A && ab_mode != BOOTSEL_B) {
		printf("fastlogo: No bootable slots found for fastlogo loading, ...!!\n");
		return -1;
	}

	pt_name = (ab_mode == BOOTSEL_A) ?  LOGO_A_NAME : LOGO_B_NAME;

	block_size = syna_get_blksize();
	pad_size = block_size * 2;
	total_header_size = GENX_IMAGE_HEADER_FASTLOGO_SIZE + LOGO_HEADER_SIZE + pad_size;
	logo_header = (UINT8 *)malloc_ion_cacheable(total_header_size);

	debug("fastlogo: logo partition name %s header-size:%d, [blk/pad]_size %d/%d\n",
	      pt_name, total_header_size, block_size, pad_size);
	printf("fastlogo: logo partition name %s header-size:%d, [blk/pad]_size %d/%d\n",
	       pt_name, total_header_size, block_size, pad_size);

	header = syna_emmc_read_from_offset(pt_name, 0,
					    total_header_size, logo_header, fl_info);

	if (!header) {
		printf("fastlogo: Header read failed in partition - %s\n", pt_name);
		goto error_out1;
	}

	img_info = (struct img_info *)header;
	if (img_info->magic != IMG_INFO_MAGIC) {
		printf("fastlogo: incorrect magic in image info  0x%08x\n", img_info->magic);
		goto error_out1;
	}

	/* find out fastlogo image read size */
	img_size = img_info->image_size;
	img_size = ALIGN(img_size, 16);
	read_size = img_size + PREPEND_IMAGE_INFO_SIZE;

	/* check read size  */
	if (read_size > MAX_FASTLOGO_IMG_SIZE || read_size < MIN_FASTLOGO_IMG_SIZE) {
		printf("fastlogo: img_size is invalid - %u\n", read_size);
		goto error_out1;
	}

	img_buff = malloc_ion_cacheable(read_size + pad_size);
	buff = syna_emmc_read_from_offset(pt_name, 0, read_size, img_buff, fl_info);
	if (!buff) {
		printf("fastlogo: image read failed in partition - %s\n", pt_name);
		goto error_out2;
	}

	img_hdr = (void *)(img_buff + PREPEND_IMAGE_INFO_SIZE);

	/* verify image */
	ret = tee_verify_image(5, (void *)img_hdr, img_size,
			       (void *)img_hdr, img_size, IMAGE_TYPE_FAST_LOGO);
	if (ret <= 0) {
		printf("fastlogo: Verify FASTLOGO image failed! ret=0x%x\n", ret);
		goto error_out2;
	} else {
		printf("fastlogo: Verify FASTLOGO image passed! ret=0x%x\n", ret);
		//reset return value
		ret = 0;
	}

	header = img_buff + GENX_IMAGE_HEADER_FASTLOGO_SIZE;
	fl_header = check_validate_logo(width, height, header);
	if (!fl_header) {
		printf("fastlogo: No matching logo found for WxH:[%d]x[%d]\n",
		       width, height);
		ret = -1;
		goto error_out2;
	}

	printf("fastlogo: matching logo found WxH:%dx%d->%dx%d\n",
	       width, height, fl_header->width, fl_header->height);
	logo_buffer = fl_header->offset + header;
	logo_size = (fl_header->stride * fl_header->height) + pad_size;
	read_buffer = (UINT8 *)malloc(logo_size);
	flush_dcache_range((unsigned long)img_buff, (unsigned long)(img_buff + read_size));
	memcpy(read_buffer, logo_buffer, logo_size);

	g_logo_info[g_logo_info_cnt].addr = (unsigned long)read_buffer;
	g_logo_info[g_logo_info_cnt].width = fl_header->width;
	g_logo_info[g_logo_info_cnt].height = fl_header->height;
	g_logo_info[g_logo_info_cnt].stride = fl_header->stride;
	g_logo_info_cnt++;  /* Increment for next display */

	p_vpp_buf->m_srcfmt = LOGO_SRC_FMT;
	p_vpp_buf->m_bytes_per_pixel = (LOGO_SRC_FMT == SRCFMT_YUV422) ? 2 : 3;
	p_vpp_buf->m_pbuf_start = read_buffer;
	p_vpp_buf->m_content_width = fl_header->width;
	p_vpp_buf->m_content_height = fl_header->height;
	p_vpp_buf->m_buf_stride =  fl_header->stride;
	p_vpp_buf->m_buf_size =  p_vpp_buf->m_buf_stride * p_vpp_buf->m_content_height;
	p_vpp_buf->m_active_width = fl_header->width;
	p_vpp_buf->m_active_height = fl_header->height;
	//Indicate the bitdepth of the frame, if 8bit, is 8, if 10bit, is 10
	p_vpp_buf->m_bits_per_pixel = p_vpp_buf->m_bytes_per_pixel * 8;
	p_vpp_buf->m_order = 0;

error_out2:
	if (img_buff)
		free_ion_cacheable(img_buff);
error_out1:
	if (logo_header)
		free_ion_cacheable(logo_header);

	return ret;
}

int get_fastlogo_addr(char *fl_args)
{
	int i;

	fl_args[0] = '\0';
	/* logo parameter: syna_drm.logo_info=addr0@w0xh0-s0,addr1@w1xh1-s1 */
	for (i = 0; i < g_logo_info_cnt; i++) {
		if (g_logo_info[i].addr) {
			/* Add prefix on first entry, comma on subsequent entries */
			fl_args += sprintf(fl_args, i ? "," : "syna_drm.logo_info=");
			fl_args += sprintf(fl_args, "%08lx@%xx%x-%x",
				 g_logo_info[i].addr, g_logo_info[i].width,
				 g_logo_info[i].height, g_logo_info[i].stride);

			debug("U-Boot: Display[%d] logo: %08lx@%xx%x-%x\n",
			      i, g_logo_info[i].addr, g_logo_info[i].width,
			      g_logo_info[i].height, g_logo_info[i].stride);
		}
	}
	return i;
}
