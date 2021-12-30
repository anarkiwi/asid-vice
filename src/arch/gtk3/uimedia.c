/** \file   uimedia.c
 * \brief   Media recording dialog
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * $VICERES SoundRecordDeviceArg        all
 * $VICERES SoundRecordDeviceName       all
 * $VICERES OCPOversizeHandling         -vsid
 * $VICERES OCPUndersizeHandling        -vsid
 * $VICERES OCPMultiColorHandling       -vsid
 * $VICERES OCPTEDLumHandling           -vsid
 * $VICERES KoalaOversizeHandling       -vsid
 * $VICERES KoalaUndersizeHandling      -vsid
 * $VICERES KoalaTEDLumHandling         -vsid
 * $VICERES MinipaintOversizeHandling   -vsid
 * $VICERES MinipaintUndersizeHandling  -vsid
 * $VICERES MinipaintTEDLumHandling     -vsid
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
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "gfxoutput.h"
#include "lastdir.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "mainlock.h"
#include "monitor.h"
#include "resources.h"
#include "screenshot.h"
#include "sound.h"
#include "statusbarrecordingwidget.h"
#include "ui.h"
#include "uiapi.h"
#include "uistatusbar.h"
#include "vice_gtk3.h"
#include "videoarch.h"
#include "vsync.h"

#ifdef HAVE_FFMPEG
#include "ffmpegwidget.h"
#endif

#include "uimedia.h"


/** \brief  Custom response IDs for the dialog
 */
enum {
    RESPONSE_SAVE = 1   /**< Save button clicked */
};


/** \brief  Name of the stack child for screenshots
 */
#define CHILD_SCREENSHOT    "Screenshot"

/** \brief  Name of the stack child for sound recording
 */
#define CHILD_SOUND         "Sound"

/** \brief  Name of the stack child for video recording
*/
#define CHILD_VIDEO         "Video"


/** \brief  Struct to hold information on available video recording drivers
 */
typedef struct video_driver_info_s {
    const char *display;    /**< display string (used in the UI) */
    const char *name;       /**< driver name */
    const char *ext;        /**< default file extension */
} video_driver_info_t;


/** \brief  Struct to hold information on available audio recording drivers
 */
typedef struct audio_driver_info_s {
    const char *display;    /**< display name (used in the UI) */
    const char *name;       /**< driver name */
    const char *ext;        /**< default file extension */
} audio_driver_info_t;


/** \brief  List of available video drivers, gotten during runtime
 */
static video_driver_info_t *video_driver_list = NULL;


/** \brief  Number of available video drivers
 */
static int video_driver_count = 0;


/** \brief  Index of currently selected screenshot driver
 *
 * Initially set to -1 to select PNG as default in the screenshot driver list.
 * Set to last selected driver on subsequent uses of the dialog.
 */
static int screenshot_driver_index = -1;


#ifdef HAVE_FFMPEG
/** \brief  Index of currently selected video driver
 *
 */
static int video_driver_index = 0;
#endif  /* HAVE_FFMPEG */


/** \brief  Index of currently selected audio driver
 *
 * Initially set to -1 to select WAV as default in the audio driver list.
 * Set to last selected driver on subsequent uses of the dialog.
 */
static int audio_driver_index = -1;


/** \brief  Last used directory of the file chooser dialogs
 *
 * We only remember the directory, not the last filename, since we propose a
 * filename with a timestamp in it to quickly save new media files without having
 * to type a new name. Using last_file would mean the generated filename gets
 * overwritten with the old one, so the convenience of the generated filenames
 * would be lost.
 */
static gchar *media_last_dir = NULL;


/** \brief  List of available audio recording drivers
 *
 * This list is dependent on compile-time options
 */
static audio_driver_info_t audio_driver_list[] = {
    { "WAV",        "wav",  "wav" },
    { "AIFF",       "aiff", "aif" },
    { "VOC",        "voc",  "voc" },
    { "IFF",        "iff",  "iff" },
#ifdef USE_LAMEMP3
    { "MP3",        "mp3",  "mp3" },
#endif
#ifdef USE_FLAC
    { "FLAC",       "flac", "flac" },
#endif
#ifdef USE_VORBIS
    { "Ogg/Vorbis", "ogg",  "ogg" },
#endif
    { NULL,         NULL,   NULL }
};


/** \brief  List of 'oversize modes' for some screenshot output drivers
 */
static vice_gtk3_combo_entry_int_t oversize_modes[] = {
    { "scale down", 0 },
    { "crop left top", 1, },
    { "crop center top", 2 },
    { "crop right top", 3 },
    { "crop left center", 4 },
    { "crop center", 5 },
    { "crop right center", 6 },
    { "crop left bottom", 7 },
    { "crop center bottom", 8 },
    { "crop right bottom", 9 },
    { NULL, -1 }
};


/** \brief  List of 'undersize modes' for some screenshot output drivers
 */
