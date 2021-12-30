/** \file   vsyncarch.c
 * \brief   End-of-frame handling for native GTK3 UI
 *
 * \note    This is altered and trimmed down to fit into the GTK3-native
 *          world, but it's still heavily reliant on UNIX internals.
 *
 * \author  Dag Lem <resid@nimrod.no>
 */

/*
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include "kbdbuf.h"
#include "mainlock.h"
#include "ui.h"
#include "vsyncapi.h"
#include "videoarch.h"

#include "joystick.h"

#ifdef WIN32_COMPILE
#   include "windows.h"
#elif defined(HAVE_NANOSLEEP)
#   include <time.h>
#else
#   include <unistd.h>
#   include <errno.h>
#   include <sys/time.h>
#endif

#ifdef MACOSX_SUPPORT
#   include <mach/mach.h>
#   include <mach/mach_time.h>
#endif

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

static int pause_pending = 0;

/* ------------------------------------------------------------------------- */

void vsyncarch_presync(void)
{
    ui_update_lightpen();
    joystick();
}

void vsyncarch_postsync(void)
{
    /* this function is called once a frame, so this
       handles single frame advance */
    if (pause_pending) {
        ui_pause_enable();
        pause_pending = 0;
    }
}

void vsyncarch_advance_frame(void)
{
    ui_pause_disable();
    pause_pending = 1;
}
