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

#include <command.h>
#include <console.h>
#include <fs.h>
#include <malloc.h>
#include <net.h>
#include <net/tftp.h>
#include <part.h>
#include <part_efi.h>
#include <usb.h>
#include <nand.h>
#include <version_table.h>
#include <env.h>
#include <linux/ctype.h>
#include <linux/mtd/mtd.h>
#include "dm/uclass.h"
#include <dm/device.h>
#include <dm/device-internal.h>
#include <memalign.h>
#include <u-boot/crc.h>
#include "nand_randomizer.h"
#include "spinand_drv.h"
#include "fastboot_syna.h"

struct mtd_info *mtd_nand;

u8 random_buf[MAX_PAGE_SIZE];
u8 tmp_buff[MAX_PAGE_SIZE];
unsigned char randomizer_buff[RANDOMIZER_BUFF_SIZE];
u32 skip_randomized_blks;

int detect_randomized_blks(enum xspi_ops ops, u32 wbuf)
{
	int ret;
	struct img_hdr_t *img_ptr = NULL;
	int i;
	struct mtd_info *mtd;
	u8 *buff;

	if (skip_randomized_blks == 0) {
		if (ops == XSPI_READ) {
			buff = malloc(VT_OFFSET_FROM_BOTTOM);
			mtd = xspi_nand_init();
			if (!mtd) {
				printf("%s xspi nand init failed\n", __func__);
				return -1;
			}
			loff_t start;

			start = NAND_BLOCK0_SIZE + NAND_BOOT_PARTITION_SIZE -
				     VT_OFFSET_FROM_BOTTOM;

			for (i = 1; i <= 8; i++) {
				struct mtd_oob_ops ops = {
					.len = VT_OFFSET_FROM_BOTTOM,
					.datbuf = buff,
				};
				ret = mtd->_read_oob(mtd,
					       start + i * NAND_BOOT_PARTITION_SIZE, &ops);
				if (unlikely(ret < 0))
					continue;
				if (mtd->ecc_strength != 0 && ret >= mtd->bitflip_threshold)
					continue;

				ret = parse_version_table(buff);
				if (ret) {
					printf("parse partition info error\n");
					continue;
				}
				break;
			}
			skip_randomized_blks = get_subimg_blks(0) + get_subimg_blks(1);
			free(buff);
		} else {
			img_ptr = (struct img_hdr_t *)(uintptr_t)wbuf;

			if (img_ptr->magic != IMG_HDR_MAGIC_NUMBER) {
				printf("Invalid image magic: 0x%x\n", img_ptr->magic);
				return 1;
			}

			skip_randomized_blks = img_ptr->sub_image[0].chip_num_blocks +
							img_ptr->sub_image[1].chip_num_blocks;
		}
		printf("skip_randomized_blks = %d\n", skip_randomized_blks);
	}
	return 0;
}

u8 is_nand_block_randomized(u32 addr)
{
	const struct mv_nand_randomizer_s *p_nr = &g_nand_randomizer;
	u32 block_index = addr >> p_nr->block_shift;

	if (block_index < skip_randomized_blks)
		return false;
	else
		return true;
}

