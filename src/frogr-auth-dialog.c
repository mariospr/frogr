/*
 * frogr-auth-dialog.c -- Authorization dialog
 *
 * Copyright (C) 2009-2012 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@igalia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 3 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#include "frogr-auth-dialog.h"

#include "frogr-controller.h"
#include "frogr-global-defs.h"
#include "frogr-gtk-compat.h"
#include "frogr-util.h"

#include <config.h>
#include <glib/gi18n.h>

static gchar *unauth_txt =
  N_("Please press the button to authorize %s "
     "and then come back to complete the process.");

static gchar *auth_txt =
  N_("Enter verification code.");

/* Prototypes */

static void _ask_for_authorization (GtkWindow *parent);

static void _ask_for_authorization_response_cb (GtkDialog *dialog, gint response, gpointer data);

static void _ask_for_auth_confirmation (GtkWindow *parent);

static void _ask_for_auth_confirmation_response_cb (GtkDialog *dialog, gint response, gpointer data);

/* Private API */

static void
_ask_for_authorization (GtkWindow *parent)
{
  GtkWidget *dialog = NULL;
  gchar *title = NULL;

  dialog = gtk_message_dialog_new (parent,
                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_INFO,
                                   GTK_BUTTONS_OK,
                                   _(unauth_txt),
                                   APP_SHORTNAME);

  title = g_strdup_printf (_("Authorize %s"), APP_SHORTNAME);
  gtk_window_set_title (GTK_WINDOW (dialog), title);
  g_free (title);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (_ask_for_authorization_response_cb), NULL);

  gtk_widget_show_all (dialog);
}

static void
_ask_for_authorization_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  if (response == GTK_RESPONSE_OK)
    {
      FrogrController *controller = frogr_controller_get_instance();
      frogr_controller_open_auth_url (controller);
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
_code_entry_text_inserted_cb (GtkEditable *editable, gchar *new_text,
                              gint new_text_length, gint *position,
                              gpointer data)
{
  gint i = 0;
  for (i = 0; i < new_text_length; i++)
    {
      /* Stop this signal's emission if one of the new characters is
         not an integer, and stop searching, obviously */
      if (!g_ascii_isdigit (new_text[i]))
        {
          g_signal_stop_emission_by_name (editable, "insert-text");
          break;
        }
    }
}

static GtkWidget *
_build_verification_code_entry_widget (GtkWidget *dialog)
{
  GtkWidget *hbox = NULL;
  GtkWidget *entry = NULL;
  GtkWidget *separator = NULL;
  gchar *entry_key = NULL;
  gint i = 0;

  hbox = frogr_gtk_compat_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  for (i = 0; i < 3; i++)
    {
      entry = gtk_entry_new ();
      gtk_entry_set_max_length (GTK_ENTRY (entry), 3);
      gtk_entry_set_width_chars (GTK_ENTRY (entry), 3);
      gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, FALSE, 6);

      entry_key = g_strdup_printf ("vercode-%d", i + 1);
      g_object_set_data (G_OBJECT (dialog), entry_key, entry);
      g_free (entry_key);

      g_signal_connect (G_OBJECT (entry), "insert-text",
                    G_CALLBACK (_code_entry_text_inserted_cb),
                    NULL);
      if (i < 2)
        {
          separator = frogr_gtk_compat_separator_new (GTK_ORIENTATION_HORIZONTAL);
          gtk_box_pack_start (GTK_BOX (hbox), separator, TRUE, TRUE, 0);
        }
    }

  return hbox;
}

static void
_ask_for_auth_confirmation (GtkWindow *parent)
{
  GtkWidget *dialog = NULL;
  GtkWidget *content_area = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *ver_code_entry = NULL;
  GtkWidget *label = NULL;
  gchar *title = NULL;

  title = g_strdup_printf (_("Authorize %s"), APP_SHORTNAME);
  dialog = gtk_dialog_new_with_buttons (title,
                                        parent,
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_OK,
                                        NULL);
  g_free (title);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  /* Fill action area */
  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  vbox = frogr_gtk_compat_box_new (GTK_ORIENTATION_VERTICAL, 0);

  /* Entry widgets for the verification code */
  ver_code_entry = _build_verification_code_entry_widget (dialog);
  gtk_box_pack_start (GTK_BOX (vbox), ver_code_entry, TRUE, TRUE, 0);

  /* Description label */
  label = gtk_label_new (auth_txt);
  gtk_misc_set_padding (GTK_MISC (label), 12, 0);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 6);

  gtk_container_add (GTK_CONTAINER (content_area), vbox);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (_ask_for_auth_confirmation_response_cb), NULL);

  gtk_widget_show_all (dialog);
}

static const gchar*
_get_entry_code_for_dialog (GtkDialog *dialog, const gchar *entry_key)
{
  GtkWidget *entry = g_object_get_data (G_OBJECT (dialog), entry_key);
  if (entry == NULL)
    return NULL;

  return gtk_entry_get_text (GTK_ENTRY (entry));
}

static void
_ask_for_auth_confirmation_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  gboolean valid = FALSE;

  if (response == GTK_RESPONSE_OK)
    {
      const gchar *vercode_part1 = NULL;
      const gchar *vercode_part2 = NULL;
      const gchar *vercode_part3 = NULL;
      gchar *vercode_full = NULL;

      vercode_part1 = _get_entry_code_for_dialog (dialog, "vercode-1");
      vercode_part2 = _get_entry_code_for_dialog (dialog, "vercode-2");
      vercode_part3 = _get_entry_code_for_dialog (dialog, "vercode-3");

      vercode_full = g_strdup_printf ("%s-%s-%s", vercode_part1, vercode_part2, vercode_part3);

      /* Ensure the user enters a valid verification code */
      if (g_regex_match_simple ("[0-9]{3}(-[0-9]{3}){2}", vercode_full, 0, 0))
        {
          frogr_controller_complete_auth (frogr_controller_get_instance(), vercode_full);
          valid = TRUE;
        }
      else
        frogr_util_show_error_dialog (GTK_WINDOW (dialog), _("Invalid verification code"));

      g_free (vercode_full);
    }

  if (valid)
    gtk_widget_destroy (GTK_WIDGET (dialog));
}

/* Public API */

void
frogr_auth_dialog_show (GtkWindow *parent, FrogrAuthDialogStep step)
{
  switch (step) {
  case REQUEST_AUTHORIZATION:
    _ask_for_authorization (parent);
    break;
  case CONFIRM_AUTHORIZATION:
    _ask_for_auth_confirmation (parent);
    break;
  default:
    g_assert_not_reached();
  }
}
