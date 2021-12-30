/** \file   carthelpers.c
 * \brief   Cartridge helper functions
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 *
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

#include "machine.h"
#include "resources.h"
#include "lib.h"
#include "debug_gtk3.h"
#include "basedialogs.h"
#include "cartridge.h"

#include "carthelpers.h"

/*
 * Function pointers, used by various other source files. These need to remain
 * public.
 */

/** \brief  Cartridge save function pointer */
int (*carthelpers_save_func)(int type, const char *filename);

/** \brief  Cartridge flush function pointer */
int (*carthelpers_flush_func)(int type);

/** \brief  Cartridge is-enabled function pointer */
int (*carthelpers_is_enabled_func)(int type);

/** \brief  Cartridge enable function pointer */
int (*carthelpers_enable_func)(int type);

/** \brief  Cartridge disable function pointer */
int (*carthelpers_disable_func)(int type);

/** \brief  Cartridge can-save function pointer */
int (*carthelpers_can_save_func)(int type);

/** \brief  Cartridge can-flush function pointer */
int (*carthelpers_can_flush_func)(int type);

/** \brief  Cartridge set-default function pointer */
void (*carthelpers_set_default_func)(void);

/** \brief  Cartridge unset-default function pointer */
void (*carthelpers_unset_default_func)(void);

/** \brief  Cartridge info-list function pointer */
cartridge_info_t * (*carthelpers_info_list_func)(void);


/** \brief  Set cartridge helper functions
 *
 * This function helps to avoid the problems with VSID wrt cartridge code:
 * VSID doesn't link against any cartridge code and since the various widgets
 * in src/arch/gtk3 are linked into a single .a object which VSID also links to
 * we need a way to use cartridge functions without VSID borking during linking.
 * Passing in pointers to the cart functions in ${emu}ui.c (except vsidui.c)
 * 'solves' this problem.
 *
 * Normally \a save_func should be cartridge_save_image(), \a flush_func should
 * be cartridge_flush_image() and \a enabled_func should be
 * \a cartridge_type_enabled.
 * These are the functions used by many/all(?) cartridge widgets
 *
 * \param[in]   save_func           cart image save-as function
 * \param[in]   flush_func          cart image flush/save function
 * \param[in]   is_enabled_func     cart enabled state function
 * \param[in]   enable_func         cart enable function
 * \param[in]   disable_func        cart disable function
 * \param[in]   can_save_func       cart-can-save function
 * \param[in]   can_flush_func      cart-can-flush function
 * \param[in]   set_default_func    set default cart function
 * \param[in]   unset_default_func  unset default cart function
 * \param[in]   info_list_func      function to retrieve list of cart info
 */
void carthelpers_set_functions(
        int (*save_func)(int, const char *),
        int (*flush_func)(int),
        int (*is_enabled_func)(int),
        int (*enable_func)(int),
        int (*disable_func)(int),
        int (*can_save_func)(int),
        int (*can_flush_func)(int),
        void (*set_default_func)(void),
        void (*unset_default_func)(void),
        cartridge_info_t * (*info_list_func)(void))
{
    carthelpers_save_func = save_func;
    carthelpers_flush_func = flush_func;
    carthelpers_is_enabled_func = is_enabled_func;
    carthelpers_enable_func = enable_func;
    carthelpers_disable_func = disable_func;
    carthelpers_can_save_func = can_save_func;
    carthelpers_can_flush_func = can_flush_func;
    carthelpers_set_default_func = set_default_func;
    carthelpers_unset_default_func = unset_default_func;
    carthelpers_info_list_func = info_list_func;
}


/** \brief  Handler for the "destroy" event of a cart enable check button
 *
 * Frees the cartridge name stored as a property in the check button.
 *
 * \param[in,out]   check   check button
 * \param[in]       data    unused
 */
static void on_cart_enable_check_button_destroy(GtkCheckButton *check,
                                                gpointer data)
{
    char *name = g_object_get_data(G_OBJECT(check), "CartridgeName");

    if (name != NULL) {
        lib_free(name);
    }
}


/** \brief  Handler for the "toggled" event of the cart enable check button
 *
 * When this function fails to set a resource, it'll revert to the old state,
 * unfortunately this also triggers a new event (calling this very function).
 * I still have to figure out how to temporarily block signals (it's not like
 * Qt)
 *
 * \param[in,out]   check   check button
 * \param[in]       data    unused
 */
static void on_cart_enable_check_button_toggled(GtkCheckButton *check,
                                                gpointer data)
{
    int id;
    int state;

    id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(check), "CartridgeId"));
    state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check));
    if (state) {
        if (carthelpers_enable_func(id) < 0) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), FALSE);
        }
    } else {
        if (carthelpers_disable_func(id) < 0) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), TRUE);
        }
    }
}


/** \brief  Create a check button to enable/disable a cartridge
 *
 * Creates a check button that enables/disables a cartridge. The \a cart_name
 * argument is copied to allow for debug/error messages to mention the cart
 * by name, rather than by ID. The name is freed when the check button is
 * destroyed.
 *
 * What the widget basically does is call cartridge_enable(\a cart_id) or
 * cartridge_disable(\a cart_id), using cartridge_type_enabled(\a cart_id) to
 * set the initial state of the widget. But since all Gtk3 widgets are
 * currently linked into a big lib and vsid doesn't like that, we use some
 * function pointer magic is used.
 *
 * \param[in]   cart_name   cartridge name (see cartridge.h)
 * \param[in]   cart_id     cartridge ID (see cartridge.h)
 *
 * \return  GtkCheckButton
 */
GtkWidget *carthelpers_create_enable_check_button(const char *cart_name,
                                                  int cart_id)
{
    GtkWidget *check;
    char *title;
    gchar *name;

    title = lib_msprintf("Enable %s cartridge", cart_name);
    check = gtk_check_button_new_with_label(title);
    lib_free(title);    /* Gtk3 makes a copy of the title */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
            carthelpers_is_enabled_func(cart_id));

    name = lib_strdup(cart_name);
    g_object_set_data(G_OBJECT(check), "CartridgeName", (gpointer)name);
    g_object_set_data(G_OBJECT(check), "CartridgeId", GINT_TO_POINTER(cart_id));

    g_signal_connect_unlocked(check, "destroy",
            G_CALLBACK(on_cart_enable_check_button_destroy), NULL);
    g_signal_connect(check, "toggled",
            G_CALLBACK(on_cart_enable_check_button_toggled), NULL);

    return check;
}
