/** \file   settings_video.c
 * \brief   Widget to control video settings
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES CrtcDoubleSize      xcbm2 xpet
 * $VICERES CrtcDoubleScan      xcbm2 xpet
 * $VICERES CrtcStretchVertical xcbm2 xpet
 * $VICERES CrtcAudioLeak       xcbm2 xpet
 * $VICERES TEDDoubleSize       xplus4
 * $VICERES TEDDoubleScan       xplus4
 * $VICERES TEDAudioLeak        xplus4
 * $VICERES VDCDoubleSize       x128
 * $VICERES VDCDoubleScan       x128
 * $VICERES VDCStretchVertical  x128
 * $VICERES VDCAudioLeak        x128
 * $VICERES VICDoubleSize       xvic
 * $VICERES VICDoubleScan       xvic
 * $VICERES VICAudioLeak        xvic
 * $VICERES VICIIDoubleSize     x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIIDoubleScan     x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIIAudioLeak      x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIICheckSbColl    x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIICheckSsColl    x64 x64sc x64dtv xscpu64 x128 xcbm5x0
 * $VICERES VICIIVSPBug         x64sc xscpu64
 * $VICERES KeepAspectRatio     -vsid
 * $VICERES TrueAspectRatio     -vsid
 * $VICERES C128HideVDC         x128
 *
 *  (see included widgets for more resources)
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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "ui.h"
#include "basewidgets.h"
#include "lib.h"
#include "resourcecheckbutton.h"
#include "widgethelpers.h"
#include "debug_gtk3.h"
#include "resources.h"
#include "machine.h"
#include "uivideo.h"
#include "videopalettewidget.h"
#include "videorenderfilterwidget.h"
#include "videobordermodewidget.h"

#include "settings_video.h"


/** \brief  Heap allocated titles for the sub-widgets
 */
static char *widget_title[2] = { NULL, NULL };

/** \brief  References to the chip names passed to create_layout()
 */
static const char *chip_name[2] = { NULL, NULL };


/* These are required for x128, since these resources are chip-independent, but
 * there's a TODO to make these resources chip-dependent. For now toggling a
 * checkbox in VICII settings should update the checkbox in VDC settings, and
 * vice-versa. Once the resources are chip-dependent, this can be removed.
 */

/** \brief  keep-aspect-ratio widgets
 */
static GtkWidget *keep_aspect_widget[2] = { NULL, NULL };

/** \brief  true-aspect-ratio widgets
 */
static GtkWidget *true_aspect_widget[2] = { NULL, NULL };

/** \brief  double-size widgets
 */
static GtkWidget *double_size_widget[2] = { NULL, NULL };

/** \brief  render-filter widgets
 */
static GtkWidget *render_filter_widget[2] = { NULL, NULL };


/** \brief  Handler for the "destroy" event of the main widget
 *
 * Cleans up heap-allocated resources
 *
 * \param[in]   widget  main widget
 */
static void on_destroy(GtkWidget *widget)
{
    if (widget_title[0] != NULL) {
        lib_free(widget_title[0]);
        widget_title[0] = NULL;
    }
    if (widget_title[1] != NULL) {
        lib_free(widget_title[1]);
        widget_title[1] = NULL;
    }
}


/** \brief  Callback for changes of the render-filter widgets
 *
 * \param[in]   widget  radio group
 * \param[in]   value   new resource value
 */
static void render_filter_callback(GtkWidget *widget, int value)
{
    int index;

    index = GPOINTER_TO_INT(g_object_get_data(
                G_OBJECT(gtk_widget_get_parent(widget)),
                "ChipIndex"));
    vice_gtk3_resource_check_button_sync(double_size_widget[index]);
}


/** \brief  Callback for changes of the "Double Size" widget
 *
 * Trigger a resize of the primary or secondary window if \a state is false.
 *
 * There's no need to trigger a resize here when \a state is true, than happens
 * automatically by GDK to accomodate for the larger canvas.
 *
 * \param[in]   widget  check button (unused)
 * \param[in]   state   check button toggle state
 */
