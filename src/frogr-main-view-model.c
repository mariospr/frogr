/*
 * frogr-main-view-model.c -- Main view model in frogr
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

#include "frogr-main-view-model.h"

#include "frogr-account.h"
#include "frogr-file-loader.h"

#define TAGS_DELIMITER " "

#define FROGR_MAIN_VIEW_MODEL_GET_PRIVATE(object)               \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_MAIN_VIEW_MODEL,     \
                                FrogrMainViewModelPrivate))

G_DEFINE_TYPE (FrogrMainViewModel, frogr_main_view_model, G_TYPE_OBJECT)

/* Private struct */
typedef struct _FrogrMainViewModelPrivate FrogrMainViewModelPrivate;
struct _FrogrMainViewModelPrivate
{
  GSList *pictures;

  /* For sequential access of groups and sets */
  GSList *remote_sets;
  GSList *local_sets;
  GSList *all_sets;
  GSList *groups;

  /* For random acces (e.g. get a group by ID) */
  GHashTable *sets_table;
  GHashTable *groups_table;

  GSList *remote_tags;
  GSList *local_tags;
  GSList *all_tags;
};

/* Signals */
enum {
  PICTURE_ADDED,
  PICTURE_REMOVED,
  MODEL_CHANGED,
  MODEL_DESERIALIZED,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

/* Private API */

static gint
_compare_photosets (FrogrPhotoSet *photoset1, FrogrPhotoSet *photoset2)
{
  g_return_val_if_fail (FROGR_IS_PHOTOSET (photoset1), 1);
  g_return_val_if_fail (FROGR_IS_PHOTOSET (photoset2), -1);

  if (photoset1 == photoset2)
    return 0;

  return g_strcmp0 (frogr_photoset_get_id (photoset1), frogr_photoset_get_id (photoset2));
}

static void
_remove_remote_photosets (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;
  FrogrPhotoSet *set = NULL;
  GSList *item = NULL;
  const gchar *id = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  if (!priv->remote_sets)
    return;

  /* Remove from the hash table first */
  for (item = priv->remote_sets; item; item = g_slist_next (item))
    {
      set = FROGR_PHOTOSET (item->data);
      id = frogr_photoset_get_id (set);
      if (id)
        g_hash_table_remove (priv->sets_table, id);

      /* A remote photo soet might be still indexed by its local ID */
      id = frogr_photoset_get_local_id (set);
      if (id)
        g_hash_table_remove (priv->sets_table, id);
    }

  g_slist_foreach (priv->remote_sets, (GFunc)g_object_unref, NULL);
  g_slist_free (priv->remote_sets);
  priv->remote_sets = NULL;
}

static void
_remove_local_photosets (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;
  FrogrPhotoSet *set = NULL;
  GSList *item = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  if (!priv->local_sets)
    return;

  /* Remove from the hash table first */
  for (item = priv->remote_sets; item; item = g_slist_next (item))
    {
      set = FROGR_PHOTOSET (item->data);
      g_hash_table_remove (priv->sets_table, frogr_photoset_get_local_id (set));
    }

  g_slist_foreach (priv->local_sets, (GFunc)g_object_unref, NULL);
  g_slist_free (priv->local_sets);
  priv->local_sets = NULL;
}

static void
_remove_all_photosets (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  if (priv->all_sets)
    {
      g_slist_free (priv->all_sets);
      priv->all_sets = NULL;
    }

  _remove_remote_photosets (self);
  _remove_local_photosets (self);

  if (priv->sets_table)
    g_hash_table_remove_all (priv->sets_table);
}

static void
_remove_groups (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  if (priv->groups)
    {
      g_slist_foreach (priv->groups, (GFunc)g_object_unref, NULL);
      g_slist_free (priv->groups);
      priv->groups = NULL;
    }

  if (priv->groups_table)
    g_hash_table_remove_all (priv->groups_table);
}

static void
_remove_remote_tags (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  if (!priv->remote_tags)
    return;

  g_slist_foreach (priv->remote_tags, (GFunc)g_free, NULL);
  g_slist_free (priv->remote_tags);
  priv->remote_tags = NULL;
}

static void
_remove_local_tags (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  if (!priv->local_tags)
    return;

  g_slist_foreach (priv->local_tags, (GFunc)g_free, NULL);
  g_slist_free (priv->local_tags);
  priv->local_tags = NULL;
}

static void
_remove_all_tags (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  if (priv->all_tags)
    {
      g_slist_free (priv->all_tags);
      priv->all_tags = NULL;
    }

  _remove_remote_tags (self);
  _remove_local_tags (self);
}

static JsonArray *
_serialize_list_to_json_array (GSList *list, GType g_type)
{
  JsonArray *json_array = NULL;
  JsonNode *json_node = NULL;
  GSList *item = NULL;

  /* Generate a JsonArray with contents */
  json_array = json_array_new ();
  for (item = list; item; item = g_slist_next (item))
    {
      if (g_type == G_TYPE_OBJECT)
        json_node = json_gobject_serialize (G_OBJECT (item->data));
      else if (g_type == G_TYPE_STRING)
        {
          json_node = json_node_new (JSON_NODE_VALUE);
          json_node_set_string (json_node, (const gchar*)item->data);
        }

      if (json_node)
        json_array_add_element (json_array, json_node);
    }

  return json_array;
}

static GSList *
_deserialize_list_from_json_array (JsonArray *array, GType g_type)
{
  JsonNode *json_node = NULL;
  GSList *result_list = NULL;
  gpointer element;
  guint n_elements = 0;
  guint i = 0;

  n_elements = json_array_get_length (array);
  if (!n_elements)
    return NULL;

  for (i = 0; i < n_elements; i++)
    {
      json_node = json_array_get_element (array, i);
      if (JSON_NODE_HOLDS_OBJECT (json_node))
        element = json_gobject_deserialize (g_type, json_node);
      else if (g_type == G_TYPE_STRING)
        element = json_node_dup_string (json_node);

      if (element)
        result_list = g_slist_append (result_list, element);
    }

  return result_list;
}

static void
_on_file_loaded (FrogrFileLoader *loader, FrogrPicture *picture, FrogrMainViewModel *self)
{
  g_return_if_fail (FROGR_IS_MAIN_VIEW_MODEL (self));
  frogr_main_view_model_add_picture (self, picture);
}

static void
_on_files_loaded (FrogrFileLoader *loader, FrogrMainViewModel *self)
{
  g_signal_emit (self, signals[MODEL_DESERIALIZED], 0);
}

static void
_frogr_main_view_model_dispose (GObject* object)
{
  FrogrMainViewModel *self = FROGR_MAIN_VIEW_MODEL (object);
  FrogrMainViewModelPrivate *priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (object);

  if (priv->pictures)
    {
      g_slist_foreach (priv->pictures, (GFunc)g_object_unref, NULL);
      g_slist_free (priv->pictures);
      priv->pictures = NULL;
    }

  _remove_all_photosets (self);
  _remove_groups (self);
  _remove_all_tags (self);

  if (priv->sets_table)
    {
      g_hash_table_destroy (priv->sets_table);
      priv->sets_table = NULL;
    }

  if (priv->groups_table)
    {
      g_hash_table_destroy (priv->groups_table);
      priv->groups_table = NULL;
    }

  G_OBJECT_CLASS (frogr_main_view_model_parent_class)->dispose (object);
}

static void
frogr_main_view_model_class_init(FrogrMainViewModelClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);
  obj_class->dispose = _frogr_main_view_model_dispose;

