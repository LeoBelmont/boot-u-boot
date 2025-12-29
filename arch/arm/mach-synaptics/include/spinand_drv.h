/* SPDX-License-Identifier: GPL-2.0+ */
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

#ifndef _SPINAND_DRV_H_
#define _SPINAND_DRV_H_
#include <linux/types.h>

#define RANDOMIZER_BUFF_SIZE 4096
#define MAX_PAGE_SIZE 8192
#define IMG_HDR_MAGIC_NUMBER 0xD2ADA3F1
#define NAND_BLOCK0_SIZE 0x20000
#define NAND_BOOT_PARTITION_SIZE 0x80000
#define VT_OFFSET_FROM_BOTTOM 2048
enum xspi_ops {
	XSPI_READ,
	XSPI_WRITE,
};

extern struct mtd_info *mtd_nand;

int parse_version_table(u8 *buff);
int get_subimg_blks(int idx);

struct mtd_info *xspi_nand_init(void);
int syna_spinand_read(u32 offset, u32 size, u32 addr);
int syna_spinand_write(u32 offset, u32 size, u32 addr);
int detect_randomized_blks(enum xspi_ops ops, uint32_t wbuf);
void spinand_boot_prepare(void);
void spi_nand_image_read(const char *cmd, void *read_buffer, unsigned int read_bytes);
#endif
