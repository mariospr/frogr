/*
 * fsp-util.c
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

#include "fsp-util.h"

#include "fsp-error.h"

#include <stdarg.h>
#include <libsoup/soup.h>

static GHashTable *
_get_params_table_from_valist           (const gchar *first_param,
                                         va_list      args)
{
  g_return_val_if_fail (first_param != NULL, NULL);
  g_return_val_if_fail (args != NULL, NULL);

  GHashTable *table = NULL;
  gchar *p, *v;

  table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                 (GDestroyNotify)g_free,
                                 (GDestroyNotify)g_free);

  /* Fill the hash table */
  for (p = (gchar *) first_param; p; p = va_arg (args, gchar*))
    {
      v = va_arg (args, gchar*);

      /* Ignore parameter with no value */
      if (v != NULL)
        g_hash_table_insert (table, g_strdup (p), soup_uri_encode (v, NULL));
      else
        g_warning ("Missing value for %s. Ignoring parameter.", p);
    }

  return table;
}

static gchar *
_get_signed_query_with_params           (const gchar      *api_sig,
                                         GHashTable       *params_table)
{
  g_return_val_if_fail (params_table != NULL, NULL);
  g_return_val_if_fail (api_sig != NULL, NULL);

  GList *keys = NULL;
  gchar *retval = NULL;

  /* Get ownership of the table */
  g_hash_table_ref (params_table);

  /* Get a list of keys */
  keys = g_hash_table_get_keys (params_table);
  if (keys != NULL)
    {
      gchar **url_params_array = NULL;
      GList *k = NULL;
      gint i = 0;

      /* Build gchar** arrays for building the final
         string to be used as the list of GET params */
      url_params_array = g_new0 (gchar*, g_list_length (keys) + 2);

      /* Fill arrays */
      for (k = keys; k; k = g_list_next (k))
        {
          gchar *key = (gchar*) k->data;
          gchar *value = g_hash_table_lookup (params_table, key);
          url_params_array[i++] = g_strdup_printf ("%s=%s", key, value);
        }

      /* Add those to the params array (space previously reserved) */
      url_params_array[i] = g_strdup_printf ("api_sig=%s", api_sig);

      /* Build the signed query */
      retval = g_strjoinv ("&", url_params_array);

      /* Free */
      g_strfreev (url_params_array);
    }
  g_list_free (keys);
  g_hash_table_unref (params_table);

  return retval;
}

gchar *
get_api_signature                      (const gchar *shared_secret,
                                         const gchar *first_param,
                                         ... )
{
  g_return_val_if_fail (shared_secret != NULL, NULL);

  va_list args;
  GHashTable *table = NULL;
  gchar *api_sig = NULL;

  va_start (args, first_param);

  /* Get the hash table for the params and the API signature from it */
  table = _get_params_table_from_valist (first_param, args);
  api_sig = get_api_signature_from_hash_table (shared_secret, table);

  g_hash_table_unref (table);
  va_end (args);

  return api_sig;
}

gchar *
get_api_signature_from_hash_table       (const gchar *shared_secret,
                                         GHashTable  *params_table)
{
  g_return_val_if_fail (shared_secret != NULL, NULL);
  g_return_val_if_fail (params_table != NULL, NULL);

  GList *keys = NULL;
  gchar *api_sig = NULL;

  /* Get ownership of the table */
  g_hash_table_ref (params_table);

  /* Get a list of keys */
  keys = g_hash_table_get_keys (params_table);
  if (keys != NULL)
    {
      gchar **sign_str_array = NULL;
      gchar *sign_str = NULL;
      GList *k = NULL;
      gint i = 0;

      /* Sort the list */
      keys = g_list_sort (keys, (GCompareFunc) g_strcmp0);

      /* Build gchar** arrays for building the signature string */
      sign_str_array = g_new0 (gchar*, (2 * g_list_length (keys)) + 2);

      /* Fill arrays */
      sign_str_array[i++] = (gchar *) shared_secret;
      for (k = keys; k; k = g_list_next (k))
        {
          gchar *key = (gchar*) k->data;
          gchar *value = g_hash_table_lookup (params_table, key);

          sign_str_array[i++] = key;
          sign_str_array[i++] = value;
        }

      /* Get the signature string and calculate the api_sig value */
      sign_str = g_strjoinv (NULL, sign_str_array);
      api_sig = g_compute_checksum_for_string (G_CHECKSUM_MD5, sign_str, -1);

      /* Free */
      g_free (sign_str);
      g_free (sign_str_array); /* Don't use g_strfreev here */
    }

  g_list_free (keys);
  g_hash_table_unref (params_table);

  return api_sig;
}