static void double_size_callback(GtkWidget *widget, int state)
{
    if (!state && !ui_is_fullscreen()) {

        GtkWidget *window;
        int index;

        /* Note:
         *
         * We cannot use `ui_get_active_window()` here: it returns the settings
         * window, not the primary or secondary window. Even if we could get
         * the active primary/secondary window it wouldn't help since that
         * would resize the window that spawned the settings window.
         * For example: spawning settings from the VDC window and toggling the
         * VICII double-size off would result in the VDC window getting resized.
         *
         * --compyx
         */

        /* get index passed in the main widget's constructor */
        index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "ChipIndex"));
        window = ui_get_window_by_index(index);
        if (window != NULL) {
            gtk_window_resize(GTK_WINDOW(window), 1, 1);
        }
    }
}



/** \brief  Create "Double Size" checkbox
 *
 * \param[in]   index   chip index
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_double_size_widget(int index)
{
    GtkWidget *widget;

    widget = vice_gtk3_resource_check_button_new_sprintf("%sDoubleSize",
                                                         "Double size",
                                                         chip_name[index]);
    vice_gtk3_resource_check_button_add_callback(widget,
                                                 double_size_callback);
    return widget;
}


/** \brief  Create "Double Scan" checkbox
 *
 * \param[in]   index   chip index
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_double_scan_widget(int index)
{
    return vice_gtk3_resource_check_button_new_sprintf(
            "%sDoubleScan", "Double scan",
            chip_name[index]);
}


/** \brief  Create "Vertical Stretch" checkbox
 *
 * \param[in]   index   chip index
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_vert_stretch_widget(int index)
{
    return vice_gtk3_resource_check_button_new_sprintf(
            "%sStretchVertical","Stretch vertically",
            chip_name[index]);
}


/** \brief  Create "Audio leak emulation" checkbox
 *
 * \param[in]   index   chip index
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_audio_leak_widget(int index)
{
    return vice_gtk3_resource_check_button_new_sprintf(
            "%sAudioLeak", "Audio leak emulation",
            chip_name[index]);
}


/** \brief  Create "Sprite-sprite collisions" checkbox
 *
 * \param[in]   index   chip index
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_sprite_sprite_widget(int index)
{
    return vice_gtk3_resource_check_button_new_sprintf(
            "%sCheckSsColl", "Sprite-sprite collisions",
            chip_name[index]);
}

/** \brief  Create "Sprite-background collisions" checkbox
 *
 * \param[in]   index   chip index
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_sprite_background_widget(int index)
{
    return vice_gtk3_resource_check_button_new_sprintf(
            "%sCheckSbColl", "Sprite-background collisions",
            chip_name[index]);
}


/** \brief  Create "VSP bug emulation" checkbox
 *
 * \param[in]   index   chip index
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_vsp_bug_widget(int index)
{
    return vice_gtk3_resource_check_button_new_sprintf(
            "%sVSPBug", "VSP bug emulation",
            chip_name[index]);
}


/** \brief  Create "Keep aspect ratio" checkbox
 *
 * \param[in]   index   chip index
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_keep_aspect_widget(int index)
{
    return vice_gtk3_resource_check_button_new(
            "KeepAspectRatio", "Keep aspect ratio");
}


/** \brief  Create "True aspect ratio" checkbox
 *
 * \param[in]   index   chip index
 *
 * \return  GtkCheckButton
 */
static GtkWidget *create_true_aspect_widget(int index)
{
    return vice_gtk3_resource_check_button_new(
            "TrueAspectRatio", "True aspect ratio");
}

#if 0
/** \brief  Handler for the 'toggled' event of the keep-aspect-ratio checkbox
 *
 * \param[in]   check       checkbox
 * \param[in]   user_data   chip index (int)
 */
