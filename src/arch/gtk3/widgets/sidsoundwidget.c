/** \file   sidsoundwidget.c
 * \brief   Settings for SID emulation
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/* Note: These only make sense with a Sid Cartridge attached for certain machines
 *
 * $VICERES SidEngine                   all
 * $VICERES SidStereo                   all
 * $VICERES SidResidSampling            all
 * $VICERES SidResidPassband            all
 * $VICERES SidResidGain                all
 * $VICERES SidResidFilterBias          all
 * $VICERES SidResid8580Passband        all
 * $VICERES SidResid8580Gain            all
 * $VICERES SidResid8580FilterBias      all
 * $VICERES SidFilters                  all
 * $VICERES Sid2AddressStart            all
 * $VICERES Sid3AddressStart            all
 * $VICERES Sid4AddressStart            -vsid
 * $VICERES Sid5AddressStart            -vsid
 * $VICERES Sid6AddressStart            -vsid
 * $VICERES Sid7AddressStart            -vsid
 * $VICERES Sid8AddressStart            -vsid
 *  (Until PSID files support a fourth SID, this will be -vsid)
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

#include <assert.h>
#include <gtk/gtk.h>
#include <stdbool.h>

#include "vice_gtk3.h"
#include "lib.h"
#include "machine.h"
#include "resources.h"
#include "sid.h"


#ifdef HAVE_CATWEASELMKIII
# include "catweaselmkiii.h"
#endif

#ifdef HAVE_HARDSID
# include "hardsid.h"
#endif

#ifdef HAVE_PARSID
# include "parsid.h"
#endif

#include "sidenginemodelwidget.h"
#include "mixerwidget.h"

#include "sidsoundwidget.h"



#ifdef HAVE_RESID
/** \brief  Values for the "SidResidSampling" resource
 */
static const vice_gtk3_radiogroup_entry_t resid_sampling_modes[] = {
    { "Fast", 0 },
    { "Interpolation", 1 },
    { "Resampling", 2 },
    { "Fast resampling", 3 },
    { NULL, -1 }
};
#endif


/** \brief  I/O addresses for extra SID's for the C64
 *
 * \note    Yes, I know I can generate this table
 */
static const vice_gtk3_combo_entry_int_t sid_address_c64[] = {
    { "$d420", 0xd420 }, { "$d440", 0xd440 }, { "$d460", 0xd460 },
    { "$d480", 0xd480 }, { "$d4a0", 0xd4a0 }, { "$d4c0", 0xd4c0 },
    { "$d4e0", 0xd4e0 },

    { "$d500", 0xd500 }, { "$d520", 0xd520 }, { "$d540", 0xd540 },
    { "$d560", 0xd560 }, { "$d580", 0xd580 }, { "$d5a0", 0xd5a0 },
    { "$d5c0", 0xd5c0 }, { "$d5e0", 0xd5e0 },

    { "$d600", 0xd600 }, { "$d620", 0xd620 }, { "$d640", 0xd640 },
    { "$d660", 0xd660 }, { "$d680", 0xd680 }, { "$d6a0", 0xd6a0 },
    { "$d6c0", 0xd6c0 }, { "$d6e0", 0xd6e0 },

    { "$d700", 0xd700 }, { "$d720", 0xd720 }, { "$d740", 0xd740 },
    { "$d760", 0xd760 }, { "$d780", 0xd780 }, { "$d7a0", 0xd7a0 },
    { "$d7c0", 0xd7c0 }, { "$d7e0", 0xd7e0 },

    { "$de00", 0xde00 }, { "$de20", 0xde20 }, { "$de40", 0xde40 },
    { "$de60", 0xde60 }, { "$de80", 0xde80 }, { "$dea0", 0xdea0 },
    { "$dec0", 0xdec0 }, { "$dee0", 0xdee0 },

    { "$df00", 0xdf00 }, { "$df20", 0xdf20 }, { "$df40", 0xdf40 },
    { "$df60", 0xdf60 }, { "$df80", 0xdf80 }, { "$dfa0", 0xdfa0 },
    { "$dfc0", 0xdfc0 }, { "$dfe0", 0xdfe0 },
    VICE_GTK3_COMBO_ENTRY_INT_LIST_END
};


