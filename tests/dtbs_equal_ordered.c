/*
 * libfdt - Flat Device Tree manipulation
 *	Tests if two given dtbs are structurally equal (including order)
 * Copyright (C) 2007 David Gibson, IBM Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <fdt.h>
#include <libfdt.h>

#include "tests.h"
#include "testdata.h"

void compare_mem_rsv(const void *fdt1, const void *fdt2)
{
	int i;
	uint64_t addr1, size1, addr2, size2;
	int err;

	if (fdt_num_mem_rsv(fdt1) != fdt_num_mem_rsv(fdt2))
		FAIL("Trees have different number of reserve entries");
	for (i = 0; i < fdt_num_mem_rsv(fdt1); i++) {
		err = fdt_get_mem_rsv(fdt1, i, &addr1, &size1);
		if (err)
			FAIL("fdt_get_mem_rsv(fdt1, %d, ...): %s", i,
			     fdt_strerror(err));
		err = fdt_get_mem_rsv(fdt2, i, &addr2, &size2);
		if (err)
			FAIL("fdt_get_mem_rsv(fdt2, %d, ...): %s", i,
			     fdt_strerror(err));
		if ((addr1 != addr2) || (size1 != size2))
			FAIL("Mismatch in reserve entry %d: "
			     "(0x%llx, 0x%llx) != (0x%llx, 0x%llx)", i,
			     addr1, size1, addr2, size2);
	}
}

void compare_structure(const void *fdt1, const void *fdt2)
{
	int nextoffset1 = 0, nextoffset2 = 0;
	int offset1, offset2;
	uint32_t tag1, tag2;
	const char *name1, *name2;
	int err;
	const struct fdt_property *prop1, *prop2;
	int len1, len2;

	while (1) {
		do {
			offset1 = nextoffset1;
			tag1 = fdt_next_tag(fdt1, offset1, &nextoffset1);
		} while (tag1 == FDT_NOP);
		do {
			offset2 = nextoffset2;
			tag2 = fdt_next_tag(fdt2, offset2, &nextoffset2);
		} while (tag2 == FDT_NOP);

		if (tag1 != tag2)
			FAIL("Tag mismatch (%d != %d) at (%d, %d)",
			     tag1, tag2, offset1, offset2);

		switch (tag1) {
		case FDT_BEGIN_NODE:
			name1 = fdt_get_name(fdt1, offset1, &err);
			if (!name1)
				FAIL("fdt_get_name(fdt1, %d, ..): %s",
				     offset1, fdt_strerror(err));
			name2 = fdt_get_name(fdt2, offset2, NULL);
			if (!name2)
				FAIL("fdt_get_name(fdt2, %d, ..): %s",
				     offset2, fdt_strerror(err));
			if (!streq(name1, name2))
			    FAIL("Name mismatch (\"%s\" != \"%s\") at (%d, %d)",
				 name1, name2, offset1, offset2);
			break;

		case FDT_PROP:
			prop1 = fdt_offset_ptr_typed(fdt1, offset1, prop1);
			if (!prop1)
				FAIL("Could get fdt1 property at %d", offset1);
			prop2 = fdt_offset_ptr_typed(fdt2, offset2, prop2);
			if (!prop2)
				FAIL("Could get fdt2 property at %d", offset2);

			name1 = fdt_string(fdt1, fdt32_to_cpu(prop1->nameoff));
			name2 = fdt_string(fdt2, fdt32_to_cpu(prop2->nameoff));
			if (!streq(name1, name2))
				FAIL("Property name mismatch \"%s\" != \"%s\" "
				     "at (%d, %d)", name1, name2, offset1, offset2);
			len1 = fdt32_to_cpu(prop1->len);
			len2 = fdt32_to_cpu(prop2->len);
			if (len1 != len2)
				FAIL("Property length mismatch %u != %u "
				     "at (%d, %d)", len1, len2, offset1, offset2);

			if (memcmp(prop1->data, prop2->data, len1) != 0)
				FAIL("Property value mismatch at (%d, %d)",
				     offset1, offset2);
			break;

		case FDT_END:
			return;
		}
	}
}

int main(int argc, char *argv[])
{
	void *fdt1, *fdt2;

	test_init(argc, argv);
	if (argc != 3)
		CONFIG("Usage: %s <dtb file> <dtb file>", argv[0]);
	fdt1 = load_blob(argv[1]);
	fdt2 = load_blob(argv[2]);

	compare_mem_rsv(fdt1, fdt2);
	compare_structure(fdt1, fdt2);

	PASS();
}