static void on_keep_aspect_toggled(GtkWidget *check, gpointer user_data)
{
    int index = GPOINTER_TO_INT(user_data);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(keep_aspect_widget[index]),
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)));
}


/** \brief  Handler for the 'toggled' event of the true-aspect-ratio checkbox
 *
 * \param[in]   check       checkbox
 * \param[in]   user_data   chip index (int)
 */
static void on_true_aspect_toggled(GtkWidget *check, gpointer user_data)
{
    int index = GPOINTER_TO_INT(user_data);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(true_aspect_widget[index]),
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)));
}
#endif

/** \brief  Event handler for the 'Hide VDC Window' checkbox
 *
 * \param[in]   check   checkbutton triggering the event
 * \param[in]   data    settings dialog
 */
static void on_hide_vdc_toggled(GtkWidget *check, gpointer data)
{
    int hide;
    GtkWidget *window;

    hide = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check));
    window = ui_get_window_by_index(SECONDARY_WINDOW);  /* VDC */
    if (window != NULL) {
        if (hide) {
            /* close setting dialog on VDC window */
            if (ui_get_main_window_index() == SECONDARY_WINDOW) {
                gtk_window_close(GTK_WINDOW(data));
            }
            /* hide VDC window and show VICII window */
            gtk_widget_hide(window);
            window = ui_get_window_by_index(PRIMARY_WINDOW);    /* VICII */
            gtk_window_present(GTK_WINDOW(window));
        } else {
            gtk_widget_show(window);
        }
    }
}


/** \brief  Create widget for double size/scan, video cache and vert stretch
 *
 * \param[in]   index   chip index (using in x128)
 * \param[in]   chip    chip name
 *
 * \return  GtkGrid
 */
static GtkWidget *create_render_widget(int index, const char *chip)
{
    GtkWidget *grid;
    GtkWidget *double_scan_widget = NULL;
    GtkWidget *vert_stretch_widget = NULL;

    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT);

    double_size_widget[index] = create_double_size_widget(index);
    g_object_set_data(G_OBJECT(double_size_widget[index]),
                               "ChipIndex",
                               GINT_TO_POINTER(index));
    g_object_set(double_size_widget[index], "margin-left", 16, NULL);

    double_scan_widget = create_double_scan_widget(index);

    gtk_grid_attach(GTK_GRID(grid), double_size_widget[index], 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), double_scan_widget, 1, 0, 1, 1);

    if (uivideo_chip_has_vert_stretch(chip)) {
        vert_stretch_widget = create_vert_stretch_widget(index);
        gtk_grid_attach(GTK_GRID(grid), vert_stretch_widget, 2, 0, 1, 1);
    }

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create widget for audio leak, sprite collisions and VSP bug
 *
 * \param[in]   index   chip index (using in x128)
 * \param[in]   chip    chip name
 *
 * \return  GtkGrid
 */
static GtkWidget *create_misc_widget(int index, const char *chip)
{
    GtkWidget *grid;
    GtkWidget *audio_leak_widget;
    GtkWidget *sprite_sprite_widget = NULL;
    GtkWidget *sprite_background_widget = NULL;
    GtkWidget *vsp_bug_widget = NULL;
    int row = 2;

    grid = vice_gtk3_grid_new_spaced_with_label(
            VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT, "Miscellaneous", 1);

    audio_leak_widget = create_audio_leak_widget(index);
    g_object_set(audio_leak_widget, "margin-left", 16, NULL);
    gtk_grid_attach(GTK_GRID(grid), audio_leak_widget, 0, 1, 1, 1);

    if (uivideo_chip_has_sprites(chip)) {
        sprite_sprite_widget = create_sprite_sprite_widget(index);
        sprite_background_widget = create_sprite_background_widget(index);
        g_object_set(sprite_sprite_widget, "margin-left", 16, NULL);
        g_object_set(sprite_background_widget, "margin-left", 16, NULL);
        gtk_grid_attach(GTK_GRID(grid), sprite_sprite_widget, 0, 2, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), sprite_background_widget, 0, 3, 1, 1);
        row = 4;
    }
    if (uivideo_chip_has_vsp_bug(chip)) {
        vsp_bug_widget = create_vsp_bug_widget(index);
        g_object_set(vsp_bug_widget, "margin-left", 16, NULL);
        gtk_grid_attach(GTK_GRID(grid), vsp_bug_widget, 0, row, 1, 1);

    }
    gtk_widget_show(grid);
    return grid;
}