/** \brief  I/O addresses for extra SID's for the C128
 */
static const vice_gtk3_combo_entry_int_t sid_address_c128[] = {
    { "$d420", 0xd420 }, { "$d440", 0xd440 }, { "$d460", 0xd460 },
    { "$d480", 0xd480 }, { "$d4a0", 0xd4a0 }, { "$d4c0", 0xd4c0 },
    { "$d4e0", 0xd4e0 },

    { "$d700", 0xd700 }, { "$d720", 0xd720 }, { "$d740", 0xd740 },
    { "$d760", 0xd760 }, { "$d780", 0xd780 }, { "$d7a0", 0xd7a0 },
    { "$d7c0", 0xd7c0 }, { "$d7e0", 0xd7e0 },

    { "$de00", 0xde00 }, { "$de20", 0xde20 }, { "$de40", 0xde40 },
    { "$de60", 0xde60 }, { "$de80", 0xde80 }, { "$dea0", 0xdea0 },
    { "$dec0", 0xdec0 }, { "$dee0", 0xdee0 },

    { "$df00", 0xdf00 }, { "$df20", 0xdf20 }, { "$df40", 0xdf40 },
    { "$df60", 0xdf60 }, { "$df80", 0xdf80 }, { "$dfa0", 0xdfa0 },
    { "$dfc0", 0xdfc0 }, { "$dfe0", 0xdfe0 },
    VICE_GTK3_COMBO_ENTRY_INT_LIST_END
};


#ifdef HAVE_RESID
/** \brief  Reference to resid sampling widget
 *
 * Used to enable/disable when the SID engine changes
 */
static GtkWidget *resid_sampling;

/** \brief  Reference to the SidResidPassband widget
 *
 * Used to enable/disable the widget based on the SidFilters setting
 */
static GtkWidget *resid_6581_passband;

/** \brief  Reference to the SidResidGain widget
 *
 * Used to enable/disable the widget based on the SidFilters setting
 */
static GtkWidget *resid_6581_gain;

/** \brief  Reference to the SidResidFilterBias widget
 *
 * Used to enable/disable the widget based on the SidFilters setting
 */
static GtkWidget *resid_6581_bias;

/** \brief  Reference to the SidResidPassband widget
 *
 * Used to enable/disable the widget based on the SidFilters setting
 */
static GtkWidget *resid_8580_passband;

/** \brief  Reference to the SidResidGain widget
 *
 * Used to enable/disable the widget based on the SidFilters setting
 */
static GtkWidget *resid_8580_gain;

/** \brief  Reference to the SidResidFilterBias widget
 *
 * Used to enable/disable the widget based on the SidFilters setting
 */
static GtkWidget *resid_8580_bias;
#endif

/** \brief  Reference to the extra SID address widgets
 *
 * Used to enable/disable depending on the number of SIDs active
 */
static GtkWidget *address_widgets[SOUND_SIDS_MAX];


/** \brief  Reference to the SID filters checkbox
 */
GtkWidget *filters;

/* only used with the ReSID engine */
#ifdef HAVE_RESID

/** \brief  6581 Passband reset button */
static GtkWidget *resid_6581_passband_button;

/** \brief  6581 Gain reset button */
static GtkWidget *resid_6581_gain_button;

/** \brief  6581 Bias reset button */
static GtkWidget *resid_6581_bias_button;

/** \brief  8580 Passband reset button */
static GtkWidget *resid_8580_passband_button;

/** \brief  8580 Gain reset button */
static GtkWidget *resid_8580_gain_button;

/** \brief  8580 Bias reset button */
static GtkWidget *resid_8580_bias_button;

/** \brief  6581 widgets grid */
static GtkWidget *resid_6581_grid;

/** \brief  8580 widgets grid */
static GtkWidget *resid_8580_grid;

/** \brief  6581 Passband spin button */
static GtkWidget *resid_6581_passband_spin;

/** \brief  6581 Gain spin button */
static GtkWidget *resid_6581_gain_spin;

/** \brief  6581 Bias spin button */
static GtkWidget *resid_6581_bias_spin;

