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

/** Randomize the data in NAND.
 *
 * At present, it only supports SAMSUNG NAND randomization way at present.
 *
 * Some key points.
 * 1. Don't randomize bad block marker.
 * 2. Seed can use randomized data.
 *
 * Important things for modifications, for the follow things would change
 * the random data, and it would make the data won't be binary compatible.
 * 1. Initial Seed.
 * 2. Page start positions.
 * 3. Randomize or unrandomized some special bytes, such as bad block marker.
 *
 * @warning: This file is used in other places, so don't inlcude any
 *           kernel related files to it.
 *
 * Author: Yongsen Chen, YongsenChen@gmail.com
 */

#include <linux/types.h>
#include <console.h>

#ifndef ASSERT
#   define ASSERT(x)                /* {if (!(x))  while (1);} */
#endif /* ASSERT */

#define DO_TRACE_LOG                (0)

#if DO_TRACE_LOG
#define TRACE_LOG               printf
#else
#define TRACE_LOG(...)
#endif

/* We use random seed to make it work for all chip that requires randomization.
 */
#define RANDOM_SEED

/*
 * INCLUDES
 */
#include "prbs.h"
#include "nand_randomizer.h"

/*
 * CONSTANTS
 */
#ifndef TRUE
#define TRUE				(1)
#endif

#ifndef FALSE
#define FALSE				(0)
#endif

#ifndef NULL
#define NULL				(0)
#endif

#define NAND_ID_MAX_SIZE                (8)
#define NAND_DEFAULT_OOB_SIZE		(32)

#define IS_POWER_OF_2(x) \
	({ typeof(x) _x = (x); !(_x & (_x - 1)); })
#define ARRAYSIZE(a)                    (sizeof(a) / sizeof(*(a)))

#define PAGE_START_POS_ALIGNED_BYTES    (4)
#define PAGE_START_POS_MASK(cycle_len)	                                \
	((cycle_len) - 1) & (~(PAGE_START_POS_ALIGNED_BYTES - 1))

/** NAND randomizer types.
 *
 * We only support SAMSUNG PRBS-15 at present.
 * @sa SAMSUNG Recommendation for a randomizer v0.1
 */
enum mv_nand_randomizer_type_e {
	MV_NAND_RANDOMIZER_FROM_MEMORY,     /* Use the input buffer directly */
	MV_NAND_RANDOMIZER_SAMSUNG_PRBS15,  /* Generate by PRBS-15 with seeds
					     * defined by SAMSUNG.
					     */

	MV_NAND_RANDOMIZER_TYPE_MAX
};

/*
 * TYPES
 */

/** NAND Randomizer chip information.
 */
struct nand_randomized_chip_info_s {
	unsigned int block_size;
	unsigned int page_size;
	unsigned int spare_size;
	enum mv_nand_randomizer_type_e randomizer_type;
	unsigned int randomizer_buffer_length;
};

/* Samsung randomizer.
 */
#ifndef RANDOM_SEED
static const unsigned short g_nand_randomozer_seed_start_pos_table_samsung[] = {
	   0,   32, 1020, 2564, 2664, 3408, 1300, 3048,
	 848,  332,  860,  804,  800, 3816, 3064, 3096,
	1136, 3208,  304, 3264,  500, 3944, 2348, 1260,
	3884,  516, 1280, 3980,  176, 2552, 2584,  648,
	1832, 1928,  656,  224, 1848,  340, 2544,  988,
	2468,  292,  224, 2968, 3000, 1168, 2000,   36,
	 792, 1972, 3684,  928, 4052, 2752, 1016,  440,
	2412,  176, 2184, 2216, 1824, 2536, 2456, 2992,
	1848, 2120,  184, 2484, 2220, 1324, 2056, 3472,
	3544, 3576, 3600,  556, 3428, 2936,  336, 2284,
	 804, 3620, 3140, 1660, 2888, 3476,   44, 1816,
	1848,  408, 3864, 1592, 2896,  452, 3564, 1916,
	3300, 1852,  172,  964,  804,  296,  328, 1052,
	2168, 2180,  356, 4064, 2096, 1228, 2484, 2204,
	1996, 1192,  984, 1052, 3356, 3388, 2368, 3260,
	2308, 1920, 1304, 2732, 3744, 3112, 3256,  732
};

#define NAND_RANDOMIZER_SEED_COUNT_SAMSUNG                                  \
	ARRAYSIZE(g_nand_randomozer_seed_start_pos_table_samsung)

