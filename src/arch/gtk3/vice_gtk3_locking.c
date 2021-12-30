/** \file   vice_gtk3_locking.c
 * \brief   GTK3 signal handling helper functions
 *
 * \author  David Hogan <david.q.hogan@gmail.com>
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
 */

#include "vice.h"

#include <stdlib.h>

#include "vice_gtk3_locking.h"
#include "mainlock.h"

/*
 * Obtains the vice lock before the signal handler is called.
 */
static void vice_gtk3_lock(gpointer data, GClosure *closure)
{
    /* printf("locking: %s\n", (const char *)data); fflush(stdout); */
    mainlock_obtain();
}

/*
 * Releases the vice lock after the signal handler is called.
 */
static void vice_gtk3_unlock(gpointer data, GClosure *closure)
{
    /* printf("unlocking: %s\n", (const char *)data); */
    mainlock_release();
}

/*
 * A replacement for g_signal_connect that wraps the signal handling callback in vice lock/unlock.
 */
gulong vice_locking_g_signal_connect(gpointer instance, const gchar *detailed_signal, GCallback signal_handler, gpointer data, const char *signal_handler_name)
{
    GClosure *closure = g_cclosure_new(signal_handler, data, NULL);

    /* Wrap invocation of signal_handler with lock & unlock. */
    g_closure_add_marshal_guards(closure, (gpointer)signal_handler_name, vice_gtk3_lock, (gpointer)signal_handler_name, vice_gtk3_unlock);

    return g_signal_connect_closure(instance, detailed_signal, closure, FALSE);
}

/*
 * A replacement for gtk_accel_group_connect that wraps the hotkey callback in vice lock/unlock.
 */
void vice_locking_gtk_accel_group_connect(
    GtkAccelGroup *accel_group,
    guint accel_key,
    GdkModifierType accel_mods,
    GtkAccelFlags accel_flags,
    GClosure *closure)
{
    /* Wrap invocation of signal_handler with lock & unlock. */
    g_closure_add_marshal_guards(closure, (gpointer)"hotkey", vice_gtk3_lock, (gpointer)"hotkey", vice_gtk3_unlock);

    gtk_accel_group_connect(accel_group, accel_key, accel_mods, accel_flags, closure);
}