#endif

/** \brief  Number of extra SIDs widget */
static GtkWidget *num_sids_widget;


/** \brief  Extra callback registered to the 'number of SIDs' radiogroup
 *
 * XXX: This function is also used in the constructor of the main widget to
 *      set the initial sensitivity of the address widgets, with NULL passed
 *      as the widget.
 *      I should probably refactor that code to have a separate
 *      function to set sensitivity to avoid this hack.
 *
 * \param[in]   widget  widget triggering the event
 * \param[in]   data   number of extra SIDs (0 - SOUND_SID_MAX-1)
 */
static void on_sid_count_changed(GtkWidget *widget, gpointer data)
{
    int count;

    if (widget == NULL) {
        /* called from main widget constructor: count is in `data` */
        count = GPOINTER_TO_INT(data);
    } else {
        /* called from an event, use the spin button's value */
        count = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
    }

    if (sid_machine_can_have_multiple_sids()) {
        gtk_widget_set_sensitive(address_widgets[0], count > 0);
        gtk_widget_set_sensitive(address_widgets[1], count > 1);
        if (machine_class != VICE_MACHINE_VSID) {
            gtk_widget_set_sensitive(address_widgets[2], count > 2);
            gtk_widget_set_sensitive(address_widgets[3], count > 3);
            gtk_widget_set_sensitive(address_widgets[4], count > 4);
            gtk_widget_set_sensitive(address_widgets[5], count > 5);
            gtk_widget_set_sensitive(address_widgets[6], count > 6);
        }
    }
}


#ifdef HAVE_RESID
/** \brief  Handler for "clicked" event of reset-to-default for passband
 *
 * Restores the ReSID passband slider back to default
 *
 * \param[in]   widget      button
 * \param[in]   user_data   extra data for event (unused)
 */
static void on_resid_6581_passband_default_clicked(GtkWidget *widget,
                                                   gpointer user_data)
{
    vice_gtk3_resource_scale_int_reset(resid_6581_passband);
}


/** \brief  Handler for "clicked" event of reset-to-default for gain
 *
 * Restores the ReSID gain slider back to default
 *
 * \param[in]   widget      button
 * \param[in]   user_data   extra data for event (unused)
 */
static void on_resid_6581_gain_default_clicked(GtkWidget *widget,
                                               gpointer user_data)
{
    vice_gtk3_resource_scale_int_reset(resid_6581_gain);
}


/** \brief  Handler for "clicked" event of reset-to-default for filter bias
 *
 * Restores the ReSID filter bias slider back to default
 *
 * \param[in]   widget      button
 * \param[in]   user_data   extra data for event (unused)
 */
static void on_resid_6581_bias_default_clicked(GtkWidget *widget,
                                               gpointer user_data)
{
    vice_gtk3_resource_scale_int_reset(resid_6581_bias);

}

/** \brief  Handler for "clicked" event of reset-to-default for 8580 passband
 *
 * Restores the 8580 ReSID passband slider back to default
 *
 * \param[in]   widget      button
 * \param[in]   user_data   extra data for event (unused)
 */
static void on_resid_8580_passband_default_clicked(GtkWidget *widget,
                                              gpointer user_data)
{
    vice_gtk3_resource_scale_int_reset(resid_8580_passband);
}


/** \brief  Handler for "clicked" event of reset-to-default for 8580 gain
 *
 * Restores the 8580 ReSID gain slider back to default
 *
 * \param[in]   widget      button
 * \param[in]   user_data   extra data for event (unused)
 */
static void on_resid_8580_gain_default_clicked(GtkWidget *widget,
                                          gpointer user_data)
{
    vice_gtk3_resource_scale_int_reset(resid_8580_gain);
}


/** \brief  Handler for "clicked" event of reset-to-default for 8580 filter bias
 *
 * Restores the 8580 ReSID filter bias slider back to default
 *
 * \param[in]   widget      button
 * \param[in]   user_data   extra data for event (unused)
 */
static void on_resid_8580_bias_default_clicked(GtkWidget *widget,
                                          gpointer user_data)
{
    vice_gtk3_resource_scale_int_reset(resid_8580_bias);

}
#endif