  signals[PICTURE_ADDED] =
    g_signal_new ("picture-added",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, FROGR_TYPE_PICTURE);

  signals[PICTURE_REMOVED] =
    g_signal_new ("picture-removed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1, FROGR_TYPE_PICTURE);

  signals[MODEL_CHANGED] =
    g_signal_new ("model-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[MODEL_DESERIALIZED] =
    g_signal_new ("model-deserialized",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_type_class_add_private (obj_class, sizeof (FrogrMainViewModelPrivate));
}

static void
frogr_main_view_model_init (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv =
    FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  /* Init private data */
  priv->pictures = NULL;
  priv->remote_sets = NULL;
  priv->local_sets = NULL;
  priv->all_sets = NULL;
  priv->groups = NULL;

  /* For random access */
  priv->sets_table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                            (GDestroyNotify)g_free,
                                            (GDestroyNotify)g_object_unref);

  priv->groups_table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                              (GDestroyNotify)g_free,
                                              (GDestroyNotify)g_object_unref);

  priv->remote_tags = NULL;
  priv->local_tags = NULL;
  priv->all_tags = NULL;
}

/* Public API */

FrogrMainViewModel *
frogr_main_view_model_new (void)
{
  GObject *new = g_object_new(FROGR_TYPE_MAIN_VIEW_MODEL, NULL);
  return FROGR_MAIN_VIEW_MODEL (new);
}

void
frogr_main_view_model_add_picture (FrogrMainViewModel *self,
                                   FrogrPicture *picture)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));
  g_return_if_fail(FROGR_IS_PICTURE (picture));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  priv->pictures = g_slist_append (priv->pictures, g_object_ref (picture));

  g_signal_emit (self, signals[PICTURE_ADDED], 0, picture);
  g_signal_emit (self, signals[MODEL_CHANGED], 0);
}

