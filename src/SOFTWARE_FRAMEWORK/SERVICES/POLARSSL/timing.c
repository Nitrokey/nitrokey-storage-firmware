/* This source file is part of the ATMEL AVR-UC3-SoftwareFramework-1.7.0 Release */

/*
 *  Portable interface to the CPU cycle counter
 *
 *  Copyright (C) 2006-2009, Paul Bakker <polarssl_maintainer at polarssl.org>
 *  All rights reserved.
 *
 *  Joined copyright on original XySSL code with: Christophe Devine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * This file contains modifications done by Rudolf Boeddeker
 * For the modifications applies:
 *
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2012-08-18
 *
 * Nitrokey  is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Nitrokey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nitrokey. If not, see <http://www.gnu.org/licenses/>.
 */


#include "polarssl/config.h"

#include "polarssl/timing.h"

#include "global.h"
#include "tools.h"
#include "TIME_MEASURING.h"

// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/time.h>
// #include <signal.h>
#include <time.h>

#if defined(__AVR32__) || defined(__ICCAVR32__)
#include <avr32/io.h>
#include "cycle_counter.h"
#endif

#if defined(POLARSSL_TIMING_C)




#if defined(WIN32)

#include <windows.h>
#include <winbase.h>

struct _hr_time
{
    LARGE_INTEGER start;
};

#else



// struct _hr_time
// {
// struct timeval start;
// };

#endif


#if defined(POLARSSL_HAVE_ASM) && 					\
	(defined(_MSC_VER) && defined(_M_IX86)) || defined(__WATCOMC__)

unsigned long hardclock (void)
{
    unsigned long tsc;
    __asm rdtsc __asm mov[tsc], eax return (tsc);
}

#else
#if defined(__ICCAVR32__) || defined(__AVR32__)

unsigned long hardclock (void)
{
    return Get_sys_count ();
}
#else
#if defined(POLARSSL_HAVE_ASM) && defined(__GNUC__) && defined(__i386__)

unsigned long hardclock (void)
{
    unsigned long tsc;
  asm ("rdtsc":"=a" (tsc));
    return (tsc);
}

#else
#if defined(POLARSSL_HAVE_ASM) && defined(__GNUC__) && 			\
	(defined(__amd64__) || defined(__x86_64__))

unsigned long hardclock (void)
{
    unsigned long lo, hi;
  asm ("rdtsc":"=a" (lo), "=d" (hi));
    return (lo | (hi << 32));
}

#else
#if defined(POLARSSL_HAVE_ASM) && defined(__GNUC__) && 			\
	(defined(__powerpc__) || defined(__ppc__))

unsigned long hardclock (void)
{
    unsigned long tbl, tbu0, tbu1;

    do
    {
      asm ("mftbu %0":"=r" (tbu0));
      asm ("mftb  %0":"=r" (tbl));
      asm ("mftbu %0":"=r" (tbu1));
    }
    while (tbu0 != tbu1);

    return (tbl);
}

#else
#if defined(POLARSSL_HAVE_ASM) && defined(__GNUC__) && defined(__sparc__)

unsigned long hardclock (void)
{
    unsigned long tick;
    asm (".byte 0x83, 0x41, 0x00, 0x00");
  asm ("mov   %%g1, %0":"=r" (tick));
    return (tick);
}

#else
#if defined(POLARSSL_HAVE_ASM) && defined(__GNUC__) && defined(__alpha__)

unsigned long hardclock (void)
{
    unsigned long cc;
  asm ("rpcc %0":"=r" (cc));
    return (cc & 0xFFFFFFFF);
}

#else
#if defined(POLARSSL_HAVE_ASM) && defined(__GNUC__) && defined(__ia64__)

unsigned long hardclock (void)
{
    unsigned long itc;
  asm ("mov %0 = ar.itc":"=r" (itc));
    return (itc);
}

#else

static int hardclock_init = 0;
static struct timeval tv_init;

