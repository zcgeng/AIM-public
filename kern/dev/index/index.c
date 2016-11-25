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

/*
 * This file serves as a wrapper for the device index
 */

#include <sys/types.h>
#include <aim/device.h>
#include <aim/panic.h>
#include <aim/console.h>
#include <aim/initcalls.h>

#include <libc/string.h>

/*
 * Some dummy implementations could return failures if interfaces
 * are called before implementations are ready. This eliminates some NULL
 * pointer dereferences.
 */

static int __add(struct device *dev) { return EOF; }
static int __remove(struct device *dev) { return EOF; }
static struct device *__next(struct device *dev, void **savep) { return NULL; }
static struct device *__from_id(dev_t devno) { return NULL; }
static struct device *__from_name(char *name) { return NULL; }

/* This should not be accessed in low address, safe for a compile-time init */
static struct device_index __index = {
	.add		= __add,
	.remove		= __remove,
	.next		= __next,
	.from_id	= __from_id,
	.from_name	= __from_name
};
extern uint32_t early_init_start;
extern uint32_t early_init_end;
extern uint32_t norm_init_start;
extern uint32_t norm_init_end;

int do_early_initcalls(){
	uint32_t start = (uint32_t)&early_init_start;
	uint32_t end = (uint32_t)&early_init_end;
	uint32_t i;
	for(i = start; i < end; i += 4){
		kpdebug("start doing early init calls at 0x%8x\n", i);
		initcall_t fp = (initcall_t)i;
		if(fp() != 0){
			return EOF;
		}
	}
	return 0;
}

int do_initcalls(){
	uint32_t *start = &norm_init_start;
	uint32_t *end = &norm_init_end;
	uint32_t *i;
	for(i = start; i < end; i ++){
		kpdebug("start doing norm init calls at 0x%8x\n", (uint32_t)i);
		initcall_t fp = (initcall_t)(*i);
		kpdebug("function address: 0x%8x\n", (uint32_t) fp);
		if(fp() != 0){
			return EOF;
		}
	}
	return 0;
}
//
// void initdev(struct device *dev, int class, const char *devname, dev_t devno,struct driver *drv){
// 	dev->class = class;
// 	memcpy(dev->name, devname, DEV_NAME_MAX);
// 	dev->devno = devno;
// 	dev->driver = *drv;
// }
//
// void register_driver(unsigned int major, struct driver *drv){
// 	kpdebug("implement me!\n");
// }

void set_device_index(struct device_index *index)
{
	memcpy(&__index, index, sizeof(*index));
}

int dev_add(struct device *dev)
{
	return __index.add(dev);
}

int dev_remove(struct device *dev)
{
	return __index.remove(dev);
}

struct device *dev_next(struct device *dev, void **savep)
{
	return __index.next(dev, savep);
}

struct device *dev_from_id(dev_t devno)
{
	return __index.from_id(devno);
}

struct device *dev_from_name(char *name)
{
	return __index.from_name(name);
}