static vice_gtk3_combo_entry_int_t undersize_modes[] = {
    { "scale up", 0 },
    { "border size", 1 },
    { NULL, -1 }
};


/** \brief  List of multi color modes for some screenshot output drivers
 */
static const vice_gtk3_combo_entry_int_t multicolor_modes[] = {
    { "B&W", 0 },
    { "2 colors", 1 },
    { "4 colors", 2 },
    { "Grey scale", 3 },
    { "Best cell colors", 4 },
    { NULL, -1 }
};


/** \brief  TED output driver Luma modes
 */
static const vice_gtk3_combo_entry_int_t ted_luma_modes[] = {
    { "ignore", 0 },
    { "Best cell colors", 1 },
    { NULL, -1 },
};


/* forward declarations of helper functions */
static GtkWidget *create_screenshot_param_widget(const char *prefix);
static void save_screenshot_handler(GtkWidget *parent);
static void save_audio_recording_handler(GtkWidget *parent);
static void save_video_recording_handler(GtkWidget *parent);


/** \brief  Screenshot filename for save_screenshot_vsync_callback()
 *
 * This gets allocated by the UI thread and freed by the VICE thread.
 *
 * If non-NULL the UI thread will not free and reallocate, it simply refuses
 * to queue another screenshot. Queuing a second screenshot while the vsync
 * handler hasn't processed the current one is an unlikely scenario, but could
 * happen with a very low custom emulation speed and a quick user. Once the
 * vsync handler has used the filename, it is deallocated and set to NULL to
 * indicate to the UI another screenshot can be queued.
 */
static char *screenshot_filename = NULL;


/** \brief  Screenshot driver name for save_screenshot_vsync_callback()
 *
 * When the dialog is closed the video drive list is also destroyed, so we
 * cannot access the list in the vsync callback.
 * The UI thread created a copy of the requested driver and the VICE thread
 * then frees it in the vsync callback function.
 */
static char *screenshot_driver = NULL;



/** \brief  Reference to the GtkStack containing the media types
 *
 * Used in the dialog response callback to determine recording mode and params
 */
static GtkWidget *stack;


/** \brief  Pause state when activating the dialog
 */
static int old_pause_state = 0;


/* references to widgets, used from various event handlers */

/** \brief  Screenshot options grid reference */
static GtkWidget *screenshot_options_grid = NULL;
/** \brief  Screenshot "Oversize" widget reference */
static GtkWidget *oversize_widget = NULL;
/** \brief  Screenshot "Undersize" widget reference */
static GtkWidget *undersize_widget = NULL;
/** \brief  Screenshot "Multicolor mode" widget reference */
static GtkWidget *multicolor_widget = NULL;
/** \brief  Screenshot "TED luma" widget reference */
static GtkWidget *ted_luma_widget = NULL;
#ifdef HAVE_FFMPEG
/** \brief  FFMPEG video driver options grid reference */
static GtkWidget *video_driver_options_grid = NULL;
#endif


/*****************************************************************************
 *                              Event handlers                               *
 ****************************************************************************/

/** \brief  Handler for the "destroy" event of the dialog
 *
 * \param[in]   widget  dialog
 * \param[in]   data    extra event data (unused)
 */
static void on_dialog_destroy(GtkWidget *widget, gpointer data)
{
    if (machine_class != VICE_MACHINE_VSID) {
        lib_free(video_driver_list);
    }

    /* only unpause when the emu wasn't paused when activating the dialog */
    if (!old_pause_state) {
        ui_pause_disable();
    }
}


/** \brief  Set new widget for the screenshot options, replacing the old one
 *
 * Destroys any widget present before setting the \a new widget.
 *
 * \param[in]   new     new widget
 */
static void update_screenshot_options_grid(GtkWidget *new)
{
    GtkWidget *old;

    if (new != NULL) {
        old = gtk_grid_get_child_at(GTK_GRID(screenshot_options_grid), 0, 1);
        if (old != NULL) {
            gtk_widget_destroy(old);
        }
        gtk_grid_attach(GTK_GRID(screenshot_options_grid), new, 0, 1, 1, 1);
    }
}


/** \brief  Handler for the "response" event of the \a dialog
 *
 * \param[in,out]   dialog      dialog triggering the event
 * \param[in]       response_id response ID
 * \param[in]       data        parent dialog
 */
static void on_response(GtkDialog *dialog, gint response_id, gpointer data)
{
    const gchar *child_name;

    switch (response_id) {
        case GTK_RESPONSE_DELETE_EVENT:
            mainlock_release();
            gtk_widget_destroy(GTK_WIDGET(dialog));
            mainlock_obtain();
            break;

        case RESPONSE_SAVE:

            if (machine_class != VICE_MACHINE_VSID) {
                /* stack child name determines what to do next */
                child_name = gtk_stack_get_visible_child_name(GTK_STACK(stack));

                if (strcmp(child_name, CHILD_SCREENSHOT) == 0) {
                    save_screenshot_handler(data);
                } else if (strcmp(child_name, CHILD_SOUND) == 0) {
                    save_audio_recording_handler(data);
                    ui_display_recording(1);
                } else if (strcmp(child_name, CHILD_VIDEO) == 0) {
                    save_video_recording_handler(data);
                    ui_display_recording(1);
                }
            } else {
                save_audio_recording_handler(data);
                ui_display_recording(1);
            }
#if 0
            mainlockk_release();
            gtk_widget_destroy(GTK_WIDGET(dialog));
            mainlock_obtain();
#endif
            break;

        default:
            break;
    }
}


