// SPDX-License-Identifier: GPL-2.0+
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
#include <linux/types.h>
#include <dm.h>
#include <fdtdec.h>
#include <libfdt.h>
#include <string.h>
#include "misc_syna.h"
#include "fastboot_syna.h"
#include <command.h>
#include <malloc.h>
#include <env.h>
#include <env_callback.h>
#include <fs.h>

#define FDTO_SIZE 0x2000
#define FDT_MAX_SIZE 0x8000  /* Max size to increase FDT into - 32KB is usually enough */
#define BASE_DTB_WORKING_MEMORY	0x10000000 /* Memory for overlay'd DTB - hopefully safe !?!*/
#define MAX_BUF_SIZE		512
#define MAX_MMCPART_STR_SIZE	16

#define DEFAULT_PANEL_DTBO_PATH "/boot"

#define ROOTFS_A			"rootfs_a"
#define ROOTFS_B			"rootfs_b"

/* Helper function to check if a DTBO filename is valid */
static bool is_valid_panel_dtbo(const char *filename)
{
	const char *valid_names[] = {"panel", "bridge"};
	int i;

	for (i = 0; i < ARRAY_SIZE(valid_names); i++) {
		if (strstr(filename, valid_names[i]))
			return true;
	}
	return false;
}

/* Helper function to prepare FDT for overlay operations */
static int prepare_fdt_overlay(const void *blob, void *new_fdt)
{
	int ret = fdt_open_into(blob, new_fdt, FDT_MAX_SIZE);

	if (ret)
		printf("Failed to resize FDT: %s\n", fdt_strerror(ret));

	return ret;
}

/* Helper function to get dtbo path */
static void get_dtbo_path(char *dtbo_path)
{
	char *path = DEFAULT_PANEL_DTBO_PATH;

	/* Use configured DTBO path if available */
	if (CONFIG_PANEL_DTBO_PATH[0] != '\0')
		path = CONFIG_PANEL_DTBO_PATH;

	strcpy(dtbo_path, path);
}

/* Helper function to get mmc part in the format mmc_dev:part_index */
static void get_mmc_part(char *mmc_part)
{
	int part_index;
	int mmc_dev = get_mmc_active_dev();

	/* Determine partition index based on current slot */
	part_index = (get_current_slot() == 0) ?
		     f_mmc_get_part_index(mmc_dev, ROOTFS_A) :
		     f_mmc_get_part_index(mmc_dev, ROOTFS_B);

	sprintf(mmc_part, "%x:%x", mmc_dev, part_index);
}

/* Helper function to load and apply a single DTBO */
static int load_and_apply_dtbo(const char *dtbo_name, char *path,
			       char *mmc_part, void *fdto_addr,
			       void *new_fdt)
{
	char cmd[MAX_BUF_SIZE];
	int ret;

	/* Load DTBO from storage */
	sprintf(cmd, "ext4load mmc %s %p %s/%s",
		mmc_part, fdto_addr, path, dtbo_name);

	ret = run_command(cmd, 0);
	if (ret) {
		debug("Unable to load DTBO [%s]\n", dtbo_name);
		return ret;
	}

	/* Apply overlay */
	ret = fdt_overlay_apply(new_fdt, fdto_addr);
	if (ret) {
		printf("ERROR: Failed to overlay [%s]: %s\n",
		       dtbo_name, fdt_strerror(ret));
		return ret;
	}

	debug("DTBO [%s] overlay success!\n", dtbo_name);
	return 0;
}

/* Process custom DTBO list from environment variable */
static int process_custom_dtbos(const char *dtbo_env, char *path,
				char *mmc_part, void *fdto_addr,
				void *new_fdt)
{
	char *copy, *tok, atleast_one_overlay_success = 0;
	int ret = -1; /* Assume failure initially */

	copy = strdup(dtbo_env);
	if (!copy) {
		printf("Failed to allocate memory for DTBO list\n");
		return -ENOMEM;
	}

	tok = strtok(copy, " ,");
	if (!tok) {
		debug("No DTBO parameters found\n");
		goto cleanup;
	}

	/* Process each DTBO in the list */
	while (tok) {
		if (is_valid_panel_dtbo(tok)) {
			ret = load_and_apply_dtbo(tok, path, mmc_part,
						  fdto_addr, new_fdt);
			if (ret == 0) {
				/* Success - at least one overlay applied */
				atleast_one_overlay_success = 1;
			}
		}
		tok = strtok(NULL, " ,");
	}

	/* return success if at least one overlay applied */
	if (atleast_one_overlay_success)
		ret = 0;

cleanup:
	free(copy);
	return ret;
}

/**
 * setup_uboot_fdt_overlay - Setup and apply device tree overlays
 *
 * This function handles loading and applying device tree blob overlays (DTBOs)
 * for panel configuration. It supports both custom DTBO lists from environment
 * variables and fallback to default configuration.
 *
 * Return: 0 on success, negative error code on failure
 */