#endif /* RANDOM_SEED */

#define NAND_RANDOMIZER_SEED_SAMSUNG                (0x576A)

/* Global randomizer. */
struct mv_nand_randomizer_s g_nand_randomizer;

/*
 * PRIVATE FUNCTIONS
 */

#define DUMP_8_BYTES(name, b) \
	do { \
		const unsigned char *_b = (b); \
		if (_b) { \
			TRACE_LOG("%s(0x%08X): %02X %02X %02X %02X %02X %02X %02X %02X\n", \
			name, (int)_b, _b[0], _b[1], _b[2], _b[3], _b[4], _b[5], _b[6], _b[7]); \
		} \
	} while (0)

/** Get shift of x.
 */
static unsigned int get_shift(unsigned int x)
{
	unsigned int s = 0;

	ASSERT(x > 0 && IS_POWER_OF_2(x));

	for (s = 0; s < sizeof(x) * 8; s++)
		if ((unsigned int)(1 << s) == x)
			break;
	return s;
}

/** Get the random data by PRBS generator
 *
 */
static unsigned int gen_prbs15_random_data_samsung(unsigned char *p_random_data_buffer,
						   unsigned int   random_data_length)
{
	unsigned short seed = NAND_RANDOMIZER_SEED_SAMSUNG;

	ASSERT(p_random_data_buffer);
	ASSERT(random_data_length);

	prbs15_gen(PRBS_POLYNOMIAL_DEFAULT,
		   seed,
		   p_random_data_buffer,
		   random_data_length,
		   FALSE);
	return random_data_length;
}

/** Randomize data by XOR way.
 *
 * For each byte, *p_dest = *p_random_data ^ *p_src.
 *
 * @param p_random_data             Random data.
 * @param p_src                     Source data.
 * @param p_dst                     Buffer to save the randomized data.
 * @param length                    Length of the data to randomize.
 *
 * @return void
 *
 * @sa mv_nand_randomizer_init(), mv_nand_randomizer_get_page().
 */
static void randomize_by_xor(const unsigned char *p_random_data,
			     const unsigned char *p_src,
			     unsigned char       *p_dst,
			     int                  length)
{
	int i;
	const int aligned_mask = sizeof(unsigned int) - 1;

	ASSERT(p_random_data);
	ASSERT(p_src);
	ASSERT(p_dst);
	ASSERT(length > 0);

	/* if it's 4-byte aligned, then we use a faster way to do so            */
	if ((0 == ((int)(uint64_t)p_random_data & aligned_mask)) &&
	    (0 == ((int)(uint64_t)p_src & aligned_mask)) &&
	    (0 == ((int)(uint64_t)p_dst & aligned_mask)) &&
	    (0 == (length & aligned_mask))) {
		const unsigned int *p_random_data_32 = (const unsigned int *)p_random_data;
		const unsigned int *p_src_32 = (const unsigned int *)p_src;
		unsigned int *p_dst_32 = (unsigned int *)p_dst;
		int length_32 = length / sizeof(*p_random_data_32);

		for (i = 0; i < length_32; i++)
			p_dst_32[i] = p_src_32[i] ^ p_random_data_32[i];
	} else {
		for (i = 0; i < length; i++)
			p_dst[i] = p_src[i] ^ p_random_data[i];
	}
}

/** It will randomize the data in ring.
 *
 * @param p_random_data             Random data.
 * @param random_data_length        Length of the random data.
 * @param p_src                     Source data.
 * @param p_dst                     Buffer to save the randomized data.
 * @param length                    Length of the data to randomize, must not
 *                                  be larger than a page_random_data_length.
 * @param start                     Start position in the page to randomize.
 *
 * @return void
 *
 * @sa randomize_by_xor()
 */
static void randomize_by_xor_ring(const unsigned char *p_random_data,
				  unsigned int         random_data_length,
				  const unsigned char *p_src,
				  unsigned char       *p_dst,
				  unsigned int         length,
				  unsigned int         start)
{
	unsigned int randomized_length = 0;
	unsigned int offset = start;

	/* make sure offset is in a page */
	while (offset >= random_data_length)
		offset -= random_data_length;

	while (randomized_length < length) {
		unsigned int processing_length = random_data_length - offset;

		if (processing_length > length - randomized_length)
			processing_length = length - randomized_length;

		if (offset != 0)
			DUMP_8_BYTES("RANDOM ", p_random_data + offset);

		randomize_by_xor(p_random_data + offset,
				 p_src + randomized_length,
				 p_dst + randomized_length,
				 processing_length);
		offset = 0;
		randomized_length += processing_length;
	}
}