void
frogr_main_view_model_remove_picture (FrogrMainViewModel *self,
                                      FrogrPicture *picture)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  priv->pictures = g_slist_remove (priv->pictures, picture);

  g_signal_emit (self, signals[PICTURE_REMOVED], 0, picture);
  g_signal_emit (self, signals[MODEL_CHANGED], 0);
  g_object_unref (picture);
}

guint
frogr_main_view_model_n_pictures (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), 0);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return g_slist_length (priv->pictures);
}

GSList *
frogr_main_view_model_get_pictures (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), NULL);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return priv->pictures;
}

void
frogr_main_view_model_notify_changes_in_pictures (FrogrMainViewModel *self)
{
  /* Just emit the signal so the main view gets notified too */
  g_signal_emit (self, signals[MODEL_CHANGED], 0);
}

void
frogr_main_view_model_set_remote_photosets (FrogrMainViewModel *self,
                                            GSList *remote_sets)
{
  FrogrMainViewModelPrivate *priv = NULL;
  FrogrPhotoSet *set = NULL;
  GSList *item = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  /* Remove all the remote photosets */
  _remove_remote_photosets (self);

  /* Set photosets now (and update the hash table) */
  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  for (item = remote_sets; item; item = g_slist_next (item))
    {
      set = FROGR_PHOTOSET (item->data);
      g_hash_table_insert (priv->sets_table,
                           g_strdup (frogr_photoset_get_id (set)),
                           g_object_ref (set));
    }
  priv->remote_sets = remote_sets;
}

void
frogr_main_view_model_add_local_photoset (FrogrMainViewModel *self,
                                          FrogrPhotoSet *set)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));
  g_return_if_fail(FROGR_IS_PHOTOSET (set));

  /* When adding one by one we prepend always to keep the order */
  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  priv->local_sets = g_slist_prepend (priv->local_sets, g_object_ref (set));

  /* Update the hash table too! */
  g_hash_table_insert (priv->sets_table,
                       g_strdup (frogr_photoset_get_id (set)),
                       g_object_ref (set));

  g_signal_emit (self, signals[MODEL_CHANGED], 0);
}

GSList *
frogr_main_view_model_get_photosets (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;
  GSList *list = NULL;
  GSList *current = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), 0);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  /* Copy the list of remote sets and add those locally added */
  list = g_slist_copy (priv->remote_sets);
  for (current = priv->local_sets; current; current = g_slist_next (current))
    {
      if (!g_slist_find_custom (list, current->data, (GCompareFunc)_compare_photosets))
        list = g_slist_prepend (list, current->data);
    }

  /* Update internal pointers to the result list */
  if (priv->all_sets)
    g_slist_free (priv->all_sets);
  priv->all_sets = list;

  return priv->all_sets;
}