int setup_uboot_fdt_overlay(void)
{
	void *fdto_addr = NULL;
	char *dtbo_env;
	int ret = -1;
	const void *blob = gd->fdt_blob;
	void *new_fdt = (void *)BASE_DTB_WORKING_MEMORY;
	bool fdt_prepared = false;
	char path[MAX_BUF_SIZE];
	char mmc_part[MAX_MMCPART_STR_SIZE];

	/* Allocate memory for DTBO loading */
	fdto_addr = malloc(FDTO_SIZE);
	if (!fdto_addr) {
		printf("Failed to allocate DTBO memory\n");
		return -ENOMEM;
	}

	get_dtbo_path(path);
	get_mmc_part(mmc_part);

	/* Try to process custom DTBOs from environment */
	dtbo_env = env_get("dtbo");
	if (dtbo_env && dtbo_env[0] != '\0') {
		/* Prepare FDT for overlay operations */
		ret = prepare_fdt_overlay(blob, new_fdt);
		if (ret)
			goto cleanup;
		fdt_prepared = true;

		/* Process custom DTBO list */
		ret = process_custom_dtbos(dtbo_env, path, mmc_part,
					   fdto_addr, new_fdt);
	}

	/* Fall back to default DTBO if custom processing failed */
	if (ret != 0) {
#ifdef CONFIG_DEFAULT_PANEL_DTBO
		if (CONFIG_DEFAULT_PANEL_DTBO[0] != '\0') {
			/* Prepare FDT if not already done */
			if (!fdt_prepared) {
				ret = prepare_fdt_overlay(blob, new_fdt);
				if (ret)
					goto cleanup;
			}

			/* Load and apply default DTBO */
			ret = load_and_apply_dtbo(CONFIG_DEFAULT_PANEL_DTBO,
						  path, mmc_part, fdto_addr, new_fdt);
		}
#endif
	}

	/* Update global FDT blob on success */
	if (ret == 0) {
		gd->fdt_blob = new_fdt;
		debug("FDT overlay setup completed successfully\n");
	} else {
		printf("FDT overlay setup failed, using default configuration\n");
	}

cleanup:
	free(fdto_addr);
	return ret;
}

extern void set_fastlogo_status(bool status);

/* Helper function to check if the path/dtbo file exists
 * Returns 0 if the dtbo file found at the path
 * Returns -ENOENT if file not found
 */
static int is_file_exists(const char *dtbo_env, char *path, char *mmc_part)
{
	char filename[MAX_BUF_SIZE];

	if (fs_set_blk_dev("mmc", mmc_part, FS_TYPE_EXT)) {
		printf("%s: mmc_part: <%s> invalid\n", __func__, mmc_part);
		return -ENODEV;
	}

	sprintf(filename, "%s/%s", path, dtbo_env);

	/* Returns 1 if present, 0 if not */
	return fs_exists(filename) ? 0 : -ENOENT;
}

/* Tokenize the DTBO env list and verify if all individual dtbo files exist */
static int verify_dtbo_env(const char *dtbo_env, char *path, char *mmc_part)
{
	char *copy, *tok;

	/* Empty dtbo env is valid */
	if (!dtbo_env)
		return 0;

	if (!path || !mmc_part) {
		printf("%s: Path or mmc_part invalid\n", __func__);
		return -1;
	}

	copy = strdup(dtbo_env);
	if (!copy) {
		printf("Failed to allocate memory for DTBO list\n");
		return -ENOMEM;
	}

	tok = strtok(copy, " ,");
	/* Process each DTBO token in the list */
	while (tok) {
		/* Check if this token has corresponding dtbo in filesystem */
		int ret = is_file_exists(tok, path, mmc_part);

		if (ret < 0) {
			printf("File <%s> doesn't exist at path <%s>: %d\n", tok, path, ret);
			return ret;
		}

		tok = strtok(NULL, " ,");
	}

	/* All dtbo tokens available, return success*/
	return 0;
}

/*
 * on_dtbo: Function callback invoked when user runs 'setenv dtbo'
 *
 * This function fetch the configure dtbo path and mmc part, verifies if
 * the user configured dtbo env variable is valid.
 * If valid, set fastlogo status to 0, let Linux re-initialize the display
 * If invalid return -EINVAL, user setting fails, old value retained
 */
static int on_dtbo(const char *name, const char *value, enum env_op op, int flags)
{
	char path[MAX_BUF_SIZE];
	char mmc_part[MAX_MMCPART_STR_SIZE];

	get_dtbo_path(path);
	get_mmc_part(mmc_part);

	/* Validate the parameter set by user */
	if (verify_dtbo_env(value, path, mmc_part) < 0)
		return -EINVAL;

	/* Set fastlogo status to 0 so that Linux re-initializes the display*/
	set_fastlogo_status(0);

	return 0;
}

U_BOOT_ENV_CALLBACK(dtbo, on_dtbo);