int syna_mtd_read_cb(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
	int ret;
	size_t done = 0;
	size_t copy_len;
	loff_t page_addr;
	u32 page;
	u32 page_off;
	int i;

	if (!len) {
		*retlen = 0;
		return 0;
	}

	ret = detect_randomized_blks(XSPI_READ, 0);
	if (ret) {
		printf("%s detect randomized blks failed\n", __func__);
		return ret;
	}

	while (done < len) {
		page     = from / mtd->writesize;
		page_off = from % mtd->writesize;
		page_addr = (loff_t)page * mtd->writesize;

		copy_len = mtd->writesize - page_off;
		if (copy_len > (len - done))
			copy_len = len - done;

		struct mtd_oob_ops ops = {
			.mode   = MTD_OPS_PLACE_OOB,
			.len    = mtd->writesize,
			.datbuf = random_buf,
		};

		ret = mtd->_read_oob(mtd, page_addr, &ops);
		if (unlikely(ret < 0)) {
			printf("%s %d read failed at 0x%llx, ret:%d, bitflip_threshold:%d\n",
			       __func__, __LINE__, from, ret, mtd->bitflip_threshold);
			return ret;
		}
		if (mtd->ecc_strength != 0 && ret >= mtd->bitflip_threshold) {
			printf("%s %d read failed at 0x%llx, ret:%d, bitflip_threshold:%d\n",
			       __func__, __LINE__, from, ret, mtd->bitflip_threshold);
			return -EBADMSG;
		}

		bool erased = true;

		for (i = 0; i < mtd->writesize; i++) {
			if (random_buf[i] != 0xFF) {
				erased = false;
				break;
			}
		}

		if (!erased && is_nand_block_randomized(page_addr)) {
			u32 out_len;

			out_len =
			mv_nand_randomizer_randomize_page(page * mtd->writesize,
							  random_buf, NULL, random_buf,
							  NULL);

			if (out_len != mtd->writesize) {
				printf("%s: randomizer failed @ page %u, out_len %x, writesize %d\n",
				       __func__, page, out_len, mtd->writesize);
				return -EIO;
			}
		}
		memcpy(buf + done, random_buf + page_off, copy_len);

		from += copy_len;
		done += copy_len;
	}

	*retlen = done;

	return 0;
}

int syna_mtd_write_cb(struct mtd_info *mtd, loff_t to, size_t len,
		      size_t *retlen, const u_char *buf)
{
	int ret;
	size_t done = 0;
	u32 page;
	loff_t page_addr;
	size_t copy_len;
	u32 page_off;

	if (!len) {
		*retlen = 0;
		printf("%s len error\n", __func__);
		return 0;
	}

	while (done < len) {
		page      = to / mtd->writesize;
		page_off  = to % mtd->writesize;
		page_addr = (loff_t)page * mtd->writesize;

		copy_len = mtd->writesize - page_off;
		if (copy_len > (len - done))
			copy_len = len - done;

		struct mtd_oob_ops rops = {
			.mode   = MTD_OPS_PLACE_OOB,
			.len    = mtd->writesize,
			.datbuf = random_buf,
		};

		ret = mtd->_read_oob(mtd, page_addr, &rops);
		if (unlikely((ret) < 0))
			return ret;
		if (mtd->ecc_strength && ret >= mtd->bitflip_threshold)
			return -EBADMSG;

		memcpy(random_buf + page_off, buf + done, copy_len);

		if (is_nand_block_randomized(page_addr)) {
			u32 out_len;

			out_len =
			mv_nand_randomizer_randomize_page(page_addr,
							  random_buf, NULL, tmp_buff, NULL);

			if (out_len != mtd->writesize) {
				printf("%s: randomizer failed @ page %u\n",
				       __func__, page);
				return -EIO;
			}
		} else {
			memcpy(tmp_buff, random_buf, mtd->writesize);
		}

		struct mtd_oob_ops wops = {
			.mode   = MTD_OPS_PLACE_OOB,
			.len    = mtd->writesize,
			.datbuf = tmp_buff,
		};

		ret = mtd->_write_oob(mtd, page_addr, &wops);
		if (ret < 0)
			return ret;

		to   += copy_len;
		done += copy_len;
	}

	*retlen = done;
	return 0;
}

struct mtd_info *xspi_nand_init(void)
{
	struct udevice *dev;
	char mtdids_buf[64];
	int ret;

	if (mtd_nand)
		return mtd_nand;

	ret = uclass_first_device_err(UCLASS_MTD, &dev);
	if (ret) {
		printf("%s No MTD device found\n", __func__);
		return NULL;
	}

	mtd_nand = dev_get_uclass_priv(dev);
	if (!mtd_nand) {
		printf("Failed to get MTD info\n");
		return NULL;
	}

