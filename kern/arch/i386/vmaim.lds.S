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
/*
 * Using the C preprocessor, we allow includes and macro expansion in this
 * linker script.
 */

ENTRY(_start)
SECTIONS
{
	. = KERNBASE + 0x100000; /* high address as 运行地址 */
        .text : AT(0x100000) /* physical address as 存储地址 */
        {
          	*(.text);
        }
        .data :
	{
		*(.data);
		
	}                                    
        .bss :
        {
		PROVIDE(_bss_start = .);
		*(.bss);
		PROVIDE(_bss_end = .);
	}                      
        _end = .;
}
