/*
 * fsp-error.h
 *
 * Copyright (C) 2010 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@igalia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 3 of the GNU Lesser General
 * Public License as published by the Free Software Foundation.
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

#ifndef _FSP_ERROR_H
#define _FSP_ERROR_H

#include <glib.h>

G_BEGIN_DECLS

/* Error domain */
#define FSP_ERROR g_quark_from_static_string ("flicksoup-error")

/* Error codes for FSP_ERROR domain */
typedef enum {
  /* Unknown errors */
  FSP_ERROR_UNKNOWN,

  /* Errors from communication layer */
  FSP_ERROR_NETWORK_ERROR,
  FSP_ERROR_CLIENT_ERROR,
  FSP_ERROR_SERVER_ERROR,

  /* Errors from REST response */
  FSP_ERROR_WRONG_RESPONSE,
  FSP_ERROR_MISSING_DATA,

  /* Errors from flickr API */
  FSP_ERROR_PHOTO_NOT_FOUND,
  FSP_ERROR_UPLOAD_MISSING_PHOTO,
  FSP_ERROR_UPLOAD_GENERAL_FAILURE,
  FSP_ERROR_UPLOAD_INVALID_FILE,
  FSP_ERROR_UPLOAD_QUOTA_EXCEEDED,
  FSP_ERROR_NOT_AUTHENTICATED,
  FSP_ERROR_NOT_ENOUGH_PERMISSIONS,
  FSP_ERROR_INVALID_API_KEY,
  FSP_ERROR_SERVICE_UNAVAILABLE,

  /* Errors from flicksoup only */
  FSP_ERROR_NOFROB,

  /* Default fallback for other kind of errors */
  FSP_ERROR_OTHER
} FspError;

FspError
fsp_error_get_from_response_code        (gint code);

G_END_DECLS

#endif