#ifdef HAVE_RESID
static void on_spin_value_changed(GtkWidget *spin, gpointer data)
{
    vice_gtk3_resource_scale_int_sync(data);
}
#endif


#ifdef HAVE_RESID
/** \brief  Create spinbutton controlling \a resource and updating \a slider
 *
 * \param[in]       resource    resource name
 * \param[in]       low         spinbox lowest value
 * \param[in]       high        spinbox highest value
 * \param[in]       step        spinbox step
 * \param[in,out]   slider      slider widget controlling \a resource
 *
 * \return  GtkSpinBox
 */
static GtkWidget *create_spin(
        const char *resource,
        int low, int high, int step,
        GtkWidget *slider)
{
    GtkWidget *spin;

    spin = vice_gtk3_resource_spin_int_new(resource, low, high, step);
    g_signal_connect(spin, "value-changed", G_CALLBACK(on_spin_value_changed),
            slider);
    /* add a bit of spacing (TODO: mabye use CSS?) */
    g_object_set(G_OBJECT(spin), "margin-left", 16 , NULL);
    return spin;
}
#endif


/** \brief  Extra callback for the SID engine/model widget
 *
 * \param[in]   engine  SID engine ID
 * \param[in]   model   SID model ID
 */
static void engine_model_changed_callback(int engine, int model)
{
#ifdef HAVE_RESID
    gboolean is_resid = engine == SID_ENGINE_RESID;
#endif
    /* Show proper ReSID slider widgets
     *
     * We can't check old model vs new model here, since the resource
     * SidModel has already been updated.
     */
#ifdef HAVE_RESID
    if (model == 1 || model == 2) {
        gtk_widget_show(resid_8580_grid);
        gtk_widget_hide(resid_6581_grid);
    } else {
        gtk_widget_show(resid_6581_grid);
        gtk_widget_hide(resid_8580_grid);
    }

    /*
     * Update mixer widget in the statusbar
     */
    mixer_widget_sid_type_changed();
#endif

#ifdef HAVE_RESID
    gtk_widget_set_sensitive(filters, is_resid);
    gtk_widget_set_sensitive(resid_6581_grid, is_resid);
    gtk_widget_set_sensitive(resid_8580_grid, is_resid);
    gtk_widget_set_sensitive(resid_sampling, is_resid);
#endif
}


#ifdef HAVE_RESID
/** \brief  Create widget to control ReSID sampling method
 *
 * \return  GtkGrid
 */
static GtkWidget *create_resid_sampling_widget(void)
{
    GtkWidget *grid;
    GtkWidget *label;
    GtkWidget *radio_group;

    grid = gtk_grid_new();
    g_object_set(grid, "margin-left", 8, NULL);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>ReSID sampling method</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    g_object_set(label, "margin-bottom", 8, NULL);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    radio_group = vice_gtk3_resource_radiogroup_new("SidResidSampling",
            resid_sampling_modes, GTK_ORIENTATION_VERTICAL);
    g_object_set(radio_group, "margin-left", 16, NULL);
    gtk_grid_attach(GTK_GRID(grid), radio_group, 0, 1, 1, 1);

    gtk_widget_show_all(grid);
    return grid;
}
#endif


/** \brief  Create widget to set the number of emulated SID's
 *
 * \return  GtkGrid
 */
static GtkWidget *create_num_sids_widget(void)
{
    GtkWidget *grid;
    GtkWidget *spin;
    int max_sids;

    grid = vice_gtk3_grid_new_spaced_with_label(16, 8, "Extra SIDs", 2);

    if (machine_class == VICE_MACHINE_VSID) {
        /* FIXME: perhaps rename to SOUNDS_SIDS_MAX_[P|V]SID? */
        max_sids = SID_COUNT_MAX_PSID;
    } else {
        max_sids = SOUND_SIDS_MAX;
    }

    /* create spinbutton for the 'SidStereo' resource */
    spin = vice_gtk3_resource_spin_int_new("SidStereo", 0, max_sids - 1, 1);
    g_object_set(G_OBJECT(spin), "margin-left", 16, NULL);
    gtk_widget_set_halign(spin, GTK_ALIGN_START);
    gtk_widget_set_hexpand(spin, FALSE);
    g_signal_connect(spin, "value-changed", G_CALLBACK(on_sid_count_changed),
             NULL);
    gtk_grid_attach(GTK_GRID(grid), spin, 0, 1, 1, 1);
    gtk_widget_show_all(grid);

    num_sids_widget = spin;
    return grid;
}


