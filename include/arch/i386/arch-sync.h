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

#ifndef _ARCH_SYNC_H
#define _ARCH_SYNC_H

#ifndef __ASSEMBLER__
#include <aim/proc.h>
#include <asm.h>
#include <aim/panic.h>
// Mutual exclusion lock.
typedef struct spinlock {
  uint locked;       // Is the lock held?

  // For debugging:
  //char *name;        // Name of lock.
	struct cpu *cpu;   // The cpu holding the lock.
  uint pcs[10];      // The call stack (an array of program counters)
                     // that locked the lock.
}lock_t;

#define EMPTY_LOCK(lock)	(UNLOCKED)

static inline
void spinlock_init(lock_t *lock)
{
	lock->locked = 0;
	lock->cpu = 0;
}

static inline
void spin_lock(lock_t *lock)
{
	cli(); // disable interrupts to avoid deadlock.
  if(spin_unlock(lock))
    panic("already acquired");

  // The xchg is atomic.
  while(xchg(&lock->locked, 1) != 0)
    ;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen after the lock is acquired.
  __sync_synchronize();

  // Record info about lock acquisition for debugging.
  //lock->cpu = cpu;
  //getcallerpcs(&lock, lock->pcs);
}

static inline
void spin_unlock(lock_t *lock)
{
	if(!spin_is_locked(lock))
    panic("release non-holding lock");

  lock->pcs[0] = 0;
  lock->cpu = 0;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other cores before the lock is released.
  // Both the C compiler and the hardware may re-order loads and
  // stores; __sync_synchronize() tells them both not to.
  __sync_synchronize();

  // Release the lock, equivalent to lk->locked = 0.
  // This code can't use a C assignment, since it might
  // not be atomic. A real OS would use C atomics here.
  asm volatile("movl $0, %0" : "+m" (lk->locked) : );

  cli();
}

static inline
bool spin_is_locked(lock_t *lock)
{
	return lock->locked;
}

/* Semaphore */
typedef struct {
	int val;
	int limit;
} semaphore_t;

static inline
void semaphore_init(semaphore_t *sem, int val)
{
}

static inline
void semaphore_dec(semaphore_t *sem)
{
}

static inline
void semaphore_inc(semaphore_t *sem)
{
}

#endif /* __ASSEMBLER__ */

#endif /* _ARCH_SYNC_H */
