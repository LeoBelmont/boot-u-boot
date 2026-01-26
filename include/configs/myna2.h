/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2016~2023 Synaptics Incorporated. All rights reserved.
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

#define CONFIG_GICV2
#define GICD_BASE       0xf7901000
#define GICC_BASE       0xf7902000

#ifdef CONFIG_SYNA_RESCUE_MODE
#define CFG_EXTRA_ENV_SETTINGS \
	"upgrade_available=0\0" \
	"rescue_load=ext4load mmc 0:2 0x10000000 Image; "\
		     "ext4load mmc 0:2 0x17c00000 board.dtb; "\
		     "ext4load mmc 0:2 0x8c00000 initramfs.rootfs.cpio.gz\0" \
	"rescue_setup=setenv kernel_comp_addr_r 0x7c00000; "\
		      "setenv kernel_comp_size $filesize; "\
		      "setenv bootm_low 0x0; "\
		      "setenv bootm_size 0x10000000; "\
		      "setenv bootargs shell earlycon console=ttyS0,115200 rootwait rootfstype=ext4\0" \
	"rescue_exec=booti 0x10000000 0x8c00000:$filesize 0x17c00000\0" \
	"rescue_boot=echo \"*** RESCUE MODE TRIGGERED ***\"; "\
		     "run rescue_load; run rescue_setup; run rescue_exec\0"
#else
#define CFG_EXTRA_ENV_SETTINGS \
	"upgrade_available=0\0" \
	"altbootcmd=if test ${boot_slot} = 1; then bootslot set b; bootcount reset;bootcount reset; run bootcmd; else bootslot set a; bootcount reset; bootcount reset; run bootcmd; fi\0"
#endif
