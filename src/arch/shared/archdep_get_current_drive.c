/** \file   archdep_get_current_drive.c
 * \brief   Get current drive on Windows
 * \author  Unknown (probably copied from old SDL code)
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
#include "archdep_defs.h"

#include <string.h>

#include "archdep.h"
#include "ioutil.h"

#include "archdep_get_current_drive.h"


/** \fn     archdep_get_current_drive
 * \brief   Get current Windows drive
 *
 * Gets current Windows drive and replaces '\\' with '/' for some reason.
 *
 * \return  Current Windows drive, including ':/'
 *
 * \note    free result after use with lib_free().
 */

#ifdef ARCHDEP_OS_WINDOWS
/* FIXME: is this needed* */
# ifdef SDL_CHOOSE_DRIVES

char *archdep_get_current_drive(void)
{
    char *p = ioutil_current_dir();
    char *p2 = strchr(p, '\\');
    p2[0] = '/';
    p2[1] = '\0';
    return p;
}
# endif
#endif