/** \brief  Create widget for extra SID addresses
 *
 * \param[in]   sid     extra SID number (1-7)
 *
 * \return  GtkGrid
 */
static GtkWidget *create_extra_sid_address_widget(int sid)
{
    GtkWidget *widget;
    const vice_gtk3_combo_entry_int_t *entries;
    char label[256];
    char *resource_name;

    resource_name = lib_msprintf("Sid%dAddressStart", sid + 1);

    g_snprintf(label, 256, "SID #%d", sid + 1);
    entries = machine_class == VICE_MACHINE_C128
        ? sid_address_c128 : sid_address_c64;

    widget = vice_gtk3_resource_combo_box_int_new_with_label(resource_name,
                                                             entries,
                                                             label);
    gtk_widget_show_all(widget);

    lib_free(resource_name);

    return widget;
}


#ifdef HAVE_RESID

static void on_resid_6581_passband_change(GtkWidget *widget, gpointer data)
{
    double value = gtk_range_get_value(GTK_RANGE(widget));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(resid_6581_passband_spin), value);
}


static void on_resid_6581_gain_change(GtkWidget *widget, gpointer data)
{
    double value = gtk_range_get_value(GTK_RANGE(widget));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(resid_6581_gain_spin), value);
}


static void on_resid_6581_bias_change(GtkWidget *widget, gpointer data)
{
    double value = gtk_range_get_value(GTK_RANGE(widget));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(resid_6581_bias_spin), value);
}


static void on_resid_6581_passband_spin_change(GtkWidget *widget, gpointer data)
{
    double value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
    gtk_range_set_value(GTK_RANGE(resid_6581_passband), value);
}


static void on_resid_6581_gain_spin_change(GtkWidget *widget, gpointer data)
{
    double value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
    gtk_range_set_value(GTK_RANGE(resid_6581_gain), value);
}


static void on_resid_6581_bias_spin_change(GtkWidget *widget, gpointer data)
{
    double value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
    gtk_range_set_value(GTK_RANGE(resid_6581_bias), value);
}
#endif



#ifdef HAVE_RESID
/** \brief  Create scale to control the "SidResidPassband" resource
 *
 * \return  GtkScale
 */
static GtkWidget *create_resid_6581_passband_widget(void)
{
    GtkWidget *scale;

    scale = vice_gtk3_resource_scale_int_new("SidResidPassband",
            GTK_ORIENTATION_HORIZONTAL, 0, 90, 5);

    g_signal_connect(scale, "value-changed",
            G_CALLBACK(on_resid_6581_passband_change),
            NULL);
    return scale;
}


static GtkWidget *create_resid_6581_passband_spin(GtkWidget *slider)
{
    GtkWidget *spin;
    spin = create_spin("SidReSidPassBand", 0, 90,1, slider);

    g_signal_connect(
            spin,
            "value-changed",
            G_CALLBACK(on_resid_6581_passband_spin_change),
            NULL);
    return spin;
}


static GtkWidget *create_resid_6581_gain_spin(GtkWidget *slider)
{
    GtkWidget *spin;
    spin = create_spin("SidReSidGain", 90, 100, 1, slider);

    g_signal_connect(
            spin,
            "value-changed",
            G_CALLBACK(on_resid_6581_gain_spin_change),
            NULL);
    return spin;
}


static GtkWidget *create_resid_6581_bias_spin(GtkWidget *slider)
{
    GtkWidget *spin;
    spin = create_spin("SidReSidFilterBias", -5000, 5000, 1, slider);

    g_signal_connect(
            spin,
            "value-changed",
            G_CALLBACK(on_resid_6581_bias_spin_change),
            NULL);
    return spin;
}


/** \brief  Create scale to control the "SidResidGain" resource
 *
 * \return  GtkScale
 */
