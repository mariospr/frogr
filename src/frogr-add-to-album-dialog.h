/*
 * frogr-add-to-album-dialog.h -- 'Add to album' dialog
 *
 * Copyright (C) 2010 Mario Sanchez Prada
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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef FROGR_ADD_TO_ALBUM_DIALOG_H
#define FROGR_ADD_TO_ALBUM_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FROGR_TYPE_ADD_TO_ALBUM_DIALOG           (frogr_add_to_album_dialog_get_type())
#define FROGR_ADD_TO_ALBUM_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_ADD_TO_ALBUM_DIALOG, FrogrAddToAlbumDialog))
#define FROGR_ADD_TO_ALBUM_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_ADD_TO_ALBUM_DIALOG, FrogrAddToAlbumDialogClass))
#define FROGR_IS_ADD_TO_ALBUM_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_ADD_TO_ALBUM_DIALOG))
#define FROGR_IS_ADD_TO_ALBUM_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_ADD_TO_ALBUM_DIALOG))
#define FROGR_ADD_TO_ALBUM_DIALOG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_ADD_TO_ALBUM_DIALOG, FrogrAddToAlbumDialogClass))

typedef struct _FrogrAddToAlbumDialog        FrogrAddToAlbumDialog;
typedef struct _FrogrAddToAlbumDialogClass   FrogrAddToAlbumDialogClass;

struct _FrogrAddToAlbumDialogClass
{
  GtkDialogClass parent_class;
};

struct _FrogrAddToAlbumDialog
{
  GtkDialog parent;
};

GType frogr_add_to_album_dialog_get_type (void) G_GNUC_CONST;

void frogr_add_to_album_dialog_show (GtkWindow *parent,
                                     GSList *pictures,
                                     GSList *albums);

G_END_DECLS  /* FROGR_ADD_TO_ALBUM_DIALOG_H */

#endif