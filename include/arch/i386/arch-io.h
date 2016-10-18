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

#ifndef _ARCH_IO_H
#define _ARCH_IO_H

#ifndef __ASSEMBLER__

/*
 * We use macros here so that, when involved in lab assignments, students
 * do not have to implement them all to compile the whole project.
 * In this way, only invoked routines need to be implemented.
 */

#define in8(port)	inb(port)
#define in16(port)	inw(port)
#define in32(port)	inl(port)

#define out8(port, data)	outb(port, data)
#define out16(port, data)	outw(port, data)
#define out32(port, data)	outl(port, data)

#endif /* !__ASSEMBLER__ */

#endif /* _ARCH_IO_H */
