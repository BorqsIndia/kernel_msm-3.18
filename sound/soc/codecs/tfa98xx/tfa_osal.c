/*
 * Copyright (c) 2015 NXP Semiconductors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "config.h"

#if (defined(WIN32) || defined(_X64))
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "tfa_internal.h"

void *kmalloc(size_t size, gfpt_t flags)
{
	/* flags are not used outside the Linux kernel */
	(void)flags;

#if !defined(__REDLIB__)
	return malloc(size);
#else
	/* TODO !need malloc here */
#endif
}

void kfree(const void *ptr)
{
#if !defined(__REDLIB__)
	free((void *)ptr);
#else
	/* TODO !need free here */
#endif
}

unsigned long msleep_interruptible(unsigned int msecs)
{
#if (defined(WIN32) || defined(_X64))
	Sleep(msecs);
#else
	usleep(1000 * msecs);
#endif
	return 0;
}
