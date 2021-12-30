/** \file   widgethelpers.c
 * \brief   Helpers for creating Gtk3 widgets
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * This file is supposed to contain some helper functions for boiler plate
 * code, such as creating layout widgets, creating lists of radio or check
 * boxes, etc.
 *
 * \todo    turn the margin/padding values into defines and move into a file
 *          like uidefs.h (partially done, \a see vice_gtk3_settings.h)
 *
 * \todo    rename/replace functions (partially done)
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
#include <string.h>
#include <stdbool.h>

#include "lib.h"
#include "resources.h"

#include "vice_gtk3_settings.h"
#include "debug_gtk3.h"

#include "widgethelpers.h"



/** \brief  Get index of \a value in \a list
 *
 * Get the index in \a list for \a value. This function is required for custom
 * radiogroups that add 'unknown' options or something similar.
 *
 * \param[in]   list    radio button group array
 * \param[in]   value   value to find in \a list
 *
 * \return  index of \a value or -1 when not found
 */
int vice_gtk3_radiogroup_get_list_index(
        const vice_gtk3_radiogroup_entry_t *list,
        int value)
{
    int i;

    for (i = 0; list[i].name != NULL; i++) {
        if (list[i].id == value) {
            return i;
        }
    }
    return -1;
}


/** \brief  Set a radio button to active in a GktGrid
 *
 * This function only checks for radio buttons in the first row of the \a grid,
 * so it works fine with widgets created through
 * uihelpers_uihelpers_create_int_radiogroup_with_label(), but not much else.
 * So it might need some refactoring
 *
 * \param[in]   grid    GtkGrid containing radio buttons
 * \param[in]   index   index of the radio button (the actual index of the
 *                      radio button, other widgets are skipped)
 */
void vice_gtk3_radiogroup_set_index(GtkWidget *grid, int index)
{
    GtkWidget *radio;
    int row = 0;
    int radio_index = 0;

    if (index < 0) {
        debug_gtk3("Warning: negative index given, giving up.");
        return;
    }

    do {
        radio = gtk_grid_get_child_at(GTK_GRID(grid), 0, row);
        if (GTK_IS_TOGGLE_BUTTON(radio)) {
            if (radio_index == index) {
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
                return;
            }
            radio_index++;
        }
        row++;
    } while (radio != NULL);
}


/** \brief  Create a left-aligned, 16 units indented label
 *
 * XXX: This function is of little use an should probably be removed in favour
 *      of something a little more flexible.
 *
 * \param[in]   text    label text
 *
 * \return  GtkLabel
  */
GtkWidget *vice_gtk3_create_indented_label(const char *text)
{
    GtkWidget *label = gtk_label_new(text);

    gtk_widget_set_halign(label, GTK_ALIGN_START);
    g_object_set(label, "margin-left", 16, NULL);
    return label;
}


/** \brief  Create a new `GtkGrid`, setting column and row spacing
 *
 * \param[in]   column_spacing  column spacing (< 0 to use default)
 * \param[in]   row_spacing     row spacing (< 0 to use default)
 *
 * \return  GtkGrid
 */
GtkWidget *vice_gtk3_grid_new_spaced(int column_spacing, int row_spacing)
{
    GtkWidget *grid = gtk_grid_new();

    gtk_grid_set_column_spacing(GTK_GRID(grid),
            column_spacing < 0 ? VICE_GTK3_GRID_COLUMN_SPACING : column_spacing);
    gtk_grid_set_row_spacing(GTK_GRID(grid),
            row_spacing < 0 ? VICE_GTK3_GRID_ROW_SPACING : row_spacing);
    return grid;
}


/** \brief  Create a new `GtkGrid` with label, setting column and row spacing
 *
 * \param[in]   column_spacing  column spacing (< 0 to use default)
 * \param[in]   row_spacing     row spacing (< 0 to use default)
 * \param[in]   label           label text
 * \param[in]   span            number of columns for the \a label to span
 *
 * \return  GtkGrid
 */
GtkWidget *vice_gtk3_grid_new_spaced_with_label(int column_spacing,
                                                int row_spacing,
                                                const char *label,
                                                int span)
{
    GtkWidget *grid = vice_gtk3_grid_new_spaced(column_spacing, row_spacing);
    GtkWidget *lbl = gtk_label_new(NULL);
    char *temp;

    if (span < 1) {
        span = 1;
    }

    /* create left-indented bold label */
    temp = lib_msprintf("<b>%s</b>", label);
    gtk_label_set_markup(GTK_LABEL(lbl), temp);
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    /* g_object_set(lbl, "margin-bottom", 8, NULL); */
    lib_free(temp);

    /* attach label */
    gtk_grid_attach(GTK_GRID(grid), lbl, 0, 0, span, 1);
    gtk_widget_show(grid);
    return grid;
}


