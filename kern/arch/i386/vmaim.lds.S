/* Copyright (C) 2016 David Gao <davidgao1001@gmail.com>
 *
 * This file is part of AIM.
 *
 * AIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#define __LDSCRIPT__
#include <aim/memlayout.h>
#include <asm-generic/vmaim.lds.h>
/*
 * Using the C preprocessor, we allow includes and macro expansion in this
 * linker script.
 */

ENTRY(_start)
SECTIONS
{
	. = KERN_LINK; /* high address as 运行地址 */
	.text : AT(KERN_START) /* physical address as 存储地址 */
	{
		TEXT;
	}
	.data :
	{
		DATA(8);

	}
	.bss :
	{
		PROVIDE(_bss_start = .);
		BSS(8);
		. = ALIGN(4096);
		PROVIDE(kstack = .);
		. += KSTACKSIZE;
		PROVIDE(kstack_top = .);
		PROVIDE(_bss_end = .);
	}
	.initcalls :
	{
    . += 0x10; /* Get away from the dangerous stack */
		EARLY_INIT(8);
		NORM_INIT(8);
		LATE_INIT(8);
	}
	_end = .;
}