static GtkWidget *create_resid_6581_gain_widget(void)
{
    GtkWidget *scale;

    scale = vice_gtk3_resource_scale_int_new("SidResidGain",
            GTK_ORIENTATION_HORIZONTAL, 90, 100, 1);

    g_signal_connect(scale, "value-changed",
            G_CALLBACK(on_resid_6581_gain_change),
            NULL);
    return scale;
}


/** \brief  Create scale to control the "SidResidFilterBias" resource
 *
 * \return  GtkScale
 */
static GtkWidget *create_resid_6581_bias_widget(void)
{
    GtkWidget *scale;

    scale = vice_gtk3_resource_scale_int_new("SidResidFilterBias",
            GTK_ORIENTATION_HORIZONTAL, -5000, 5000, 1);
    g_signal_connect(scale, "value-changed",
            G_CALLBACK(on_resid_6581_bias_change),
            NULL);
    return scale;
}

/** \brief  Create scale to control the "SidResid8580Passband" resource
 *
 * \return  GtkScale
 */
static GtkWidget *create_resid_8580_passband_widget(void)
{
    GtkWidget *scale;

    scale = vice_gtk3_resource_scale_int_new("SidResid8580Passband",
            GTK_ORIENTATION_HORIZONTAL, 0, 90, 1);
    return scale;
}


/** \brief  Create scale to control the "SidResid8580Gain" resource
 *
 * \return  GtkScale
 */
static GtkWidget *create_resid_8580_gain_widget(void)
{
    GtkWidget *scale;

    scale = vice_gtk3_resource_scale_int_new("SidResid8580Gain",
            GTK_ORIENTATION_HORIZONTAL, 90, 100, 1);
    return scale;
}


/** \brief  Create scale to control the "SidResid8580FilterBias" resource
 *
 * \return  GtkScale
 */
static GtkWidget *create_resid_8580_bias_widget(void)
{
    GtkWidget *scale;

    scale = vice_gtk3_resource_scale_int_new("SidResid8580FilterBias",
            GTK_ORIENTATION_HORIZONTAL, -5000, 5000, 1);
/*    vice_gtk3_resource_scale_int_set_marks(scale, 500); */
    return scale;
}
#endif


#ifdef HAVE_RESID
/** \brief  Create "Reset to default" button
 *
 * \param[in]   callback    callback for the button
 *
 * \return  GtkButton
 */
static GtkWidget *create_resource_reset_button(
        void (*callback)(GtkWidget *, gpointer))
{
    GtkWidget *button;

    button = gtk_button_new_with_label("Reset");
    gtk_widget_set_valign(button, GTK_ALIGN_END);
    gtk_widget_set_hexpand(button, FALSE);
    g_object_set(button, "margin-left", 16, NULL);

    g_signal_connect(button, "clicked", G_CALLBACK(callback), NULL);

    gtk_widget_show(button);
    return button;
}
#endif


/** \brief  Create widget to control SID settings
 *
 * \param[in]   parent  parent widget (unused)
 *
 * \return  GtkGrid
 */