/** \brief  Handler for the "toggled" event of the screenshot driver radios
 *
 * \param[in]       widget      radio button riggering the event
 * \param[in]       data        extra event data (unused)
 */
static void on_screenshot_driver_toggled(GtkWidget *widget, gpointer data)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        int index = GPOINTER_TO_INT(data);
        screenshot_driver_index = index;
        update_screenshot_options_grid(
                create_screenshot_param_widget(video_driver_list[index].name));
    }
}


/** \brief  Handler for the "toggled" event of the audio driver radios
 *
 * \param[in]       widget      radio button riggering the event
 * \param[in]       data        extra event data (unused)
 */
static void on_audio_driver_toggled(GtkWidget *widget, gpointer data)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        int index = GPOINTER_TO_INT(data);
        audio_driver_index = index;
    }
}



/*****************************************************************************
 *                              Helpers functions                            *
 ****************************************************************************/

/** \brief  Create a string in the format 'yyyymmddHHMMssffffff' of the current time
 *
 * \return  string owned by GLib, free with g_free()
 */
static gchar *create_datetime_string(void)
{
    GDateTime *d;
    gint m;
    gchar *s;
    gchar *t;

    d = g_date_time_new_now_local();
    m = g_date_time_get_microsecond(d);
    s = g_date_time_format(d, "%Y%m%d%H%M%S");
    g_date_time_unref(d);
    t = g_strdup_printf("%s%02d", s, m / 10000);
    g_free(s);
    return t;
}


/** \brief  Create a filename based on the current datetime and \a ext
 *
 * \param[in]   ext file extension (without the dot)
 *
 * \return  heap-allocated string, owned by VICE, free with lib_free()
 */
static char *create_proposed_screenshot_name(const char *ext)
{
    char *date;
    char *filename;

    date = create_datetime_string();
    filename = lib_msprintf("vice-screen-%s.%s", date, ext);
    g_free(date);
    return filename;
}



/** \brief  Create a filename based on the current datetime and \a ext
 *
 * \param[in]   ext file extension (without the dot)
 *
 * \return  heap-allocated string, owned by VICE, free with lib_free()
 */
static char *create_proposed_video_recording_name(const char *ext)
{
    gchar *date;
    char *filename;

    date = create_datetime_string();
    filename = lib_msprintf("vice-video-%s.%s", date, ext);
    g_free(date);
    return filename;
}



/** \brief  Create a filename based on the current datetime and \a ext
 *
 * \param[in]   ext file extension (without the dot)
 *
 * \return  heap-allocated string, owned by VICE, free with lib_free()
 */
static char *create_proposed_audio_recording_name(const char *ext)
{
    gchar *date;
    char *filename;

    date = create_datetime_string();
    filename = lib_msprintf("vice-audio-%s.%s", date, ext);
    g_free(date);
    return filename;
}


/** \brief  UI error message handler for saving screenshots
 *
 * \param[in]   data    filename of screenshot
 *
 * \return  G_SOURCE_REMOVE to make this a one-shot timer event
 */
static gboolean save_screenshot_error_impl(gpointer data)
{
    char *filename = data;

    vice_gtk3_message_error("Screenshot error",
                            "Failed to write screenshot file '%s.'",
                            filename);
    lib_free(filename);
    return G_SOURCE_REMOVE;
}


/** \brief  Callback to make a screenshot on vsync to avoid tearing
 *
 * The `screenshot_filename` variable is allocated by the UI thread and freed
 * and set to `NULL` by this function, thus indicating to the UI it is allowed
 * to queue another screenshot on vsync.
 *
 * \param[in]   param   video canvas to take screenshot of
 */
static void save_screenshot_vsync_callback(void *param)
{
    video_canvas_t *canvas = param;

    if (screenshot_save(screenshot_driver, screenshot_filename, canvas) < 0) {
        char *filename_copy;

        log_error(LOG_ERR, "Failed to write screenshot file '%s'.",
                  screenshot_filename);
        /* push error message handler onto the UI thread */
        filename_copy = lib_strdup(screenshot_filename);
        g_timeout_add(0, save_screenshot_error_impl, (gpointer)filename_copy);
    }
    lib_free(screenshot_filename);
    lib_free(screenshot_driver);
    screenshot_filename = NULL; /* signal UI it can queue another screenshot */
    screenshot_driver = NULL;
}


