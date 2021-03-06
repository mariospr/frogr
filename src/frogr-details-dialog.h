/*
 * frogr-details-dialog.h -- Picture details dialog
 *
 * Copyright (C) 2009-2012 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@gnome.org>
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

#ifndef FROGR_DETAILS_DIALOG_H
#define FROGR_DETAILS_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FROGR_TYPE_DETAILS_DIALOG (frogr_details_dialog_get_type())

G_DECLARE_FINAL_TYPE (FrogrDetailsDialog, frogr_details_dialog, FROGR, DETAILS_DIALOG, GtkDialog)

void frogr_details_dialog_show (GtkWindow *parent,
                                const GSList *pictures,
                                const GSList *tags);

G_END_DECLS  /* FROGR_DETAILS_DIALOG_H */

#endif