unsigned long hardclock (void)
{
    struct timeval tv_cur;

    if (hardclock_init == 0)
    {
        gettimeofday (&tv_init, NULL);
        hardclock_init = 1;
    }

    gettimeofday (&tv_cur, NULL);
    return ((tv_cur.tv_sec - tv_init.tv_sec) * 1000000 + (tv_cur.tv_usec - tv_init.tv_usec));
}

#endif /* generic */
#endif /* IA-64 */
#endif /* Alpha */
#endif /* SPARC8 */
#endif /* PowerPC */
#endif /* AMD64 */
#endif /* i586+ */
#endif /* AVR32 */

volatile int alarmed = 0;

#if defined(WIN32)

unsigned long get_timer (struct hr_time* val, int reset)
{
    unsigned long delta;
    LARGE_INTEGER offset, hfreq;
    struct _hr_time* t = (struct _hr_time *) val;

    QueryPerformanceCounter (&offset);
    QueryPerformanceFrequency (&hfreq);

    delta = (unsigned long) ((1000 * (offset.QuadPart - t->start.QuadPart)) / hfreq.QuadPart);

    if (reset)
        QueryPerformanceCounter (&t->start);

    return (delta);
}

DWORD WINAPI TimerProc (LPVOID uElapse)
{
    Sleep ((DWORD) uElapse);
    alarmed = 1;
    return (TRUE);
}

void set_alarm (int seconds)
{
    DWORD ThreadId;

    alarmed = 0;
    CloseHandle (CreateThread (NULL, 0, TimerProc, (LPVOID) (seconds * 1000), 0, &ThreadId));
}

void m_sleep (int milliseconds)
{
    Sleep (milliseconds);
}

#else

// unsigned long get_timer( struct hr_time *val, int reset )
// {
// unsigned long delta;
// struct timeval offset;
// struct _hr_time *t = (struct _hr_time *) val;
//
// gettimeofday( &offset, NULL );
//
// delta = ( offset.tv_sec - t->start.tv_sec ) * 1000
// + ( offset.tv_usec - t->start.tv_usec ) / 1000;
//
// if( reset )
// {
// t->start.tv_sec = offset.tv_sec;
// t->start.tv_usec = offset.tv_usec;
// }
//
// return( delta );
// }
#if defined(__ICCAVR32__) || defined(__AVR32__)

#ifndef HZ
#define HZ (unsigned long)AVR32_PM_CPU_MAX_FREQ
#endif


t_cpu_time timer;

u32 RealTimeOffsetInSec_u32 = 0;    // Offset from cpu clock time to real time

time_t time (time_t * timer)
{
    if (timer != NULL)
    {
        *timer = (time_t) (TIME_MEASURING_GetTime () / (u64) TIME_MEASURING_TICKS_IN_USEC / 1000000LL);
        *timer += (time_t) RealTimeOffsetInSec_u32;
        // *timer=cpu_cy_2_us(Get_sys_count(), HZ)/1000000;
    }
    return 1;
}



void set_alarm (int seconds)
{
    cpu_set_timeout (seconds * 1000, &timer);
}

void m_sleep (int milliseconds)
{
    cpu_delay_ms (milliseconds, HZ);
}

#else
static void sighandler (int signum)
{
    alarmed = 1;
    signal (signum, sighandler);
}

void set_alarm (int seconds)
{
    alarmed = 0;
    signal (SIGALRM, sighandler);
    alarm (seconds);
}

void m_sleep (int milliseconds)
{
    struct timeval tv;

    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = milliseconds * 1000;

    select (0, NULL, NULL, NULL, &tv);
}

#endif

#endif
#endif

void set_time (time_t TimeInSec)
{
time_t time;

    // Get local cpu time
    time = (time_t) (TIME_MEASURING_GetTime () / (u64) TIME_MEASURING_TICKS_IN_USEC / 1000000LL);

    RealTimeOffsetInSec_u32 = TimeInSec - time; // Get offset to real time
}