void
frogr_main_view_model_set_photosets (FrogrMainViewModel *self,
                                     GSList *photosets)
{
  FrogrPhotoSet *set = NULL;
  GSList *item = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  /* Remove all sets */
  _remove_all_photosets (self);

  /* Set groups now, separating them into two lists: local and remote */
  for (item = photosets; item; item = g_slist_next (item))
    {
      set = FROGR_PHOTOSET(item->data);
      if (frogr_photoset_is_local (set))
        {
          photosets = g_slist_remove_link (photosets, item);
          frogr_main_view_model_add_local_photoset (self, set);
          g_object_unref (set);
          g_slist_free (item);
        }
    }

  /* Once separated the local photosets, we assign the remote ones */
  frogr_main_view_model_set_remote_photosets (self, photosets);
}

guint
frogr_main_view_model_n_photosets (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), 0);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return g_slist_length (priv->remote_sets) + g_slist_length (priv->local_sets);
}

FrogrPhotoSet *
frogr_main_view_model_get_photoset_by_id (FrogrMainViewModel *self, const gchar *id)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), NULL);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return g_hash_table_lookup (priv->sets_table, id);
}

guint
frogr_main_view_model_n_groups (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), 0);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return g_slist_length (priv->groups);
}

GSList *
frogr_main_view_model_get_groups (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), NULL);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return priv->groups;
}

void
frogr_main_view_model_set_groups (FrogrMainViewModel *self,
                                  GSList *groups)
{
  FrogrMainViewModelPrivate *priv = NULL;
  FrogrGroup *group = NULL;
  GSList *item = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  /* Remove all groups */
  _remove_groups (self);

  /* Set groups now (and update the hash table) */
  for (item = groups; item; item = g_slist_next (item))
    {
      group = FROGR_GROUP (item->data);
      g_hash_table_insert (priv->groups_table,
                           g_strdup (frogr_group_get_id (group)),
                           g_object_ref (group));
    }

  /* Set groups now */
  priv->groups = groups;
}

FrogrGroup *
frogr_main_view_model_get_group_by_id (FrogrMainViewModel *self, const gchar *id)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), NULL);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  return g_hash_table_lookup (priv->groups_table, id);
}

void
frogr_main_view_model_set_remote_tags (FrogrMainViewModel *self, GSList *tags_list)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  _remove_remote_tags (self);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
  priv->remote_tags = tags_list;
}

void
frogr_main_view_model_remove_remote_tags (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  g_slist_foreach (priv->remote_tags, (GFunc)g_free, NULL);
  g_slist_free (priv->remote_tags);

  priv->remote_tags = NULL;
}

void
frogr_main_view_model_add_local_tags_from_string (FrogrMainViewModel *self,
                                                  const gchar *tags_string)
{
  gchar *stripped_tags = NULL;
  gboolean added_new_tags = FALSE;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  if (!tags_string || !tags_string[0])
    return;

  stripped_tags = g_strstrip (g_strdup (tags_string));
  if (!g_str_equal (stripped_tags, ""))
    {
      FrogrMainViewModelPrivate *priv = NULL;
      gchar **tags_array = NULL;
      gchar *tag;
      gint i;

      /* Now iterate over every token, adding it to the list */
      priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);
      tags_array = g_strsplit (stripped_tags, TAGS_DELIMITER, -1);
      for (i = 0; tags_array[i]; i++)
        {
          /* add stripped tag if not already set*/
          tag = g_strstrip(g_strdup (tags_array[i]));
          if (!g_str_equal (tag, "") && !g_slist_find_custom (priv->local_tags, tag, (GCompareFunc)g_strcmp0))
            {
              priv->local_tags = g_slist_prepend (priv->local_tags, g_strdup (tag));
              added_new_tags = TRUE;
            }

          g_free (tag);
        }
      g_strfreev (tags_array);

      priv->local_tags = g_slist_sort (priv->local_tags, (GCompareFunc)g_strcmp0);
    }
  g_free (stripped_tags);

  if (added_new_tags)
    g_signal_emit (self, signals[MODEL_CHANGED], 0);
}