/** Check whether a block is randomized.
 *
 * @param block_index               Block index to check.
 *
 * @retval 0                        Chip is not randomized.
 * @retval !0                       Chip is randomized.
 *
 * @sa
 */
int __weak mv_nand_block_randomized(unsigned int block_index)
{
	/* Modify it to support other blocks */
	const unsigned int unrandomized_block_table[] = {
		/* block 0    */0,
		/* bootloader */1, 2, 3, 4, 5, 6, 7, 8,
		/* bootloader */9, 10, 11, 12, 13, 14, 15, 16
	};
	unsigned int i;
	int randomized = TRUE;

	for (i = 0; i < ARRAYSIZE(unrandomized_block_table); i++) {
		if (block_index == unrandomized_block_table[i]) {
			randomized = FALSE;
			break;
		}
	}
	return randomized;
}

/** Check whether the address is randomized  */
int mv_nand_addr_randomized(unsigned int addr)
{
	const struct mv_nand_randomizer_s *p_nr = &g_nand_randomizer;
	unsigned int block_index = addr >> p_nr->block_shift;

	return mv_nand_block_randomized(block_index);
}

/** Get the start random data pos for a page.
 *
 * @param page_addr                 Address of a page.
 *
 * @return unsigned int             Start pos in random data for the page.
 *
 * @sa
 */
static unsigned int mv_nand_get_page_start(unsigned int page_addr)
{
	struct mv_nand_randomizer_s *p_nr = &g_nand_randomizer;
	unsigned int page_index_in_block = (page_addr & p_nr->page_mask) / p_nr->page_size;

#ifdef RANDOM_SEED
	unsigned int start;
	/* 2 for each pos need 2 bytes
	 * we don't use pos = page_addr * 2; because LEAPimg need repeat the seed
	 * in each block, for the which block is bad is unexpected.
	 */
	unsigned int pos = page_index_in_block * 2;

	pos = pos & (p_nr->random_data_buffer_length - 1);

	start = (p_nr->p_random_data_buffer[pos + 1] << 8) +
		p_nr->p_random_data_buffer[pos];

	start &= PAGE_START_POS_MASK(p_nr->random_data_buffer_length);
	/* & p_nr->random_data_buffer_length is to make sure it won't exceed random
	 * data length.
	 * & (~0x3) is to make sure it's 32-bit aligned
	 */

	return start;
#else /* !RANDOM_SEED */
	while (page_index_in_block >= NAND_RANDOMIZER_SEED_COUNT_SAMSUNG)
		page_index_in_block -= NAND_RANDOMIZER_SEED_COUNT_SAMSUNG;

	return g_nand_randomozer_seed_start_pos_table_samsung[page_index_in_block];
#endif /* RANDOM_SEED */
}

/** The function to do real randomization.
 *
 * It will be called by all randomization functions.
 *
 * @param page_addr                 Address of the page (start from 0).
 * @param p_src                     Source data.
 * @param p_dst                     Buffer to save the randomized data.
 * @param length                    Length of the data to randomize, must not
 *                                  be larger than a page_random_data_length.
 * @param offset_in_page            Start position in the page to randomize.
 *
 * @return void
 *
 * @sa
 */
static void mv_nand_do_randomize(unsigned int         page_addr,
				 const unsigned char *p_src,
				 unsigned char       *p_dst,
				 unsigned int         length,
				 unsigned int         offset_in_page)
{
	struct mv_nand_randomizer_s *p_nr = &g_nand_randomizer;

	int start = mv_nand_get_page_start(page_addr);

	randomize_by_xor_ring(p_nr->p_random_data_buffer,
			      p_nr->random_data_buffer_length,
			      p_src,
			      p_dst,
			      length,
				 start + offset_in_page);
}

/*
 * PUBLIC FUNCTIONS
 */

