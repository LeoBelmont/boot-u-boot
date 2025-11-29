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

#include <command.h>
#include <env.h>
#include <asm/io.h>
#include <vsprintf.h>
#include <linux/arm-smccc.h>

#define SYNA_SIP_SMC64_OTP_PROGRAM     0xC2000008
#define SYNA_SIP_SMC64_OTP_READ_OTP    0xC200000A
#define SYNA_SIP_SMC64_OTP_WRITE_OTP   0xC200000B
#define SYNA_SIP_SMC64_RD_PUBLIC_OTP   0xC200000C

enum OTP_FIELD_ID {
	/* ---- USER DATA Section ---- */
	OTP_USER_DATA_0		= 0,	/* 128 bytes = 1024 bits = 32 x uint32_t */
	OTP_USER_DATA_1,
	OTP_USER_DATA_2,
	OTP_USER_DATA_3,
	OTP_USER_DATA_4,
	OTP_USER_DATA_5,
	OTP_USER_DATA_6,
	OTP_USER_DATA_7,
	OTP_USER_DATA_8,
	OTP_USER_DATA_9,
	OTP_USER_DATA_10,
	OTP_USER_DATA_11,
	OTP_USER_DATA_12,
	OTP_USER_DATA_13,
	OTP_USER_DATA_14,
	OTP_USER_DATA_15,
	OTP_USER_DATA_16,
	OTP_USER_DATA_17,
	OTP_USER_DATA_18,
	OTP_USER_DATA_19,
	OTP_USER_DATA_20,
	OTP_USER_DATA_21,
	OTP_USER_DATA_22,
	OTP_USER_DATA_23,
	OTP_USER_DATA_24,
	OTP_USER_DATA_25,
	OTP_USER_DATA_26,
	OTP_USER_DATA_27,
	OTP_USER_DATA_28,
	OTP_USER_DATA_29,
	OTP_USER_DATA_30,
	OTP_USER_DATA_31,

	/* ---- OEM Ownership ---- */
	OTP_K0_OEM_HASH_0	= 100,	/* 256 bits = 8 x uint32_t */
	OTP_K0_OEM_HASH_1,
	OTP_K0_OEM_HASH_2,
	OTP_K0_OEM_HASH_3,
	OTP_K0_OEM_HASH_4,
	OTP_K0_OEM_HASH_5,
	OTP_K0_OEM_HASH_6,
	OTP_K0_OEM_HASH_7,

	OTP_AESK0_0,			/* 128 bits = 4 x uint32_t */
	OTP_AESK0_1,
	OTP_AESK0_2,
	OTP_AESK0_3,

	OTP_REE_SEGID,			/* 32 bits (1 x uint32_t) */
	OTP_REE_SECURITY_ENABLE,	/* 1 bit  (1 x uint32_t) */
	OTP_BOOT_SECURITY_ENABLE,	/* 1 bit  (1 x uint32_t) */
	OTP_MP_PROVISION_DONE,		/* 1 bit  (1 x uint32_t) */
	OTP_SCS_AREA_SIZE_SEL,		/* 2 bits (1 x uint32_t) */
	OTP_REE_JTAG_PROTECTION_POLICY, /* 2 bits (1 x uint32_t) */

	/* ---- Synaptics Ownership ---- */
	OTP_EMMC_BOOT_DISABLE	= 200,	/* 1 bit  (1 x uint32_t) */
	OTP_SPI_BOOT_DISABLE,		/* 1 bit  (1 x uint32_t) */
	OTP_DOLBY_AUDIO_DISABLE,	/* 1 bit  (1 x uint32_t) */
	OTP_OEM_AUDIO_CUSTOMER_ID,	/* 32 bits (1 x uint32_t) */

	OTP_FIELD_MAX
};

/* ---- Lookup Table ---- */
struct otp_field_entry  {
	int value;
	const char *name;
};