/** \brief  Callback for the save-screenshot dialog
 *
 * \param[in,out]   dialog      dialog
 * \param[in,out]   filename    screenshot filename
 * \param[in]       data        extra data (unused)
 */
static void on_save_screenshot_filename(GtkDialog *dialog,
                                        gchar *filename,
                                        gpointer data)
{
    if (filename != NULL) {

        gchar *filename_locale = file_chooser_convert_to_locale(filename);

        /* TODO: add extension if not present? */

        /* check if a screenshot is pending */
        if (screenshot_filename == NULL) {
            /* update last dir/file */
            lastdir_update(GTK_WIDGET(dialog), &media_last_dir, NULL);
            /* no, queue screenshot on vsync */
            screenshot_filename = lib_strdup(filename_locale);
            screenshot_driver = lib_strdup(video_driver_list[screenshot_driver_index].name);

            if (monitor_is_inside_monitor()) {
                /* screenshot immediately if monitor is open */
                save_screenshot_vsync_callback((void*)ui_get_active_canvas());
            } else {
                /* queue screenshot grab on vsync to avoid tearing */
                vsync_on_vsync_do(save_screenshot_vsync_callback, (void *)ui_get_active_canvas());
            }
        }
        g_free(filename);
        g_free(filename_locale);
    }
    mainlock_release();
    gtk_widget_destroy(GTK_WIDGET(dialog));
    mainlock_obtain();
}


/** \brief  Save a screenshot
 *
 * Pops up a save-file dialog with a proposed filename (ie
 * 'screenshot-197411151210.png'.
 *
 * \note    Destroys \a parent along with the dialog
 *
 * \param[in,out]   parent  parent dialog
 */
static void save_screenshot_handler(GtkWidget *parent)
{
    GtkWidget *dialog;
    const char *display;
    char *title;
    char *proposed;
    const char *ext;

    ext = video_driver_list[screenshot_driver_index].ext;
    display = video_driver_list[screenshot_driver_index].display;
    title = lib_msprintf("Save %s file", display);
    proposed = create_proposed_screenshot_name(ext);

    dialog = vice_gtk3_save_file_dialog(
            title, proposed, TRUE, NULL,
            on_save_screenshot_filename,
            NULL);
    lastdir_set(dialog, &media_last_dir, NULL);

    /* destroy parent dialog when the dialog is destroyed */
    g_signal_connect_swapped(
            dialog,
            "destroy",
            G_CALLBACK(gtk_widget_destroy),
            parent);

    lib_free(proposed);
    lib_free(title);
}


/** \brief  Callback for the save-audio dialog
 *
 * \param[in,out]   dialog      dialog
 * \param[in,out]   filename    audio recording filename
 * \param[in]       data        extra data (unused)
 */
static void on_save_audio_filename(GtkDialog *dialog,
                                   gchar *filename,
                                   gpointer data)
{
    gchar *filename_locale;

    if (filename != NULL) {
        lastdir_update(GTK_WIDGET(dialog), &media_last_dir, NULL);

        filename_locale = file_chooser_convert_to_locale(filename);
        const char *name = audio_driver_list[audio_driver_index].name;
        /* XXX: setting resources doesn't exactly help with catching errors */
        resources_set_string("SoundRecordDeviceArg", filename_locale);
        resources_set_string("SoundRecordDeviceName", name);
        g_free(filename);
        g_free(filename_locale);
    }
    mainlock_release();
    gtk_widget_destroy(GTK_WIDGET(dialog));
    mainlock_obtain();
}



/** \brief  Start an audio recording
 *
 * Pops up a save-file dialog with a proposed filename (ie
 * 'audio-recording-197411151210.png'.
 *
 * \note    Destroys \a parent along with the dialog
 *
 * \param[in,out]   parent dialog
 */
static void save_audio_recording_handler(GtkWidget *parent)
{
    GtkWidget *dialog;
    const char *display;
    const char *ext;
    char *title;
    char *proposed;

    ext = audio_driver_list[audio_driver_index].ext;
    display = audio_driver_list[audio_driver_index].display;
    title = lib_msprintf("Save %s file", display);
    proposed = create_proposed_audio_recording_name(ext);

    dialog = vice_gtk3_save_file_dialog(
            title, proposed, TRUE, NULL,
            on_save_audio_filename,
            NULL);
    lastdir_set(dialog, &media_last_dir, NULL);
    /* destroy parent dialog when the dialog is destroyed */
    g_signal_connect_swapped(
            dialog,
            "destroy",
            G_CALLBACK(gtk_widget_destroy),
            parent);

    lib_free(title);
    lib_free(proposed);
}



/** \brief  Callback for the save-video dialog
 *
 * \param[in,out]   dialog      dialog
 * \param[in,out]   filename    video recording filename
 * \param[in]       data        extra data (unused)
 */