GSList *
frogr_main_view_model_get_tags (FrogrMainViewModel *self)
{
  FrogrMainViewModelPrivate *priv = NULL;
  GSList *list = NULL;
  GSList *current = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), 0);

  priv = FROGR_MAIN_VIEW_MODEL_GET_PRIVATE (self);

  /* Copy the list of remote tags and add those locally added */
  list = g_slist_copy (priv->remote_tags);
  for (current = priv->local_tags; current; current = g_slist_next (current))
    {
      if (!g_slist_find_custom (list, current->data, (GCompareFunc)g_strcmp0))
        list = g_slist_prepend (list, current->data);
    }
  list = g_slist_sort (list, (GCompareFunc)g_strcmp0);

  /* Update internal pointers to the result list */
  if (priv->all_tags)
    g_slist_free (priv->all_tags);
  priv->all_tags = list;

  return priv->all_tags;
}

JsonNode *
frogr_main_view_model_serialize (FrogrMainViewModel *self)
{
  JsonArray *json_array = NULL;
  JsonNode *root_node = NULL;
  JsonObject *root_object = NULL;
  GSList *data_list = NULL;

  g_return_val_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self), NULL);

  root_object = json_object_new ();

  data_list = frogr_main_view_model_get_pictures (self);
  json_array = _serialize_list_to_json_array (data_list, G_TYPE_OBJECT);
  json_object_set_array_member (root_object, "pictures", json_array);

  data_list = frogr_main_view_model_get_photosets (self);
  json_array = _serialize_list_to_json_array (data_list, G_TYPE_OBJECT);
  json_object_set_array_member (root_object, "photosets", json_array);

  data_list = frogr_main_view_model_get_groups (self);
  json_array = _serialize_list_to_json_array (data_list, G_TYPE_OBJECT);
  json_object_set_array_member (root_object, "groups", json_array);

  data_list = frogr_main_view_model_get_tags (self);
  json_array = _serialize_list_to_json_array (data_list, G_TYPE_STRING);
  json_object_set_array_member (root_object, "tags", json_array);

  root_node = json_node_new (JSON_NODE_OBJECT);
  json_node_set_object (root_node, root_object);

  return root_node;
}

void
frogr_main_view_model_deserialize (FrogrMainViewModel *self, JsonNode *json_node)
{
  FrogrFileLoader *loader = NULL;
  JsonObject *root_object = NULL;
  JsonArray *array_member = NULL;
  GSList *pictures = NULL;
  GSList *sets = NULL;
  GSList *groups = NULL;
  GSList *tags = NULL;

  g_return_if_fail(FROGR_IS_MAIN_VIEW_MODEL (self));

  /* First we get the different lists with data */
  root_object = json_node_get_object (json_node);

  array_member = json_object_get_array_member (root_object, "pictures");
  if (array_member)
    pictures = _deserialize_list_from_json_array (array_member, FROGR_TYPE_PICTURE);

  array_member = json_object_get_array_member (root_object, "photosets");
  if (array_member)
    sets = _deserialize_list_from_json_array (array_member, FROGR_TYPE_PHOTOSET);

  array_member = json_object_get_array_member (root_object, "groups");
  if (array_member)
    groups = _deserialize_list_from_json_array (array_member, FROGR_TYPE_GROUP);

  array_member = json_object_get_array_member (root_object, "tags");
  if (array_member)
    tags = _deserialize_list_from_json_array (array_member, G_TYPE_STRING);

  /* We set the sets, groups and tags */
  frogr_main_view_model_set_photosets (self, sets);
  frogr_main_view_model_set_groups (self, groups);
  frogr_main_view_model_set_remote_tags (self, tags);

  /* Now we take the list of pictures and carefully ad them into the
     model as long as the associated thumbnails are being loaded */
  loader = frogr_file_loader_new_from_pictures (pictures);

  g_signal_connect (G_OBJECT (loader), "file-loaded",
                    G_CALLBACK (_on_file_loaded),
                    self);

  g_signal_connect (G_OBJECT (loader), "files-loaded",
                    G_CALLBACK (_on_files_loaded),
                    self);

  /* Load the pictures! */
  frogr_file_loader_load (loader);
}