static const struct otp_field_entry  otp_field_table[] = {
#define X(v) {v, #v}
	/* USER DATA Section */
	X(OTP_USER_DATA_0),  X(OTP_USER_DATA_1),  X(OTP_USER_DATA_2),  X(OTP_USER_DATA_3),
	X(OTP_USER_DATA_4),  X(OTP_USER_DATA_5),  X(OTP_USER_DATA_6),  X(OTP_USER_DATA_7),
	X(OTP_USER_DATA_8),  X(OTP_USER_DATA_9),  X(OTP_USER_DATA_10), X(OTP_USER_DATA_11),
	X(OTP_USER_DATA_12), X(OTP_USER_DATA_13), X(OTP_USER_DATA_14), X(OTP_USER_DATA_15),
	X(OTP_USER_DATA_16), X(OTP_USER_DATA_17), X(OTP_USER_DATA_18), X(OTP_USER_DATA_19),
	X(OTP_USER_DATA_20), X(OTP_USER_DATA_21), X(OTP_USER_DATA_22), X(OTP_USER_DATA_23),
	X(OTP_USER_DATA_24), X(OTP_USER_DATA_25), X(OTP_USER_DATA_26), X(OTP_USER_DATA_27),
	X(OTP_USER_DATA_28), X(OTP_USER_DATA_29), X(OTP_USER_DATA_30), X(OTP_USER_DATA_31),

	/* OEM Ownership */
	X(OTP_K0_OEM_HASH_0), X(OTP_K0_OEM_HASH_1), X(OTP_K0_OEM_HASH_2), X(OTP_K0_OEM_HASH_3),
	X(OTP_K0_OEM_HASH_4), X(OTP_K0_OEM_HASH_5), X(OTP_K0_OEM_HASH_6), X(OTP_K0_OEM_HASH_7),
	X(OTP_AESK0_0), X(OTP_AESK0_1), X(OTP_AESK0_2), X(OTP_AESK0_3),
	X(OTP_REE_SEGID), X(OTP_REE_SECURITY_ENABLE), X(OTP_BOOT_SECURITY_ENABLE),
	X(OTP_MP_PROVISION_DONE), X(OTP_SCS_AREA_SIZE_SEL), X(OTP_REE_JTAG_PROTECTION_POLICY),

	/* Synaptics Ownership */
	X(OTP_EMMC_BOOT_DISABLE), X(OTP_SPI_BOOT_DISABLE), X(OTP_DOLBY_AUDIO_DISABLE),
	X(OTP_OEM_AUDIO_CUSTOMER_ID),

	X(OTP_FIELD_MAX)
#undef X
};

#define PUBLIC_OTP_SIZE 256

struct PUBLIC_OTP_ITEMS {
	unsigned int   TA_Vendor_SegID[6];
	unsigned char  TA_Vendor_GID[6];
	unsigned char  Security_Indicators;
	unsigned int   BOOT_SegID;
	unsigned char  Lock_Boot_SegID;
	unsigned char  MP_Provision_Done;
	unsigned char  REE_Security_Enable;
	unsigned char  TEE_Segmentation_Version_Enable;
	unsigned char  Boot_Security_Enable;
	unsigned int   TEE_SegID;
	unsigned char  SCS_Total_Area_Size_Sel;
	unsigned char  Background_Check_Enable;
	unsigned char  JtagProg_Disable;
	unsigned int   REE_SegID;
	unsigned int   ConcurrencyControl;
	unsigned char  v_PubOtpMinConfVer;
	unsigned char  v_PubOtpRsaIndex;
	unsigned short v_PubOtpVID;
	unsigned short v_PubOtpOID;
	unsigned char  OtpWrtPwdProt;
	unsigned char  JTAG_Protection;
	unsigned char  JTAG_Protection_dup;
	unsigned char  BSCAN_Protection;
	unsigned char  BSCAN_Protection_dup;
	unsigned char  STEST_Protection;
	unsigned char  STEST_Protection_dup;
};