static void on_save_video_filename(GtkDialog *dialog,
                                   gchar *filename,
                                   gpointer data)
{
    if (filename != NULL) {

        const char *driver;
        int ac;
        int vc;
        int ab;
        int vb;
        gchar *filename_locale;

        lastdir_update(GTK_WIDGET(dialog), &media_last_dir, NULL);

        resources_get_string("FFMPEGFormat", &driver);
        resources_get_int("FFMPEGVideoCodec", &vc);
        resources_get_int("FFMPEGVideoBitrate", &vb);
        resources_get_int("FFMPEGAudioCodec", &ac);
        resources_get_int("FFMPEGAudioBitrate", &ab);

        filename_locale = file_chooser_convert_to_locale(filename);

        /* TODO: add extension if not present? */
        if (screenshot_save("FFMPEG", filename_locale, ui_get_active_canvas()) < 0) {
            vice_gtk3_message_error("VICE Error",
                    "Failed to write video file '%s'", filename);
        }
        g_free(filename);
        g_free(filename_locale);
    }
    mainlock_release();
    gtk_widget_destroy(GTK_WIDGET(dialog));
    mainlock_obtain();
}


/** \brief  Start recording a video
 *
 * Pops up a save-file dialog with a proposed filename (ie
 * 'video-recording-197411151210.png'.
 *
 * \note    Destroys \a parent along with the dialog
 *
 * \param[in,out]   parent  parent dialog
 */
static void save_video_recording_handler(GtkWidget *parent)
{
    GtkWidget *dialog;
    const char *ext;
    char *title;
    char *proposed;

#if 0
    display = video_driver_list[video_driver_index].display;
    name = video_driver_list[video_driver_index].name;
#endif
    /* we don't have a format->extension mapping, so the format name itself is
       better than `video_driver_list[video_driver_index].ext' */
    resources_get_string("FFMPEGFormat", &ext);

    title = lib_msprintf("Save %s file", "FFMPEG");
    proposed = create_proposed_video_recording_name(ext);

    dialog = vice_gtk3_save_file_dialog(
            title, proposed, TRUE, NULL,
            on_save_video_filename, NULL);
    lastdir_set(dialog, &media_last_dir, NULL);

    /* destroy parent dialog when the dialog is destroyed */
    g_signal_connect_swapped(
            dialog,
            "destroy",
            G_CALLBACK(gtk_widget_destroy),
            parent);

    lib_free(proposed);
    lib_free(title);
}


/** \brief  Create heap-allocated list of available video drivers
 *
 * Queries the gfxoutputdrv subsystem and builds a list of currently available
 * drivers and stores it in `driver_list`. Must be freed with lib_free() after
 * use.
 */
static void create_video_driver_list(void)
{
    gfxoutputdrv_t *driver;
    int index;

    video_driver_count = gfxoutput_num_drivers();
    video_driver_list = lib_malloc((size_t)(video_driver_count + 1) *
            sizeof *video_driver_list);

    index = 0;

    if (video_driver_count > 0) {
        driver = gfxoutput_drivers_iter_init();
        while (driver != NULL) {
            video_driver_list[index].display = driver->displayname;
            video_driver_list[index].name = driver->name;
            video_driver_list[index].ext = driver->default_extension;
            index++;
            driver = gfxoutput_drivers_iter_next();
        }
    }
    video_driver_list[index].display = NULL;
    video_driver_list[index].name = NULL;
    video_driver_list[index].ext = NULL;
}


/** \brief  Determine if driver \a name is a video driver
 *
 * \param[in]   name    driver name
 *
 * \return  bool
 *
 * \todo    There has to be a better, more reliable way than this
 */
static int driver_is_video(const char *name)
{
    int result;

    result = strcmp(name, "FFMPEG") == 0;
    return result;
}


/** \brief  Create widget to control screenshot parameters
 *
 * This widget exposes controls to alter screenshot parameters, depending on
 * machine class and output file format
 *
 * \param[in]   prefix  resource prefix (either "OCP", "Koala", or ""/NULL)
 *
 * \return  GtkGrid
 */