	snprintf(mtdids_buf, sizeof(mtdids_buf), "%s=%s", mtd_nand->name, mtd_nand->name);
	env_set("mtdids", mtdids_buf);
	if (IS_ENABLED(CONFIG_NAND_RANDOMIZER)) {
		mv_nand_randomizer_init(mtd_nand->erasesize, mtd_nand->writesize,
					mtd_nand->oobsize, randomizer_buff,
					RANDOMIZER_BUFF_SIZE);
		mtd_nand->_read = syna_mtd_read_cb;
		mtd_nand->_write = syna_mtd_write_cb;
	}
	return mtd_nand;
}

int syna_spinand_read(u32 offset, u32 size, u32 addr)
{
	int ret = -1;
	size_t rw_size;
	struct mtd_info *mtd;
	int i = 0;
	u32 page_cnt = 0, remain = 0;
	u8 *buf = NULL;
	u32 randomized_length = 0;
	int randomized = 0;

	mtd = xspi_nand_init();
	if (!mtd) {
		printf("%s xspi nand init failed\n", __func__);
		return -1;
	}

	if (IS_ENABLED(CONFIG_NAND_RANDOMIZER)) {
		ret = detect_randomized_blks(XSPI_READ, 0);
		if (ret) {
			printf("detect_randomized_blks failed\n");
			return -1;
		}
		randomized = is_nand_block_randomized(offset);
		if (!randomized) {
			ret = mtd_read(mtd, offset, size, &rw_size, (void *)(uintptr_t)addr);
			if (ret || rw_size != size) {
				printf("read failed at 0x%x\n", offset);
				return ret;
			}
			return 0;
		}
		page_cnt = size / mtd->writesize;
		remain = size % mtd->writesize;
		if (remain > 0)
			page_cnt++;
		for (i = 0; i < page_cnt; i++) {
			ret = mtd_read(mtd, offset, mtd->writesize, &rw_size,
				       (void *)(uintptr_t)random_buf);
			if (ret || rw_size != mtd->writesize) {
				printf("read failed at 0x%x\n", offset);
				return ret;
			}

			if (i == page_cnt - 1 && remain > 0)
				buf = tmp_buff;
			else
				buf = (void *)(uintptr_t)addr;
			randomized_length =
			mv_nand_randomizer_randomize_page(offset,
							  random_buf, NULL, buf, NULL);
			if (randomized_length != mtd->writesize) {
				printf("randomizer failed at 0x%x\n", offset);
				printf("%s %d randomized_length=%u, expected=%u\n", __func__,
				       __LINE__, randomized_length, mtd->writesize);
				return -1;
			}
			if (i == page_cnt - 1 && remain > 0)
				memcpy((void *)(uintptr_t)addr, (void *)(uintptr_t)buf, remain);

			offset += mtd->writesize;
			addr += mtd->writesize;
		}
	} else {
		ret = mtd_read(mtd, offset, size, &rw_size, (void *)(uintptr_t)addr);
		if (ret || rw_size != size) {
			printf("read failed at 0x%x\n", offset);
			return ret;
		}
	}

	return 0;
}