GtkWidget *sid_sound_widget_create(GtkWidget *parent)
{
    GtkWidget *layout;
    GtkWidget *label;
    GtkWidget *engine;
    GtkWidget *sids = NULL;
    int current_engine;
    int stereo;
    int row = 2;
    int i;
    int model;
#ifdef HAVE_RESID
    int is_resid;
#endif

    layout = vice_gtk3_grid_new_spaced(8, VICE_GTK3_DEFAULT);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>SID settings</b>");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(layout), label, 0, 0, 3, 1);

    resources_get_int("SidModel", &model);

    engine = sid_engine_model_widget_create();
    sid_engine_model_widget_set_callback(engine_model_changed_callback);
    gtk_grid_attach(GTK_GRID(layout), engine, 0, 1, 1,1);
#ifdef HAVE_RESID
    resid_sampling = create_resid_sampling_widget();
    gtk_grid_attach(GTK_GRID(layout), resid_sampling, 1, 1, 1, 1);
#endif
    resources_get_int("SidEngine", &current_engine);
#ifdef HAVE_RESID
    is_resid = current_engine == SID_ENGINE_RESID;
#endif

    if (sid_machine_can_have_multiple_sids()) {
        int max = sid_machine_get_max_sids();
        int c = 1;
        GtkWidget *sid_addresses;

        sids = create_num_sids_widget();
        gtk_grid_attach(GTK_GRID(layout), sids, 2, 1, 1, 1);

        for (i = 1; i < max; i++) {

            address_widgets[i - 1] = create_extra_sid_address_widget(i);
        }

        sid_addresses = vice_gtk3_grid_new_spaced_with_label(
                16, 8, "SID I/O addresses", 3);
        g_object_set(sid_addresses,
                "margin-left", 16,
                "margin-top", 16,
                "margin-bottom", 16,
                NULL);

        /* lay out address widgets in a grid of four columns max, skip the
         * first SID
         */
        i = 0;
        while (i < max - 1) {
            while ((c < 4) && (i < max -1)) {
                gtk_grid_attach(GTK_GRID(sid_addresses),
                                address_widgets[i],
                                c, ((i + 1) / 4) + 1, 1, 1);
                c++;
                i++;
            }
            c = 0;
        }
        gtk_grid_attach(GTK_GRID(layout), sid_addresses, 0, row++, 3, 1);
    } else {
        row = 2;
    }

#ifdef HAVE_RESID
    filters = vice_gtk3_resource_check_button_new("SidFilters",
            "Enable SID filter emulation");
    gtk_grid_attach(GTK_GRID(layout), filters, 0, row, 3, 1);
#endif

#ifdef HAVE_RESID
    gtk_widget_set_sensitive(resid_sampling, current_engine == SID_ENGINE_RESID);
    gtk_widget_set_sensitive(filters, current_engine == SID_ENGINE_RESID);
#endif


#ifdef HAVE_RESID
    /* TODO:    check engine as well (hardSID)
     *          Also somehow delete and replace 6581/8580 mixer widget when
     *          changing model, so this has to go, mostly.
     */

    resid_6581_grid = gtk_grid_new();
    resid_8580_grid = gtk_grid_new();
    g_object_set(resid_6581_grid, "margin", 0, NULL);
    g_object_set(resid_8580_grid, "margin", 0, NULL);

    /* 8580 */

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>ReSID 8580 filter settings</b>");
    gtk_widget_show(label);
    gtk_grid_attach(GTK_GRID(resid_8580_grid), label, 0, 0, 3, 1);

    /* 8580 passband */
    label = gtk_label_new("8580 passband");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    g_object_set(label, "margin-left", 16, NULL);
    resid_8580_passband = create_resid_8580_passband_widget();
    resid_8580_passband_button = create_resource_reset_button(
            on_resid_8580_passband_default_clicked);
    gtk_grid_attach(GTK_GRID(resid_8580_grid),
            label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(resid_8580_grid),
            resid_8580_passband, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(resid_8580_grid),
            resid_8580_passband_button, 3, 1, 1, 1);

    /* We need to do this due to show_all getting disabled for the containing
     * GtkGrid and the other widgets (slider, button) having their own call
     * to Show() except this simple label:
     */
    gtk_widget_show(label);

    /* 8580 gain */
    label = gtk_label_new("8580 gain");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    g_object_set(label, "margin-left", 16, NULL);
    resid_8580_gain = create_resid_8580_gain_widget();
    resid_8580_gain_button = create_resource_reset_button(
            on_resid_8580_gain_default_clicked);
    gtk_grid_attach(GTK_GRID(resid_8580_grid),
            label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(resid_8580_grid),
            resid_8580_gain, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(resid_8580_grid),
            resid_8580_gain_button, 3, 2, 1, 1);
    gtk_widget_show(label);

    /* 8580 bias */
    label = gtk_label_new("8580 filter bias");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    g_object_set(label, "margin-left", 16, NULL);
    resid_8580_bias = create_resid_8580_bias_widget();
    resid_8580_bias_button = create_resource_reset_button(
            on_resid_8580_bias_default_clicked);
    gtk_grid_attach(GTK_GRID(resid_8580_grid),
            label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(resid_8580_grid),
            resid_8580_bias, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(resid_8580_grid),
            resid_8580_bias_button, 3, 3, 1, 1);
    gtk_widget_show(label);

    /* 6581 (YAY!) */

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<b>ReSID 6581 filter settings</b>");
    gtk_widget_show(label);
    gtk_grid_attach(GTK_GRID(resid_6581_grid), label, 0, 0, 3, 1);

    /* 6581 passband */
    label = gtk_label_new("6581 passband");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    g_object_set(label, "margin-left", 16, NULL);
    resid_6581_passband = create_resid_6581_passband_widget();
    resid_6581_passband_button = create_resource_reset_button(
            on_resid_6581_passband_default_clicked);
    resid_6581_passband_spin = create_resid_6581_passband_spin(resid_6581_passband);
    gtk_grid_attach(GTK_GRID(resid_6581_grid),
            label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(resid_6581_grid),
            resid_6581_passband, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(resid_6581_grid),
            resid_6581_passband_spin, 2, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(resid_6581_grid),
            resid_6581_passband_button, 3, 1, 1, 1);
    gtk_widget_show(label);

    /* 6581 gain */
    label = gtk_label_new("6581 gain");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    g_object_set(label, "margin-left", 16, NULL);
    resid_6581_gain = create_resid_6581_gain_widget();
    resid_6581_gain_button = create_resource_reset_button(
            on_resid_6581_gain_default_clicked);
    resid_6581_gain_spin = create_resid_6581_gain_spin(resid_6581_gain);
    gtk_grid_attach(GTK_GRID(resid_6581_grid),
            label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(resid_6581_grid),
            resid_6581_gain, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(resid_6581_grid),
            resid_6581_gain_spin, 2, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(resid_6581_grid),
            resid_6581_gain_button, 3, 2, 1, 1);
    gtk_widget_show(label);

    /* 6581 bias */
    label = gtk_label_new("6581 filter bias");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    g_object_set(label, "margin-left", 16, NULL);
    resid_6581_bias = create_resid_6581_bias_widget();
    resid_6581_bias_button = create_resource_reset_button(
            on_resid_6581_bias_default_clicked);
    resid_6581_bias_spin = create_resid_6581_bias_spin(resid_6581_bias);
    gtk_grid_attach(GTK_GRID(resid_6581_grid),
            label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(resid_6581_grid),
            resid_6581_bias, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(resid_6581_grid),
            resid_6581_bias_spin, 2, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(resid_6581_grid),
            resid_6581_bias_button, 3, 3, 1, 1);
    gtk_widget_show(label);

    /* force expansion */
    gtk_widget_set_hexpand(resid_6581_gain, TRUE);
    gtk_widget_set_hexpand(resid_8580_gain, TRUE);

    gtk_widget_set_hexpand(resid_6581_grid, TRUE);
    gtk_grid_attach(GTK_GRID(layout), resid_6581_grid, 0, row + 1, 3 ,1);
    gtk_grid_attach(GTK_GRID(layout), resid_8580_grid, 0, row + 2, 3, 1);

#endif

    if (machine_class != VICE_MACHINE_PLUS4
            && machine_class != VICE_MACHINE_CBM5x0
            && machine_class != VICE_MACHINE_CBM6x0)
    {
        /* set sensitivity of address widgets */
        resources_get_int("SidStereo", &stereo);
        on_sid_count_changed(NULL, GINT_TO_POINTER(stereo));
    }

#ifdef HAVE_RESID
    /* only enable proper widgets */
    gtk_widget_set_no_show_all(resid_6581_grid, TRUE);
    gtk_widget_set_no_show_all(resid_8580_grid, TRUE);
    if (model == 1 || model == 2) {
        gtk_widget_show(resid_8580_grid);
        gtk_widget_hide(resid_6581_grid);
    } else {
        gtk_widget_show(resid_6581_grid);
        gtk_widget_hide(resid_8580_grid);
    }

    gtk_widget_set_sensitive(resid_6581_grid, is_resid);
    gtk_widget_set_sensitive(resid_8580_grid, is_resid);
#endif
    gtk_widget_show_all(layout);

    return layout;
}