static GtkWidget *create_screenshot_param_widget(const char *prefix)
{
    GtkWidget *grid;
    GtkWidget *label;
    int row;
    int artstudio = 0;
    int koala = 0;
    int minipaint = 0;

    grid = vice_gtk3_grid_new_spaced(VICE_GTK3_DEFAULT, VICE_GTK3_DEFAULT);

    /* according to the standard, doing a strcmp() with one or more `NULL`
     * arguments is implementation-defined, so better safe than sorry */
    if (prefix == NULL) {
        return grid;
    }

    if (strcmp(prefix, "ARTSTUDIO") == 0) {
        prefix = "OCP";  /* XXX: not strictly required since resource names
                                    seem to be case insensitive, but better
                                    safe than sorry */
        artstudio = 1;
    } else if (strcmp(prefix, "KOALA") == 0) {
        prefix = "Koala";
        koala = 1;
    } else if (strcmp(prefix, "MINIPAINT") == 0) {
        prefix = "Minipaint";
        minipaint = 1;
    }

    if (!koala && !artstudio && !minipaint) {
        label = gtk_label_new("No parameters required");
        g_object_set(label, "margin-left", 16, NULL);
        gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
        gtk_widget_show_all(grid);
        return grid;
    }

    /* ${FORMAT}OversizeHandling */
    label = gtk_label_new("Oversize handling");
    g_object_set(label, "margin-left", 16, NULL);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    oversize_widget = vice_gtk3_resource_combo_box_int_new_sprintf(
            "%sOversizeHandling", oversize_modes, prefix);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), oversize_widget, 1, 0, 1, 1);

    /* ${FORMAT}UndersizeHandling */
    label = gtk_label_new("Undersize handling");
    g_object_set(label, "margin-left", 16, NULL);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    undersize_widget = vice_gtk3_resource_combo_box_int_new_sprintf(
            "%sUndersizeHandling", undersize_modes, prefix);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), undersize_widget, 1, 1, 1, 1);

    /* ${FORMAT}MultiColorHandling */

    row = 2;    /* from now on, the widgets depend on machine and image type */

    /* OCPMultiColorHandling */
    if (artstudio) {
        label = gtk_label_new("Multi color handling");
        g_object_set(label, "margin-left", 16, NULL);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        multicolor_widget = vice_gtk3_resource_combo_box_int_new_sprintf(
                "%sMultiColorHandling", multicolor_modes, prefix);
        gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), multicolor_widget, 1, row, 1, 1);
        row++;
    }

    /* ${FORMAT}TEDLumaHandling */
    if (machine_class == VICE_MACHINE_PLUS4) {
        label = gtk_label_new("TED luma handling");
        g_object_set(label, "margin-left", 16, NULL);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        ted_luma_widget = vice_gtk3_resource_combo_box_int_new_sprintf(
                "%sTEDLumHandling", ted_luma_modes, prefix);
        gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), ted_luma_widget, 1, row, 1, 1);
        row++;
    }

    gtk_widget_show_all(grid);
    return grid;
}




/** \brief  Create the main 'screenshot' widget
 *
 * \return  GtkGrid
 */
static GtkWidget *create_screenshot_widget(void)
{
    GtkWidget *grid;
    GtkWidget *drv_grid;
    GtkWidget *label;
    GtkWidget *radio;
    GtkWidget *last;
    GSList *group = NULL;
    int index;
    int grid_index;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    /* grid without extra row spacing */
    drv_grid = vice_gtk3_grid_new_spaced_with_label(-1, 0, "Driver", 1);
    g_object_set(drv_grid, "margin-top", 8, "margin-left", 16, NULL);
    /* add some padding to the label */
    label = gtk_grid_get_child_at(GTK_GRID(drv_grid), 0, 0);
    g_object_set(label, "margin-bottom", 8, NULL);

    /* add drivers */
    grid_index = 1;
    last = NULL;
    for (index = 0; video_driver_list[index].name != NULL; index++) {
        const char *display = video_driver_list[index].display;
        const char *name = video_driver_list[index].name;

        if (!driver_is_video(name)) {
            radio = gtk_radio_button_new_with_label(group, display);
            g_object_set(radio, "margin-left", 8, NULL);
            gtk_radio_button_join_group(GTK_RADIO_BUTTON(radio),
                    GTK_RADIO_BUTTON(last));
            gtk_grid_attach(GTK_GRID(drv_grid), radio, 0, grid_index, 1, 1);

            /* Make PNG default (doesn't look we have any numeric define to
             * indicate PNG, so a strcmp() will have to do)
             * Also trigger the event handler to set the driver index (by
             * connecting it before this call), so I don't have to manually
             * set it here, though all this text could have been used to set it.
             *
             * Only set PNG first time the dialog is created, on second time
             * it should use whatever was used before
             */
            if (screenshot_driver_index < 0) {
                if (strcmp(name, "PNG") == 0) {
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
                    screenshot_driver_index = index; /* set this driver as
                                                        currently selected one */
                }
            } else {
                if (screenshot_driver_index == index) {
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
                }
            }

            /* connect signal *after* setting a radio button's state, to avoid
             * triggering the handler before the UI is properly set up. */
            g_signal_connect(radio, "toggled",
                    G_CALLBACK(on_screenshot_driver_toggled),
                    GINT_TO_POINTER(index));

            last = radio;
            grid_index++;
        }
    }

    /* this is where the various options go per screenshot driver (for example
     * Koala or Artstudio) */
    screenshot_options_grid = vice_gtk3_grid_new_spaced_with_label(
            -1, -1, "Driver options", 1);
    g_object_set(screenshot_options_grid,
                 "margin-top", 8, "margin-left", 16, NULL);
    gtk_grid_attach(GTK_GRID(grid), drv_grid, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), screenshot_options_grid, 1, 0, 1, 1);

    update_screenshot_options_grid(create_screenshot_param_widget(""));

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create the main 'sound recording' widget
 *
 * \return  GtkGrid
 */