/**
 * get_signed_query:
 * @shared_secret: secret associated to the Flickr API key being used
 * @first_param: key of the first parameter
 * @...: value for the first parameter, followed optionally by more
 *  key/value parameters pairs, followed by %NULL
 *
 * Gets a signed query part for a given set of pairs
 * key/value. The returned string should be freed with g_free() 
 * when no longer needed.
 *
 * Returns: a newly-allocated @str with the signed query
 */
gchar *
get_signed_query                        (const gchar *shared_secret,
                                         const gchar *first_param,
                                         ... )
{
  g_return_val_if_fail (shared_secret != NULL, NULL);
  g_return_val_if_fail (first_param != NULL, NULL);

  va_list args;
  GHashTable *table = NULL;
  gchar *api_sig = NULL;
  gchar *retval = NULL;

  va_start (args, first_param);

  /* Get the hash table for the params and the API signature from it */
  table = _get_params_table_from_valist (first_param, args);
  api_sig = get_api_signature_from_hash_table (shared_secret, table);

  /* Get the signed URL with the needed params */
  if ((table != NULL) && (api_sig != NULL))
    retval = _get_signed_query_with_params (api_sig, table);

  g_hash_table_unref (table);
  g_free (api_sig);

  va_end (args);

  return retval;
}

gchar *
get_signed_query_from_hash_table        (const gchar *shared_secret,
                                         GHashTable  *params_table)
{
  g_return_val_if_fail (shared_secret != NULL, NULL);
  g_return_val_if_fail (params_table != NULL, NULL);

  gchar *api_sig = NULL;
  gchar *retval = NULL;

  /* Get api signature */
  api_sig = get_api_signature_from_hash_table (shared_secret, params_table);

  /* Get the signed URL with the needed params */
  if ((params_table != NULL) && (api_sig != NULL))
    retval = _get_signed_query_with_params (api_sig, params_table);

  g_free (api_sig);

  return retval;
}

void
build_async_result_and_complete         (GAsyncData *clos,
                                         gpointer    result,
                                         GError     *error)
{
  g_assert (clos != NULL);

  GSimpleAsyncResult *res = NULL;
  GObject *object = NULL;
  GAsyncReadyCallback  callback = NULL;
  gpointer source_tag;
  gpointer data;

  /* Get data from closure, and free it */
  object = clos->object;
  callback = clos->callback;
  source_tag = clos->source_tag;
  data = clos->data;
  g_slice_free (GAsyncData, clos);

  /* Build response and call async callback */
  res = g_simple_async_result_new (object, callback,
                                   data, source_tag);

  /* Return the given value or an error otherwise */
  if (error != NULL)
    g_simple_async_result_set_from_error (res, error);
  else
    g_simple_async_result_set_op_res_gpointer (res, result, NULL);

  /* Execute the callback */
  g_simple_async_result_complete_in_idle (res);
}

gboolean
check_async_errors_on_finish            (GObject       *object,
                                         GAsyncResult  *res,
                                         gpointer       source_tag,
                                         GError       **error)
{
  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  gboolean errors_found = TRUE;

  if (g_simple_async_result_is_valid (res, object, source_tag))
    {
      GSimpleAsyncResult *simple = NULL;

      /* Check error */
      simple = G_SIMPLE_ASYNC_RESULT (res);
      if (!g_simple_async_result_propagate_error (simple, error))
	errors_found = FALSE;
    }
  else
    g_set_error_literal (error, FSP_ERROR, FSP_ERROR_OTHER, "Internal error");

  return errors_found;
}