static void printPublicOtp(const struct PUBLIC_OTP_ITEMS *otp)
{
	int i;

	printf("TA_Vendor_SegID: ");
	for (i = 0; i < 6; i++)
		printf("0x%08X ", otp->TA_Vendor_SegID[i]);

	printf("\n");

	printf("TA_Vendor_GID: ");
	for (i = 0; i < 6; i++)
		printf("0x%02X ", otp->TA_Vendor_GID[i]);

	printf("\n");

	printf("Security_Indicators: 0x%02X\n", otp->Security_Indicators);
	printf("BOOT_SegID: 0x%08X\n", otp->BOOT_SegID);
	printf("Lock_Boot_SegID: 0x%02X\n", otp->Lock_Boot_SegID);
	printf("MP_Provision_Done: 0x%02X\n", otp->MP_Provision_Done);
	printf("REE_Security_Enable: 0x%02X\n", otp->REE_Security_Enable);
	printf("TEE_Segmentation_Version_Enable: 0x%02X\n", otp->TEE_Segmentation_Version_Enable);
	printf("Boot_Security_Enable: 0x%02X\n", otp->Boot_Security_Enable);
	printf("TEE_SegID: 0x%08X\n", otp->TEE_SegID);
	printf("SCS_Total_Area_Size_Sel: 0x%02X\n", otp->SCS_Total_Area_Size_Sel);
	printf("Background_Check_Enable: 0x%02X\n", otp->Background_Check_Enable);
	printf("JtagProg_Disable: 0x%02X\n", otp->JtagProg_Disable);
	printf("REE_SegID: 0x%08X\n", otp->REE_SegID);
	printf("ConcurrencyControl: 0x%08X\n", otp->ConcurrencyControl);
	printf("v_PubOtpMinConfVer: 0x%02X\n", otp->v_PubOtpMinConfVer);
	printf("v_PubOtpRsaIndex: 0x%02X\n", otp->v_PubOtpRsaIndex);
	printf("v_PubOtpVID: 0x%04X\n", otp->v_PubOtpVID);
	printf("v_PubOtpOID: 0x%04X\n", otp->v_PubOtpOID);
	printf("OtpWrtPwdProt: 0x%02X\n", otp->OtpWrtPwdProt);
	printf("JTAG_Protection: 0x%02X\n", otp->JTAG_Protection);
	printf("JTAG_Protection_dup: 0x%02X\n", otp->JTAG_Protection_dup);
	printf("BSCAN_Protection: 0x%02X\n", otp->BSCAN_Protection);
	printf("BSCAN_Protection_dup: 0x%02X\n", otp->BSCAN_Protection_dup);
	printf("STEST_Protection: 0x%02X\n", otp->STEST_Protection);
	printf("STEST_Protection_dup: 0x%02X\n", otp->STEST_Protection_dup);
}