int mv_nand_randomizer_init(unsigned int         block_size,
			    unsigned int         page_size,
			    unsigned int         oob_size,
			    unsigned char       *p_randomizer_buffer,
			    unsigned int         randomizer_buffer_length)
{
	struct mv_nand_randomizer_s *p_nr = &g_nand_randomizer;

	ASSERT(IS_POWER_OF_2(page_size));
	ASSERT(IS_POWER_OF_2(block_size));

	if (!p_nr->chip_randomized)
		printf("Nand Randomizer is enabled!\n");

	TRACE_LOG("%s(block_size=%d, page_size=%d, oob_size=%d, buf_len=%d)\n",
		  __func__,
		  block_size, page_size, oob_size, randomizer_buffer_length);

	p_nr->p_random_data_buffer      = p_randomizer_buffer;

	/* we initialize randomizer ASAP, to make sure the randomizer can be initialized
	 * as unrandomized first.
	 */
	/*memset(p_nr, 0, sizeof(*p_nr));*/
	p_nr->chip_randomized           = FALSE;

	p_nr->block_size                = block_size;
	p_nr->page_size                 = page_size;
	p_nr->oob_size                  = oob_size;

	/* calculate the masks and shifts */
	p_nr->page_per_block = block_size / page_size;
	p_nr->page_mask = p_nr->block_size - 1;
	p_nr->block_shift = get_shift(p_nr->block_size);

	gen_prbs15_random_data_samsung(p_randomizer_buffer, randomizer_buffer_length);

	ASSERT(IS_POWER_OF_2(randomizer_buffer_length));
	p_nr->chip_randomized = TRUE;
	p_nr->random_data_buffer_length = randomizer_buffer_length;
	return randomizer_buffer_length;
}

unsigned int mv_nand_randomizer_randomize(unsigned int         page_addr,
					  const unsigned char *p_src,
					  unsigned char       *p_dst,
					  unsigned int         length,
					  unsigned int         offset_in_page)
{
	const struct mv_nand_randomizer_s *p_nr = &g_nand_randomizer;
	unsigned int block_index = page_addr >> p_nr->block_shift;

	int need_set_bad_marker = FALSE;
	unsigned char bad_marker = 0;
	unsigned int bad_marker_pos = 0;

	ASSERT(p_src);
	ASSERT(p_dst);
	ASSERT(length > 0);

	if (!p_nr->chip_randomized || !mv_nand_block_randomized(block_index))
		return 0;

	if (offset_in_page <= p_nr->page_size &&
	    (offset_in_page + length) > p_nr->page_size) {
		need_set_bad_marker = TRUE;
		bad_marker_pos = p_nr->page_size - offset_in_page;
		bad_marker = p_src[bad_marker_pos];
	}

	mv_nand_do_randomize(page_addr, p_src, p_dst, length, offset_in_page);

	if (need_set_bad_marker)
		p_dst[bad_marker_pos] = bad_marker;

	return length;
}

unsigned int mv_nand_randomizer_randomize_page(unsigned int         page_addr,
					       const unsigned char *p_data_src,
					       const unsigned char *p_oob_src,
					       unsigned char       *p_data_dst,
					       unsigned char       *p_oob_dst)
{
	const struct mv_nand_randomizer_s *p_nr = &g_nand_randomizer;
	unsigned int block_index = page_addr >> p_nr->block_shift;
#if DO_TRACE_LOG
	unsigned int page_index = (page_addr & p_nr->page_mask) / p_nr->page_size;
#endif /* DO_TRACE_LOG */
	unsigned int randomized_length = 0;

	if (!p_nr->chip_randomized || !mv_nand_block_randomized(block_index)) {
		TRACE_LOG("nand_randomize_page(0x%08X): UNRONDOMIZED\n", page_addr);
		return 0;
	}

	TRACE_LOG("%s(0x%08X): block %d, page %d, !!! RANDOMIZED !!!.\n",
		  __func__, page_addr, block_index, page_index);

	if (p_data_src) {
		/* randomize page data  */
		DUMP_8_BYTES("DATA_SRC", p_data_src);
		mv_nand_do_randomize(page_addr, p_data_src, p_data_dst, p_nr->page_size, 0);
		randomized_length += p_nr->page_size;
		DUMP_8_BYTES("DATA_DST", p_data_dst);
	}

	//if oob data is null or oob_size is zero, ignore this step
	if (p_oob_src && p_nr->oob_size) {
		unsigned char bad_marker = p_oob_src[0];
		/* randomize oob data   */
		DUMP_8_BYTES("OOB_SRC", p_oob_src);

		mv_nand_do_randomize(page_addr, p_oob_src, p_oob_dst,
				     p_nr->oob_size, p_nr->page_size);
		/* we don't randomize the bad block marker  */
		p_oob_dst[0] = bad_marker;
		randomized_length += p_nr->oob_size;
		DUMP_8_BYTES("OOB_DST", p_oob_dst);
	}

	return randomized_length;
}