static GtkWidget *create_sound_widget(void)
{
    GtkWidget *grid;
    GtkWidget *drv_grid;
    GtkWidget *label;
    GtkWidget *radio;
    GtkWidget *last;
    GSList *group = NULL;
    int index;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    drv_grid = vice_gtk3_grid_new_spaced_with_label(-1, 0, "Driver", 1);
    label = gtk_grid_get_child_at(GTK_GRID(drv_grid), 0, 0);
    g_object_set(label, "margin-bottom", 8, NULL);
    g_object_set(drv_grid, "margin-top", 8, "margin-left", 16, NULL);

    last = NULL;
    for (index = 0; audio_driver_list[index].name != NULL; index++) {
        const char *display = audio_driver_list[index].display;

        radio = gtk_radio_button_new_with_label(group, display);
        g_object_set(radio, "margin-left", 8, NULL);
        gtk_radio_button_join_group(GTK_RADIO_BUTTON(radio),
                GTK_RADIO_BUTTON(last));
        gtk_grid_attach(GTK_GRID(drv_grid), radio, 0, index + 1, 1, 1);

        /*
         * Set default audio driver, or restore previously selected audio
         * driver.
         */
        if (audio_driver_index < 0) {
            if (index == 0) {   /* 0 == WAV */
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
                audio_driver_index = index;
            }
        } else {
            if (index == audio_driver_index) {
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
            }
        }

        g_signal_connect(radio, "toggled",
                G_CALLBACK(on_audio_driver_toggled),
                GINT_TO_POINTER(index));

        last = radio;
    }

    gtk_grid_attach(GTK_GRID(grid), drv_grid, 0, 0, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create the main 'video recording' widget
 *
 * \return  GtkGrid
 */
static GtkWidget *create_video_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
#ifdef HAVE_FFMPEG
    GtkWidget *combo;
    int index;
    GtkWidget *selection_grid;
    GtkWidget *options_grid;
#endif
    grid = vice_gtk3_grid_new_spaced(16, 8);


#ifdef HAVE_FFMPEG
    label = gtk_label_new("Video driver");
    g_object_set(label, "margin-left", 16, NULL);

    combo = gtk_combo_box_text_new();
    for (index = 0; video_driver_list[index].name != NULL; index++) {
        const char *display = video_driver_list[index].display;
        const char *name = video_driver_list[index].name;

        if (driver_is_video(name)) {
            gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), name, display);
        }

        if (video_driver_index < 0) {
            video_driver_index = 0;
            gtk_combo_box_set_active(GTK_COMBO_BOX(combo), index);
        } else {
            if (video_driver_index == index) {
                gtk_combo_box_set_active(GTK_COMBO_BOX(combo), index);
            }
        }

    }
    gtk_widget_set_hexpand(combo, TRUE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

    selection_grid = vice_gtk3_grid_new_spaced_with_label
        (-1, -1, "Driver selection", 2);
    g_object_set(selection_grid, "margin-top", 8, "margin-left", 16, NULL);
    gtk_grid_set_column_spacing(GTK_GRID(selection_grid), 16);
    gtk_grid_set_row_spacing(GTK_GRID(selection_grid), 8);
    gtk_grid_attach(GTK_GRID(selection_grid), label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(selection_grid), combo, 1, 1, 1, 1);
    gtk_widget_show_all(selection_grid);

    gtk_grid_attach(GTK_GRID(grid), selection_grid, 0, 0, 1, 1);

    /* grid around ffmpeg */
    options_grid = vice_gtk3_grid_new_spaced_with_label(
            -1, -1, "Driver options", 1);
    g_object_set(options_grid, "margin-top", 8, "margin-left", 16, NULL);
    gtk_grid_set_column_spacing(GTK_GRID(options_grid), 16);
    gtk_grid_set_row_spacing(GTK_GRID(options_grid), 8);

    gtk_grid_attach(GTK_GRID(options_grid), ffmpeg_widget_create(), 0, 1, 1,1);
    video_driver_options_grid = options_grid;

    gtk_grid_attach(GTK_GRID(grid), options_grid, 0, 1, 1, 1);
#else
    label = gtk_label_new(NULL);
    gtk_label_set_line_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD);
    g_object_set(G_OBJECT(label),
            "margin-left", 16,
            "margin-right", 16,
            "margin-top", 16, NULL);
    gtk_label_set_markup(GTK_LABEL(label),
            "Video recording is unavailable due to VICE having being compiled"
            " without FFMPEG support.\nPlease recompile with either"
            " <tt>--enable-static-ffmpeg</tt> or"
            " <tt>--enable-external-ffmpeg</tt>.\n\n"
            "If you didn't compile VICE yourself, ask your provider.");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

#endif
    gtk_widget_show_all(grid);
    return grid;
}


/** \brief  Create the content widget for the dialog
 *
 * Contains a GtkStack and GtkStackSwitcher to switch between 'screenshot',
 * 'sound' and 'video' sub widgets.
 *
 * \return  GtkGrid
 */