/** \brief  Set margin on \a grid
 *
 * Set margins on a GtkGrid. passing a value of <0 means skipping that property.
 *
 * \param[in,out]   grid    GtkGrid instance
 * \param[in]       top     top margin
 * \param[in]       bottom  bottom margin
 * \param[in]       left    left margin
 * \param[in]       right   right margin
 *
 */
void vice_gtk3_grid_set_margins(GtkWidget *grid,
                                gint top,
                                gint bottom,
                                gint left,
                                gint right)
{
    if (top >= 0) {
        g_object_set(grid, "margin-top", top, NULL);
    }
    if (bottom >= 0) {
        g_object_set(grid, "margin-bottom", bottom, NULL);
    }
    if (left >= 0) {
        g_object_set(grid, "margin-left", left, NULL);
    }
    if (right >= 0) {
        g_object_set(grid, "margin-right", right, NULL);
    }
}


/** \brief  Convert petscii encoded string to utf8 string we can show using the CBM font
 *
 * this function handles all characters that may appear in a directory listing,
 * including "non printable" control characters, which appear as inverted characters
 * in so called "quote mode".
 *
 * \param[in]   s           PETSCII string to convert to UTF-8
 * \param[in]   inverted    use inverted mode
 * \param[in]   lowercase   use the lowercase chargen
 *
 * \return  heap-allocated UTF-8 string, free with lib_free()
 *
 * \note    only valid for the "C64_Pro_Mono-STYLE.ttf" font, not the old
 *          "CBM.ttf" font.
 *
 * \note    Somehow the inverted space has a line on top on at least Linux,
 *          the codepoint seems fine though, so perhaps a bug in Pango?
 */
unsigned char *vice_gtk3_petscii_to_utf8(unsigned char *s,
                                         bool inverted,
                                         bool lowercase)
{
    unsigned char *d, *r;
    unsigned int codepoint;

    r = d = lib_malloc((size_t)(strlen((char *)s) * 3 + 1));
#if 0
    debug_gtk3("Input: '%s'", s);
#ifdef HAVE_DEBUG_GTK3UI
    unsigned char *t = s;
    while (*t) {
        printf(" %02x", *t++);
    }
    putchar('\n');
#endif
#endif
    while (*s) {

        /* 0xe000-0xe0ff codepoints cover the regular, uppercase, petscii codes
                         in ranges 0x20-0x7f and 0xa0-0xff
           0xe200-0xe2ff codepoints cover the same characters, but contain the
                         respective inverted glyphs.

           regular valid petscii codes are converted as is, petscii control
           codes will produce the glyph that the petscii code would produce
           in so called "quote mode".
        */

        /* first convert petscii to utf8 codepoint */
        if (*s < 0x20) {
            /* petscii 0x00-0x1f  control codes (inverted @ABC..etc) */
            codepoint = *s + 0xe240;            /* 0xe240-0xe25f */
        } else if (*s < 0x80) {
            /* petscii 0x20-0x7f  printable petscii codes */
            codepoint = *s + 0xe000;            /* 0xe020-0xe07f */
        } else if (*s < 0xa0) {
            /* petscii 0x80-0x9f  control codes (inverted SHIFT+@ABC..etc) */
            codepoint = (*s - 0x80) + 0xe260;   /* 0xe260-0xe27f */
        } else {
            /* petscii 0xa0-0xff  printable petscii codes */
            codepoint = *s + 0xe000;            /* 0xe0a0-0xe0ff */
        }
        if (inverted) {
            codepoint ^= 0x0200;                /* 0xe0XX <-> 0xe2XX */
        }
        /* switch to lower case if requested */
        if (lowercase) {
            codepoint ^= 0x0100;
        }
        s++;

#if 0
        if (codepoint == 0xe220) {
            codepoint = 0xeee4;
        }
#endif
        /* now copy to the destination string and convert to utf8 */
        /* we can get away with just this, because all codepoints are > 4095 */
        /* three byte form - 1110xxxx 10xxxxxx 10xxxxxx */
        *d++ = 0xe0 | ((codepoint >> 12) & 0x0f);
        *d++ = 0x80 | ((codepoint >> (6)) & 0x3f);
        *d   = 0x80 | ((codepoint >> (0)) & 0x3f);
        d++;
    }
    *d = '\0';
#if 0
    debug_gtk3("Result: ");
#ifdef HAVE_DEBUG_GTK3UI
    t = r;
    while (*t) {
        printf(" %02x", *t++);
    }
    putchar('\n');
#endif
#endif
    return r;
}