static int do_otp_read_public(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	struct arm_smccc_res res;
	unsigned char tmp_buf[PUBLIC_OTP_SIZE] = {0};
	unsigned char *shared_buf = tmp_buf;
	struct PUBLIC_OTP_ITEMS pubOtp;
	unsigned char read_byte = 0;

	memset(tmp_buf, 0, PUBLIC_OTP_SIZE);

	flush_dcache_range((uintptr_t)tmp_buf, (uintptr_t)tmp_buf + sizeof(tmp_buf));

	printf("Address of &tmp_buf[0]: %p,  phys: %p\n", (void *)tmp_buf,
	       (void *)virt_to_phys(tmp_buf));

	arm_smccc_smc(SYNA_SIP_SMC64_RD_PUBLIC_OTP, virt_to_phys(tmp_buf),
		      PUBLIC_OTP_SIZE, 0, 0, 0, 0, 0, &res);

	invalidate_dcache_range((uintptr_t)tmp_buf, (uintptr_t)tmp_buf + sizeof(tmp_buf));

	printf("otp operation %s\n", res.a0 == 0 ? "succeed" : "failed");

	if (res.a0 != 0) {
		printf("ret a0 = 0x%lx, ret a1 = 0x%lx, ret a2 = 0x%lx, ret a3 = 0x%lx\n",
		       res.a0, res.a1, res.a2, res.a3);
	}

	/* Gen2 row 64~66, shared_buf[8] */
	memcpy((unsigned char *)pubOtp.TA_Vendor_SegID, shared_buf + 8, 24);

	/* Gen2 row 74, shared_buf[32] */
	memcpy((unsigned char *)pubOtp.TA_Vendor_GID, shared_buf + 32, 8);

	/* Gen2 row 96 byte 2, shared_buf[40 + 2] */
	memcpy((unsigned char *)&pubOtp.Security_Indicators, shared_buf + 40 + 2, 1);

	/* Gen3 row 7 byte 4~7, shared_buf[56] */
	memcpy((unsigned char *)&pubOtp.BOOT_SegID, shared_buf + 56 + 4, 4);

	/* Gen3 row 8 byte 4 bit 4~7, shared_buf[64] */
	read_byte = (*(unsigned char *)(shared_buf + 64 + 4) & 0xf0) >> 4;
	memcpy((unsigned char *)&pubOtp.Lock_Boot_SegID, &read_byte, 1);

	/* Gen3 row 8 byte 7 bit 2, shared_buf[64] */
	read_byte = (*(unsigned char *)(shared_buf + 64 + 7) & 0x04) >> 2;
	memcpy((unsigned char *)&pubOtp.MP_Provision_Done, &read_byte, 1);

	/* Gen3 row 9 byte 4 bit 3, shared_buf[72] */
	read_byte = (*(unsigned char *)(shared_buf + 72 + 4) & 0x08) >> 3;
	memcpy((unsigned char *)&pubOtp.REE_Security_Enable, &read_byte, 1);

	/* Gen3 row 9 byte 4 bit 2, shared_buf[72] */
	read_byte = (*(unsigned char *)(shared_buf + 72 + 4) & 0x04) >> 2;
	memcpy((unsigned char *)&pubOtp.TEE_Segmentation_Version_Enable, &read_byte, 1);

	/* Gen3 row 9 byte 7 bit 0, shared_buf[72] */
	read_byte = *(unsigned char *)(shared_buf + 72 + 7) & 0x01;
	memcpy((unsigned char *)&pubOtp.Boot_Security_Enable, &read_byte, 1);

	/* Gen3 row 13 byte 4~7, shared_buf[80] */
	memcpy((unsigned char *)&pubOtp.TEE_SegID, shared_buf + 80 + 4, 4);

	/* Gen3 row 27 byte 0 bit 6~7, shared_buf[88] */
	read_byte = (*(unsigned char *)(shared_buf + 88) & 0xC0) >> 6;
	memcpy((unsigned char *)&pubOtp.SCS_Total_Area_Size_Sel, &read_byte, 1);

	/* Gen3 row 27 byte 1 bit 2, shared_buf[88 + 1] */
	read_byte = (*(unsigned char *)(shared_buf + 88 + 1) & 0x04) >> 2;
	memcpy((unsigned char *)&pubOtp.Background_Check_Enable, &read_byte, 1);

	/* Gen3 row 29 byte 2 bit 4, shared_buf[96 + 2] */
	read_byte = (*(unsigned char *)(shared_buf + 96 + 2) & 0x10) >> 4;
	memcpy((unsigned char *)&pubOtp.JtagProg_Disable, &read_byte, 1);

	/* Gen3 row 29 byte 4~7, shared_buf[96 + 4] */
	memcpy((unsigned char *)&pubOtp.REE_SegID, shared_buf + 96 + 4, 4);

	/* CAS4 row 19 byte 0~3, shared_buf[104] */
	memcpy((unsigned char *)&pubOtp.ConcurrencyControl, shared_buf + 104, 4);

	/* CAS4 row 21 byte 2 bit 2~7, shared_buf[112 + 2] */
	read_byte = (*(unsigned char *)(shared_buf + 112 + 2) & 0xfc) >> 2;
	memcpy((unsigned char *)&pubOtp.v_PubOtpMinConfVer, &read_byte, 1);

	/* CAS4 row 21 byte 3 bit 4~7, shared_buf[112 + 3] */
	read_byte = (*(unsigned char *)(shared_buf + 112 + 3) & 0xf0) >> 4;
	memcpy((unsigned char *)&pubOtp.v_PubOtpRsaIndex, &read_byte, 1);

	/* CAS4 row 21 byte 4~5, shared_buf[112 + 4] */
	memcpy((unsigned char *)&pubOtp.v_PubOtpVID, shared_buf + 112 + 4, 2);

	/* CAS4 row 21 byte 6~7, shared_buf[112 + 6] */
	memcpy((unsigned char *)&pubOtp.v_PubOtpOID, shared_buf + 112 + 6, 2);

	/* CAS4 row 26 byte 0 bit 4, shared_buf[120] */
	read_byte = (*(unsigned char *)(shared_buf + 120) & 0x10) >> 4;
	memcpy((unsigned char *)&pubOtp.OtpWrtPwdProt, &read_byte, 1);

	/* CAS4 row 26 byte 1 bit 0~1, shared_buf[120 + 1] */
	read_byte = *(unsigned char *)(shared_buf + 120 + 1) & 0x03;
	memcpy((unsigned char *)&pubOtp.JTAG_Protection, &read_byte, 1);

	/* CAS4 row 26 byte 1 bit 2~3, shared_buf[120 + 1] */
	read_byte = (*(unsigned char *)(shared_buf + 120 + 1) & 0x0c) >> 2;
	memcpy((unsigned char *)&pubOtp.JTAG_Protection_dup, &read_byte, 1);

	/* CAS4 row 26 byte 2 bit 0~1, shared_buf[120 + 2] */
	read_byte = *(unsigned char *)(shared_buf + 120 + 2) & 0x03;
	memcpy((unsigned char *)&pubOtp.BSCAN_Protection, &read_byte, 1);

	/* CAS4 row 26 byte 2 bit 2~3, shared_buf[120 + 2] */
	read_byte = (*(unsigned char *)(shared_buf + 120 + 2) & 0x0c) >> 2;
	memcpy((unsigned char *)&pubOtp.BSCAN_Protection_dup, &read_byte, 1);

	/* CAS4 row 26 byte 3 bit 0~1, shared_buf[120 + 3] */
	read_byte = *(unsigned char *)(shared_buf + 120 + 3) & 0x03;
	memcpy((unsigned char *)&pubOtp.STEST_Protection, &read_byte, 1);

	/* CAS4 row 26 byte 3 bit 2~3, shared_buf[120 + 3] */
	read_byte = (*(unsigned char *)(shared_buf + 120 + 3) & 0x0c) >> 2;
	memcpy((unsigned char *)&pubOtp.STEST_Protection_dup, &read_byte, 1);

	printPublicOtp(&pubOtp);

	return res.a0 == 0 ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

#define UNUSED(x) ((void)(x))

/* ---- Dump Implementation ---- */
static int do_otp_idx_dump(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	UNUSED(cmdtp);
	UNUSED(flag);
	UNUSED(argc);
	UNUSED(argv);

	printf("%-5s %-40s\n", "Idx", "Name");
	printf("-----------------------------------------------\n");
	for (size_t i = 0; i < ARRAY_SIZE(otp_field_table); ++i)
		printf("%-5d %s\n", otp_field_table[i].value, otp_field_table[i].name);

	return CMD_RET_SUCCESS;
}

static int do_otp(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	u32 data_addr, data_size;
	struct arm_smccc_res res;

	if (argc < 4)
		return -1;

	data_addr = simple_strtoull(argv[2], NULL, 16);
	data_size = simple_strtoull(argv[3], NULL, 16);

	flush_dcache_all();

	debug("program otp data at 0x%08x, size=0x%x\n", data_addr, data_size);

	arm_smccc_smc(SYNA_SIP_SMC64_OTP_PROGRAM, data_addr, data_size, 0, 0, 0, 0, 0, &res);

	printf("otp operation %s\n", res.a0 == 0 ? "succeed" : "failed");

	if (res.a0 != 0)
		printf("ret = 0x%08lx\n", res.a0);

	return res.a0 == 0 ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

static int do_otp_read(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	u32 index;
	u32 p_phy_data = 0; /* phys addr of data */
	u32 p_phy_mask = 0; /* phys addr of mask */
	struct arm_smccc_res res;
	u32 otp_shm_buff[8]; /* shared memory buffer for smc call */

	if (argc < 2)
		return -1;

	if (strncmp(argv[1], "0x", 2) == 0 || strncmp(argv[1], "0X", 2) == 0)
		index = simple_strtoul(argv[1], NULL, 16);
	else
		index = simple_strtoul(argv[1], NULL, 10);

	memset((void *)otp_shm_buff, 0, sizeof(otp_shm_buff));

	flush_dcache_range((uintptr_t)otp_shm_buff, (uintptr_t)otp_shm_buff + sizeof(otp_shm_buff));

	p_phy_data = virt_to_phys(&otp_shm_buff[0]);
	p_phy_mask = virt_to_phys(&otp_shm_buff[1]);

	arm_smccc_smc(SYNA_SIP_SMC64_OTP_READ_OTP, index, p_phy_data, p_phy_mask, 0, 0, 0, 0, &res);

	invalidate_dcache_range((uintptr_t)otp_shm_buff,
				(uintptr_t)otp_shm_buff + sizeof(otp_shm_buff));

	printf("otp operation %s\n", res.a0 == 0 ? "succeed" : "failed");

	if (res.a0 != 0)
		printf("%s: ret = 0x%08lx\n", __func__, res.a0);

	printf("read otp[%d] data=0x%08x, mask=0x%08x\n", index, otp_shm_buff[0], otp_shm_buff[1]);

	return res.a0 == 0 ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

static int do_otp_write(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	u32 index;
	u32 data, mask;
	struct arm_smccc_res res;

	if (argc < 4)
		return -1;

	if (strncmp(argv[1], "0x", 2) == 0 || strncmp(argv[1], "0X", 2) == 0)
		index = simple_strtoul(argv[1], NULL, 16);
	else
		index = simple_strtoul(argv[1], NULL, 10);

	data = simple_strtoull(argv[2], NULL, 16);
	mask = simple_strtoull(argv[3], NULL, 16);

	flush_dcache_all();

	debug("program otp[%d] data=0x%08x, mask=0x%08x\n", index, data, mask);

	arm_smccc_smc(SYNA_SIP_SMC64_OTP_WRITE_OTP, index, data, mask, 0, 0, 0, 0, &res);

	printf("otp operation %s\n", res.a0 == 0 ? "succeed" : "failed");

	if (res.a0 != 0)
		printf("ret = 0x%08lx\n", res.a0);

	return res.a0 == 0 ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

static char otp_help[] =
	"Program OTP data from memory.";

static char otp_usage[] =
	"\n"
	"examples:\n"
	"     otp write 0x7000000 0x500 to program OTP data(size 0x500) at addr 0x7000000\n"
	;

static char otp_pub_help[] =
	"\n"
	"Read OTP public items\n"
	;

static char otp_pub_usage[] =
	"\n"
	"examples:\n"
	"    otp_pub\n"
	;

static char otp_read_help[] =
	"Read OTP data by otp_index.\n"
	"    Use otp_idx_dump to see all otp_index values.\n";

static char otp_read_usage[] =
	"\n"
	"examples:\n"
	"   otpread <otp_index>\n"
	"   otpread   1\n"
	"   otpread   2\n\n"
	"   Use otp_idx_dump to see all otp_index values.\n"
	"           return: 0      : Success\n"
	"               0xFF000001 : STATUS_FAILURE\n"
	"               0xFF000050 : STATUS_OTP_INVALID_IDX\n"
	"               0xFF000052 : STATUS_OTP_ERROR_WRITEONLY_FIELD"
	;

static char otp_write_help[] =
	"Program OTP data by otp_index.\n"
	"   Use otp_idx_dump to see all otp_index values.\n";

static char otp_write_usage[] =
	"\n"
	" examples:\n"
	"     otpwrite <otp_index> <data> <mask>\n"
	"     otpwrite   1 0x500 0xFFF\n"
	"     otpwrite   2 0x1A2B3C4D 0xFFFFFFFF\n"
	"     otpwrite   3 0x1A2B0000 0xFFFF0000\n"
	"     otpwrite   4 0x00010000 0x00010000\n\n"
	"              return: 0          : Success\n"
	"                      0xFF000001 : STATUS_FAILURE\n"
	"                      0xFF000050 : STATUS_OTP_INVALID_IDX\n"
	"                      0xFF000051 : STATUS_OTP_ERROR_READONLY_FIELD\n\n"
	"     Use otp_idx_dump to see all otp_index values.\n"
	;

static char otp_idx_dump_help[] =
	"Dump all OTP field indices and names.";

static char otp_idx_dump_usage[] =
	"\n"
	"Examples:\n"
	"     otp_idx_dump\n";

U_BOOT_CMD(otp, 4, 0, do_otp, otp_help, otp_usage);
U_BOOT_CMD(otp_pub, 1, 0, do_otp_read_public, otp_pub_help, otp_pub_usage);
U_BOOT_CMD(otpread, 2, 0, do_otp_read, otp_read_help, otp_read_usage);
U_BOOT_CMD(otpwrite, 4, 0, do_otp_write, otp_write_help, otp_write_usage);
U_BOOT_CMD(otp_idx_dump, 1, 0, do_otp_idx_dump, otp_idx_dump_help, otp_idx_dump_usage);
