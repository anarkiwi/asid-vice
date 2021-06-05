/** \file   archdep_default_portable_resource_file_name.c
 * \brief   Retrieve default portable resource file path
 * \author  groepaz <groepaz@gmx.de>
 *
 * Get path to default portable resource file (vicerc/vice.ini)
 *
 * unlike the normal resource file, this one is located in the same
 * directory as the .exe file (on windows).
 *
 * OS support:
 *  - Windows
 *
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

#include <stdlib.h>

#include "archdep_join_paths.h"
#include "archdep_boot_path.h"
#include "archdep_home_path.h"

#include "archdep_default_portable_resource_file_name.h"


/** \brief  Get path to default portable resource file
 *
 * \return  heap-allocated path, free with lib_free()
 */
char *archdep_default_portable_resource_file_name(void)
{
    const char *cfg;
#ifdef ARCHDEP_OS_WINDOWS
    cfg = archdep_boot_path();
    return archdep_join_paths(cfg, ARCHDEP_VICERC_NAME, NULL);
#else
    cfg = archdep_home_path();
    /* Don't pass the leading dot of .vicerc as a separate argument, but use
     * the preprocessor's string concatenation feature.
     * Perhaps at some point archdep_join_paths() might be updated to normalize
     * its path, meaning it would eat the ".".
     */
    return archdep_join_paths(cfg, "." ARCHDEP_VICERC_NAME, NULL);
#endif
}