static GtkWidget *create_content_widget(void)
{
    GtkWidget *grid;
    GtkWidget *switcher;

    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);

    stack = gtk_stack_new();
    gtk_stack_add_titled(GTK_STACK(stack), create_screenshot_widget(),
            CHILD_SCREENSHOT, "Screenshot");
    gtk_stack_add_titled(GTK_STACK(stack), create_sound_widget(),
            CHILD_SOUND, "Sound recording");
    gtk_stack_add_titled(GTK_STACK(stack), create_video_widget(),
            CHILD_VIDEO, "Video recording");
    gtk_stack_set_transition_type(GTK_STACK(stack),
            GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(stack), 1000);
    /* avoid resizing the dialog when switching */
    gtk_stack_set_homogeneous(GTK_STACK(stack), TRUE);

    switcher = gtk_stack_switcher_new();
    gtk_widget_set_halign(switcher, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(switcher, TRUE);
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher), GTK_STACK(stack));
    /* make all titles of the switcher the same size */
    gtk_box_set_homogeneous(GTK_BOX(switcher), TRUE);

    gtk_widget_show_all(stack);
    gtk_widget_show_all(switcher);

    gtk_grid_attach(GTK_GRID(grid), switcher, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), stack, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}



/** \brief  Show dialog to save screenshot, record sound and/or video
 *
 * \param[in]   parent  parent widget (unused)
 * \param[in]   data    extra data (unused)
 *
 * \return  TRUE
 */
gboolean ui_media_dialog_show(GtkWidget *parent, gpointer data)
{
    GtkWidget *dialog;
    GtkWidget *content;

    /*
     * Pause emulation
     */

    /* remember pause state before entering the widget */
    old_pause_state = ui_pause_active();

    /* pause emulation */
    ui_pause_enable();

    /* create driver list */
    if (machine_class != VICE_MACHINE_VSID) {
        create_video_driver_list();
    }

    dialog = gtk_dialog_new_with_buttons(
            "Record media file",
            ui_get_active_window(),
            GTK_DIALOG_MODAL,
            "Save", RESPONSE_SAVE,
            "Close", GTK_RESPONSE_DELETE_EVENT,
            NULL);

    /* add content widget */
    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    if (machine_class != VICE_MACHINE_VSID) {
        gtk_container_add(GTK_CONTAINER(content), create_content_widget());
    } else {
        gtk_container_add(GTK_CONTAINER(content), create_sound_widget());
    }

    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    g_signal_connect(dialog, "response", G_CALLBACK(on_response), (gpointer)dialog);
    g_signal_connect_unlocked(dialog, "destroy", G_CALLBACK(on_dialog_destroy), NULL);

    gtk_widget_show_all(dialog);
    return TRUE;
}


/** \brief  Stop audio or video recording, if active
 *
 * \param[in]   parent      parent widget (unused)
 * \param[in]   data        extra event data (unused)
 *
 * \return  TRUE, so the emulated machine doesn't get the shortcut key
 */
gboolean ui_media_stop_recording(GtkWidget *parent, gpointer data)
{
    /* stop sound recording, if active */
    if (sound_is_recording()) {
        sound_stop_recording();
    }
    /* stop video recording */
    if (screenshot_is_recording()) {
        screenshot_stop_recording();
    }

    ui_display_recording(0);
    statusbar_recording_widget_hide_all(ui_statusbar_get_recording_widget(), 10);

    return TRUE;
}


/** \brief  Callback for vsync_on_vsync_do()
 *
 * Create a screenshot on vsync to avoid tearing.
 *
 * This function is called on the VICE thread, so the canvas is retrieved in
 * ui_media_auto_screenshot() on the UI thread and passed via \a param.
 *
 * \param[in]   param   video canvas
 */
static void auto_screenshot_vsync_callback(void *param)
{
    char *filename;
    video_canvas_t *canvas = param;

    /* no need for locale bullshit */
    filename = create_proposed_screenshot_name("png");
    if (screenshot_save("PNG", filename, canvas) < 0) {
        log_error(LOG_ERR, "Failed to autosave screenshot.");
    }
}


/** \brief  Create screenshot with autogenerated filename
 *
 * Creates a PNG screenshot with an autogenerated filename with an ISO-8601-ish
 * timestamp:
 * "vice-screenshot-<year><month><day><hour><month><seconds><sec-frac>.png"
 */
void ui_media_auto_screenshot(void)
{
    if (monitor_is_inside_monitor()) {
        /* screenshot immediately if monitor is open */
        auto_screenshot_vsync_callback((void *)ui_get_active_canvas());
    } else {
        /* queue screenshot grab on vsync to avoid tearing */
        vsync_on_vsync_do(auto_screenshot_vsync_callback, (void *)ui_get_active_canvas());
    }
}


/** \brief  Clean up resources used
 *
 * Frees the last dir/file strings
 */
void ui_media_shutdown(void)
{
    lastdir_shutdown(&media_last_dir, NULL);
}