/** \brief  Create widget for HW scaling and keep/true aspect ratio
 *
 * \param[in]   index   chip index (used in x128)
 * \param[in]   chip    chip name
 *
 * \return  GtkGrid
 */
static GtkWidget *create_scaling_widget(int index, const char *chip)
{
    GtkWidget *grid;

    grid = vice_gtk3_grid_new_spaced_with_label(
            VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT, "Scaling and fullscreen", 3);

    keep_aspect_widget[index] = create_keep_aspect_widget(index);
    g_object_set(keep_aspect_widget[index], "margin-left", 16, NULL);
    /* until per-chip KeepAspectRatio is implemented, connect the VICII and
     * VDC KeepAspectRatio checkboxes, so toggling the VICII checkbox also
     * updates the VDC checkbox, and vice-versa */

    /* No longer required: VICII and VDC have separate 'pages' now */
#if 0
    if (machine_class == VICE_MACHINE_C128) {
        g_signal_connect(keep_aspect_widget[index], "toggled",
                G_CALLBACK(on_keep_aspect_toggled),
                GINT_TO_POINTER(index == 0 ? 1: 0));
    }
#endif
    gtk_grid_attach(GTK_GRID(grid), keep_aspect_widget[index], 0, 1, 1, 1);

    true_aspect_widget[index] = create_true_aspect_widget(index);
    /* until per-chip TrueAspectRatio is implemented, connect the VICII and
     * VDC TrueAspectRatio checkboxes, so toggling the VICII checkbox also
     * updates the VDC checkbox, and vice-versa */

    /* No longer required: VICII and VDC have separate 'pages' now */
#if 0
    if (machine_class == VICE_MACHINE_C128) {
        g_signal_connect(true_aspect_widget[index], "toggled",
                G_CALLBACK(on_true_aspect_toggled),
                GINT_TO_POINTER(index == 0 ? 1: 0));
    }
#endif
    gtk_grid_attach(GTK_GRID(grid), true_aspect_widget[index], 1, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create a per-chip video settings layout
 *
 * \param[in]   parent  parent widget, required for dialogs and window switching
 * \param[in]   chip    chip name ("Crtc", "TED", "VDC", "VIC", or "VICII")
 * \param[in]   index   index in the general layout (0 or 1 (x128))
 *
 * \return  GtkGrid
 */
static GtkWidget *create_layout(GtkWidget *parent, const char *chip, int index)
{
    GtkWidget *layout;
    GtkWidget *wrapper;
    GtkWidget *hide_vdc;

    widget_title[index] = lib_msprintf("%s Settings", chip);
    chip_name[index] = chip;

    /* row 0, col 0-2: title */
    layout = vice_gtk3_grid_new_spaced_with_label(
            VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT, widget_title[index], 3);
    /* spread out the rows a bit more */
    gtk_grid_set_row_spacing(GTK_GRID(layout),
            VICE_GTK3_GRID_ROW_SPACING * 2);

    /* row 1, col 0-2: video output options */
    wrapper = create_render_widget(index, chip);
    gtk_grid_attach(GTK_GRID(layout), wrapper, 0, 1, 3, 1);

    /* row 2, col 0-2: palette selection */
    gtk_grid_attach(GTK_GRID(layout),
            video_palette_widget_create(chip),
            0, 2, 3, 1);

    /* row 3, col 0: rendering filter */
    render_filter_widget[index] = video_render_filter_widget_create(chip);
    /* set chip index to use for the callback */
    g_object_set_data(G_OBJECT(render_filter_widget[index]),
                      "ChipIndex",
                      GINT_TO_POINTER(index));
    /* add callback to widget */
    video_render_filter_widget_add_callback(render_filter_widget[index],
                                            render_filter_callback);
    gtk_grid_attach(GTK_GRID(layout),
            render_filter_widget[index],
            0, 3, 1, 1);
    /* row 3, col 1: border-mode  */
    if (uivideo_chip_has_border_mode(chip)) {
        /* add border mode widget */
        gtk_grid_attach(GTK_GRID(layout),
                video_border_mode_widget_create(chip),
                1, 3, 1, 1);
    }
    /* row 3, col 2: misc options */
    wrapper = create_misc_widget(index, chip);
    gtk_grid_attach(GTK_GRID(layout), wrapper, 2, 3, 1, 1);

    /* row 4, col 0-2: scaling and aspect ratio resources */
    wrapper = create_scaling_widget(index, chip);
    gtk_grid_attach(GTK_GRID(layout), wrapper, 0, 4, 3, 1);

    /* Hide VDC checkbox
     *
     * Only show when VICII window, VICII settings
     */
    if (machine_class == VICE_MACHINE_C128) {
        /* compare 4 bytes, so perhaps an int cmp is faster? */
        if (strcmp(chip, "VDC") == 0) {
            hide_vdc = vice_gtk3_resource_check_button_new(
                    "C128HideVDC", "Hide VDC display");
            g_signal_connect(hide_vdc, "toggled",
                    G_CALLBACK(on_hide_vdc_toggled), parent);
            gtk_grid_attach(GTK_GRID(layout), hide_vdc, 0, 5, 3, 1);
        }
    }
    gtk_widget_show_all(layout);
    return layout;
}


/** \brief  Create video settings widget
 *
 * This will create the VICII widget for x128, for the VDC widget use
 * settings_video_create_vdc().
 *
 * \param[in]   parent  parent widget
 *
 * \return  GtkGrid
 */
GtkWidget *settings_video_create(GtkWidget *parent)
{
    GtkWidget *grid;
    const char *chip;

    /* XXX: is this really required anymore? */
    chip_name[0] = NULL;
    chip_name[1] = NULL;
    widget_title[0] = NULL;
    widget_title[1] = NULL;
    keep_aspect_widget[0] = NULL;
    keep_aspect_widget[1] = NULL;
    true_aspect_widget[0] = NULL;
    true_aspect_widget[1] = NULL;
    double_size_widget[0] = NULL;
    double_size_widget[1] = NULL;
    render_filter_widget[0] = NULL;
    render_filter_widget[1] = NULL;

    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT);
    chip = uivideo_chip_name();
    gtk_grid_attach(GTK_GRID(grid),
            create_layout(parent, chip, PRIMARY_WINDOW),
            0, 0, 1, 1);

    g_signal_connect_unlocked(grid, "destroy", G_CALLBACK(on_destroy), NULL);
    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create video settings widget for VDC
 *
 * This will create the VDC widget for x128, for the VICII widget use
 * settings_video_create().
 *
 * \param[in]   parent  parent widget
 *
 * \return  GtkGrid
 */
GtkWidget *settings_video_create_vdc(GtkWidget *parent)
{
    GtkWidget *grid;

    chip_name[0] = NULL;
    chip_name[1] = NULL;
    widget_title[0] = NULL;
    widget_title[1] = NULL;
    keep_aspect_widget[0] = NULL;
    keep_aspect_widget[1] = NULL;
    true_aspect_widget[0] = NULL;
    true_aspect_widget[1] = NULL;

    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT);
    gtk_grid_attach(GTK_GRID(grid),
            create_layout(parent, "VDC", SECONDARY_WINDOW),
            0, 0, 1, 1);

    g_signal_connect_unlocked(grid, "destroy", G_CALLBACK(on_destroy), NULL);
    gtk_widget_show_all(grid);
    return grid;
}
