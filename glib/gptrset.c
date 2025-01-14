/*
 * Copyright © 2024 Ole André Vadla Ravnås <oleavr@frida.re>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gptrset.h"
#include "gmem.h"

#define G_PTR_SET_INITIAL_CAPACITY 16
#define G_PTR_INDEX_MAP_TOMBSTONE  ((gpointer) -1)

typedef struct _GPtrIndexMapEntry GPtrIndexMapEntry;

struct _GPtrIndexMap
{
  GPtrIndexMapEntry *entries;
  gsize              capacity;
  gsize              size;
  gsize              tombstones;
};

struct _GPtrIndexMapEntry
{
  gpointer key;
  gsize    val;
};

static void g_ptr_set_grow (GPtrSet *pset);

static GPtrIndexMap *g_ptr_index_map_new (void);
static void g_ptr_index_map_free (GPtrIndexMap   *map);
static void g_ptr_index_map_insert (GPtrIndexMap *map,
                                    gpointer      key,
                                    gsize         val);
static gboolean g_ptr_index_map_lookup (GPtrIndexMap  *map,
                                        gconstpointer  key,
                                        gsize         *out_val);
static void g_ptr_index_map_update (GPtrIndexMap *map,
                                    gpointer      key,
                                    gsize         new_val);
static void g_ptr_index_map_remove (GPtrIndexMap  *map,
                                    gconstpointer  key);
static gsize g_ptr_index_map_probe (GPtrIndexMap  *map,
                                    gconstpointer  key,
                                    gboolean      *found);
static void g_ptr_index_map_rehash (GPtrIndexMap *map,
                                    gsize         new_capacity);
static gsize g_ptr_index_map_adjust_capacity (gsize desired);
static gsize g_ptr_index_map_ptr_hash (gconstpointer ptr);

GPtrSet *
g_ptr_set_new (void)
{
  GPtrSet *pset;

  pset = glib_mem_table->malloc (sizeof (GPtrSet));
  pset->size = 0;
  pset->capacity = G_PTR_SET_INITIAL_CAPACITY;
  pset->items = glib_mem_table->calloc (pset->capacity, sizeof (gpointer));
  pset->index_map = g_ptr_index_map_new ();

  return pset;
}

void
g_ptr_set_free (GPtrSet *pset)
{
  if (pset == NULL)
    return;

  g_ptr_index_map_free (pset->index_map);
  glib_mem_table->free (pset->items);
  glib_mem_table->free (pset);
}

void
g_ptr_set_add (GPtrSet  *pset,
               gpointer  ptr)
{
  if (pset->size == pset->capacity)
    g_ptr_set_grow (pset);

  pset->items[pset->size] = ptr;
  g_ptr_index_map_insert (pset->index_map, ptr, pset->size);
  pset->size++;
}

void
g_ptr_set_remove (GPtrSet  *pset,
                  gpointer  ptr)
{
  gsize idx, last_idx;

  if (pset->size == 0)
    return;

  if (!g_ptr_index_map_lookup (pset->index_map, ptr, &idx))
    return;

  last_idx = pset->size - 1;
  if (idx != last_idx)
    {
      gpointer last_ptr = pset->items[last_idx];
      pset->items[idx] = last_ptr;

      g_ptr_index_map_update (pset->index_map, last_ptr, idx);
    }

  g_ptr_index_map_remove (pset->index_map, ptr);

  pset->size--;
}

static void
g_ptr_set_grow (GPtrSet *pset)
{
  pset->capacity *= 2;
  pset->items = glib_mem_table->realloc (pset->items, pset->capacity * sizeof (gpointer));
}

void
g_ptr_set_foreach (GPtrSet  *pset,
                   GFunc     func,
                   gpointer  user_data)
{
  gsize i;

  for (i = 0; i != pset->size; i++)
    func (pset->items[i], user_data);
}

static GPtrIndexMap *
g_ptr_index_map_new (void)
{
  GPtrIndexMap *map;

  map = glib_mem_table->malloc (sizeof (GPtrIndexMap));
  map->capacity   = G_PTR_SET_INITIAL_CAPACITY;
  map->entries    = glib_mem_table->calloc (map->capacity,
                                            sizeof (GPtrIndexMapEntry));
  map->size       = 0;
  map->tombstones = 0;

  return map;
}

static void
g_ptr_index_map_free (GPtrIndexMap *map)
{
  glib_mem_table->free (map->entries);
  glib_mem_table->free (map);
}

static void
g_ptr_index_map_insert (GPtrIndexMap *map,
                        gpointer      key,
                        gsize         val)
{
  gsize idx;
  gboolean found;

  if ((map->size + map->tombstones) * 10 >= map->capacity * 7)
    {
      gsize new_cap = g_ptr_index_map_adjust_capacity (map->capacity * 2);
      g_ptr_index_map_rehash (map, new_cap);
    }

  idx = g_ptr_index_map_probe (map, key, &found);
  if (found)
    {
      map->entries[idx].val = val;
      return;
    }

  if (map->entries[idx].key == G_PTR_INDEX_MAP_TOMBSTONE)
    map->tombstones--;

  map->entries[idx].key = key;
  map->entries[idx].val = val;
  map->size++;
}

static gboolean
g_ptr_index_map_lookup (GPtrIndexMap  *map,
                        gconstpointer  key,
                        gsize         *out_val)
{
  gsize idx;
  gboolean found;

  if (map->capacity == 0)
    return FALSE;

  idx = g_ptr_index_map_probe (map, key, &found);
  if (!found)
    return FALSE;

  if (out_val != NULL)
    *out_val = map->entries[idx].val;

  return TRUE;
}

static void
g_ptr_index_map_update (GPtrIndexMap *map,
                        gpointer      key,
                        gsize         new_val)
{
  gsize idx;
  gboolean found;

  idx = g_ptr_index_map_probe (map, key, &found);

  if (found)
    map->entries[idx].val = new_val;
}

static void
g_ptr_index_map_remove (GPtrIndexMap  *map,
                        gconstpointer  key)
{
  gsize idx;
  gboolean found;

  if (map->size == 0)
    return;

  idx = g_ptr_index_map_probe (map, key, &found);
  if (!found)
    return;

  map->entries[idx].key = G_PTR_INDEX_MAP_TOMBSTONE;
  map->size--;
  map->tombstones++;
}

static gsize
g_ptr_index_map_probe (GPtrIndexMap  *map,
                       gconstpointer  key,
                       gboolean      *found)
{
  gsize mask, hashv, idx;
  gssize first_tombstone;

  *found = FALSE;

  mask = map->capacity - 1;
  hashv = g_ptr_index_map_ptr_hash (key);
  idx = hashv & mask;
  first_tombstone = -1;

  while (TRUE)
    {
      gpointer slot_key = map->entries[idx].key;

      if (slot_key == NULL)
        {
          if (first_tombstone != -1)
            return (gsize) first_tombstone;
          return idx;
        }
      else if (slot_key == G_PTR_INDEX_MAP_TOMBSTONE)
        {
          if (first_tombstone == -1)
            first_tombstone = idx;
        }
      else if (slot_key == key)
        {
          *found = TRUE;
          return idx;
        }

      idx = (idx + 1) & mask;
    }
}

static void
g_ptr_index_map_rehash (GPtrIndexMap *map,
                        gsize         new_capacity)
{
  GPtrIndexMapEntry *old_entries = map->entries;
  gsize              old_capacity = map->capacity;
  GPtrIndexMapEntry *new_entries;
  gsize i;

  new_entries = glib_mem_table->calloc (new_capacity,
                                        sizeof (GPtrIndexMapEntry));

  map->entries    = new_entries;
  map->capacity   = new_capacity;
  map->size       = 0;
  map->tombstones = 0;

  for (i = 0; i != old_capacity; i++)
    {
      gpointer old_key = old_entries[i].key;
      if (old_key != NULL && old_key != G_PTR_INDEX_MAP_TOMBSTONE)
        g_ptr_index_map_insert (map, old_key, old_entries[i].val);
    }

  glib_mem_table->free (old_entries);
}

static gsize
g_ptr_index_map_adjust_capacity (gsize desired)
{
  gsize cap;

  if (desired < 8)
    return 8;

  cap = 8;
  while (cap < desired)
    cap <<= 1;

  return cap;
}

static gsize
g_ptr_index_map_ptr_hash (gconstpointer ptr)
{
  gsize h;

  h = GPOINTER_TO_SIZE (ptr);
  h ^= h >> 16;
  h *= G_GUINT64_CONSTANT (0x9e3779b97f4a7c15);
  h ^= h >> 23;

  return h;
}