int syna_spinand_write(u32 offset, u32 size, u32 addr)
{
	int ret = -1;
	size_t rw_size;
	struct mtd_info *mtd;
	int i = 0;
	u32 page_cnt = 0, remain = 0;
	u8 *buf = NULL;
	u32 randomized = 0;
	u32 randomized_length = 0;

	mtd = xspi_nand_init();
	if (!mtd) {
		printf("%s xspi nand init failed\n", __func__);
		return -1;
	}

	if (IS_ENABLED(CONFIG_NAND_RANDOMIZER)) {
		randomized = is_nand_block_randomized(offset);
		if (!randomized) {
			ret = mtd_write(mtd, offset, size, &rw_size, (void *)(uintptr_t)addr);
			if (ret || rw_size != size) {
				printf("write failed at 0x%x\n", offset);
				return ret;
			}
			return 0;
		}
		page_cnt = size / mtd->writesize;
		remain = size % mtd->writesize;
		if (remain > 0)
			page_cnt++;
		for (i = 0; i < page_cnt; i++) {
			if (i == page_cnt - 1 && remain > 0) {
				buf = tmp_buff;
				ret = mtd_read(mtd, offset, mtd->writesize, &rw_size,
					       (void *)(uintptr_t)random_buf);
				if (ret || rw_size != mtd->writesize) {
					printf("read failed for rmw at 0x%x\n", offset);
					return ret;
				}
				randomized_length =
				mv_nand_randomizer_randomize_page(offset,
								  random_buf, NULL, buf, NULL);
				if (randomized_length != mtd->writesize) {
					printf("randomizer failed at 0x%x\n", offset);
					printf("%s %d randomized_length=%u, expected=%u\n",
					       __func__, __LINE__,
					       randomized_length, mtd->writesize);
					return -1;
				}
				memcpy((void *)buf, (void *)(uintptr_t)addr, remain);
			} else {
				buf = (void *)(uintptr_t)addr;
			}
			randomized_length =
			mv_nand_randomizer_randomize_page(offset,
							  buf, NULL, random_buf, NULL);
			if (randomized_length != mtd->writesize) {
				printf("randomizer failed at 0x%x\n", offset);
				printf("%s %d randomized_length=%u, expected=%u\n", __func__,
				       __LINE__, randomized_length, mtd->writesize);
				return -1;
			}

			ret = mtd_write(mtd, offset, mtd->writesize, &rw_size,
					(void *)(uintptr_t)random_buf);
			if (ret || rw_size != mtd->writesize) {
				printf("write failed at 0x%x\n", offset);
				return ret;
			}

			offset += mtd->writesize;
			addr += mtd->writesize;
		}
	} else {
		ret = mtd_write(mtd, offset, size, &rw_size, (void *)(uintptr_t)addr);
		if (ret || rw_size != size) {
			printf("write failed at 0x%x\n", offset);
			return ret;
		}
	}

	return 0;
}

#define KERNEL_A_NAME           "boot_a"
#define KERNEL_B_NAME           "boot_b"
#define ROOTFS_A                "rootfs_a"
#define ROOTFS_B                "rootfs_b"
#define KERNEL_UBIFS_NAME       "boot/linux_bootimgs.subimg"

int mount_ubifs(const char *pt)
{
	char cmd[128];
	static char last_pt_ubifs[16];

	if (strcmp(pt, last_pt_ubifs) == 0)
		return 0;

	snprintf(cmd, sizeof(cmd), "ubi part %s", pt);
	if (run_command(cmd, 0))
		return -1;

	if (run_command("ubifsmount ubi0", 0))
		return -1;

	strcpy(last_pt_ubifs, pt);
	return 0;
}

int load_image_from_ubifs(const char *file, void *read_buffer, u32 read_bytes)
{
	char cmd[128];

	snprintf(cmd, sizeof(cmd), "ubifsload 0x%lx %s %x",
		 (uintptr_t)read_buffer, file, read_bytes);
	if (run_command(cmd, 0))
		return -1;

	return 0;
}

int spi_nand_image_read(const char *cmd, void *read_buffer, u32 read_bytes)
{
	const char *pt_ubifs = NULL;
	int ret = -1;

	if ((strcmp(cmd, KERNEL_A_NAME) == 0) || (strcmp(cmd, KERNEL_B_NAME) == 0)) {
		pt_ubifs = strcmp(cmd, KERNEL_A_NAME) == 0 ? ROOTFS_A : ROOTFS_B;
		if (mount_ubifs(pt_ubifs) == 0) {
			ret = load_image_from_ubifs(KERNEL_UBIFS_NAME,
						    read_buffer, read_bytes);
		} else {
			printf("Mount %s fail\n", pt_ubifs);
			run_command("reset", 0);
		}
	} else {
		printf("Unknown image name %s\n", cmd);
		run_command("reset", 0);
	}

	return ret;
}
