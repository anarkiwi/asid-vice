/** \file   scpu64ui.c
 * \brief   Native GTK3 SCPU64 UI
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 * \author  Bas Wassink <b.wassink@ziggo.nl>
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

#include <stdio.h>

#include "debug_gtk3.h"
#include "widgethelpers.h"
#include "c64model.h"
#include "crtcontrolwidget.h"
#include "vicii.h"
#include "machinemodelwidget.h"
#include "videomodelwidget.h"
#include "sampler.h"
#include "ui.h"
#include "uimachinewindow.h"

#include "clockport.h"

#include "cartridge.h"
#include "c64model.h"
#include "settings_model.h"

#include "scpu64ui.h"


/** \brief  List of C64 models
 *
 * Used in the machine-model widget
 */
static const char *c64scpu_model_list[] = {
    "C64 PAL", "C64C PAL", "C64 old PAL",
    "C64 NTSC", "C64C NTSC", "C64 old NTSC",
    "Drean",
    "C64 SX PAL", "C64 SX NTSC",
    "Japanese", "C64 GS",
    NULL
};


/** \brief  List of VIC-II models
 *
 * Used in the VIC-II model widget
 */
static const vice_gtk3_radiogroup_entry_t c64scpu_vicii_models[] = {
    { "6569 (PAL)",             VICII_MODEL_6569 },
    { "8565 (PAL)",             VICII_MODEL_8565 },
    { "6569R1 (old PAL)",       VICII_MODEL_6569R1 },
    { "6567 (NTSC)",            VICII_MODEL_6567 },
    { "8562 (NTSC)",            VICII_MODEL_8562 },
    { "6567R56A (old NTSC)",    VICII_MODEL_6567R56A },
    { "6572 (PAL-N)",           VICII_MODEL_6572 },
    { NULL, -1 }
};


/** \brief  Identify the canvas used to create a window
 *
 * \param[in]   canvas  video canvas reference
 *
 * \return  window index on success, -1 on failure
 */
static int identify_canvas(video_canvas_t *canvas)
{
    if (canvas != vicii_get_canvas()) {
        return -1;
    }

    return PRIMARY_WINDOW;
}

/** \brief  Create CRT controls widget for \a target window
 *
 * \param[in]   target_window   target window index
 *
 * \return  GtkGrid
 */
static GtkWidget *create_crt_widget(int target_window)
{
    return crt_control_widget_create(NULL, "VICII", TRUE);
}

/** \brief  Pre-initialize the UI before the canvas window gets created
 *
 * \return  0 on success, -1 on failure
 */
int scpu64ui_init_early(void)
{
    ui_machine_window_init();
    ui_set_identify_canvas_func(identify_canvas);
    ui_set_create_controls_widget_func(create_crt_widget);
    return 0;
}


/** \brief  Initialize the UI
 *
 * \return  0 on success, -1 on failure
 */
int scpu64ui_init(void)
{
    machine_model_widget_getter(c64model_get);
    machine_model_widget_setter(c64model_set);
    machine_model_widget_set_models(c64scpu_model_list);

    video_model_widget_set_title("VIC-II model");
    video_model_widget_set_resource("VICIIModel");
    video_model_widget_set_models(c64scpu_vicii_models);

    /* set C64 model_get function */
    settings_model_widget_set_model_func(c64model_get);
    return 0;
}


/** \brief  Shut down the UI
 */
void scpu64ui_shutdown(void)
{
    /* NOP */
}
