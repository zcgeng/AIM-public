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

#ifndef __ASM_H
#define __ASM_H
#include <sys/types.h>

static inline void
insl(int port, void *addr, int cnt)
{
  asm volatile("cld; rep insl" :
               "=D" (addr), "=c" (cnt) :
               "d" (port), "0" (addr), "1" (cnt) :
               "memory", "cc");
}

static inline void
outw(uint16_t port, uint16_t data)
{
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
}

static inline void
outsl(int port, const void *addr, int cnt)
{
  asm volatile("cld; rep outsl" :
               "=S" (addr), "=c" (cnt) :
               "d" (port), "0" (addr), "1" (cnt) :
               "cc");
}

static inline void
stosb(void *addr, int data, int cnt)
{
  asm volatile("cld; rep stosb" :
               "=D" (addr), "=c" (cnt) :
               "0" (addr), "1" (cnt), "a" (data) :
               "memory", "cc");
}

static inline void
stosl(void *addr, int data, int cnt)
{
  asm volatile("cld; rep stosl" :
               "=D" (addr), "=c" (cnt) :
               "0" (addr), "1" (cnt), "a" (data) :
               "memory", "cc");
}

struct gatedesc;

static inline void
lidt(struct gatedesc *p, int size)
{
  volatile uint16_t pd[3];

  pd[0] = size-1;
  pd[1] = (uint32_t)p;
  pd[2] = (uint32_t)p >> 16;

  asm volatile("lidt (%0)" : : "r" (pd));
}

static inline void
ltr(uint16_t sel)
{
  asm volatile("ltr %0" : : "r" (sel));
}

static inline uint32_t
readeflags(void)
{
  uint32_t eflags;
  asm volatile("pushfl; popl %0;" : "=r" (eflags));
  return eflags;
}

static inline void
seteflags(uint32_t eflags)
{
  asm volatile("pushl %0; popfl;" :: "r" (eflags));
}

static inline void
loadgs(uint16_t v)
{
  asm volatile("movw %0, %%gs" : : "r" (v));
}

static inline void
cli(void)
{
  asm volatile("cli");
}

static inline void
sti(void)
{
  asm volatile("sti");
}

static inline uint32_t
xchg(volatile uint32_t *addr, uint32_t newval)
{
  uint32_t result;

  // The + in "+m" denotes a read-modify-write operand.
  asm volatile("lock; xchgl %0, %1" :
               "+m" (*addr), "=a" (result) :
               "1" (newval) :
               "cc");
  return result;
}

static inline uint32_t
rcr2(void)
{
  uint32_t val;
  asm volatile("movl %%cr2,%0" : "=r" (val));
  return val;
}

static inline void
lcr3(uint32_t val)
{
  asm volatile("movl %0,%%cr3" : : "r" (val));
}

//PAGEBREAK: 36

static inline
uint8_t inb(uint16_t port)
{
	uint8_t data;
	asm volatile("in %1,%0" : "=a" (data) : "d" (port));
	return data;
}

static inline
uint16_t inw(uint16_t port)
{
	uint16_t data;
	asm volatile("in %1, %0" : "=a"(data) : "d"(port));
	return data;
}

static inline
uint32_t inl(uint16_t port)
{
	uint32_t data;
	asm volatile("in %1, %0" : "=a"(data) : "d"(port));
	return data;
}

static inline
void outb(uint16_t port, uint8_t data)
{
	asm volatile("out %0,%1" : : "a" (data), "d" (port));
}

static inline
void outl(uint16_t port, uint8_t data)
{
	asm volatile("out %%eax, %%dx" : : "a"(data), "d"(port));
}

/* modify the value of IDTR */
static inline void
write_idtr(void *addr, uint32_t size) {
	static volatile uint16_t data[3];
	data[0] = size - 1;
	data[1] = (uint32_t)addr;
	data[2] = ((uint32_t)addr) >> 16;
	asm volatile("lidt (%0)" : : "r"(data));
}

struct segdesc;
static inline void
lgdt(struct segdesc *p, int size)
{
  volatile uint16_t pd[3];

  pd[0] = size-1;
  pd[1] = (uint32_t)p;
  pd[2] = (uint32_t)p >> 16;

  asm volatile("lgdt (%0)" : : "r" (pd));
}

__noreturn static inline
void jmp(void *addr){
	asm(
		"mov %0, %%eax;"
		"jmp *%%eax;"
		::"m"(addr)
	);
	for(;;);
}

#endif
