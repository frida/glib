/* Trimmed down version of GSList which only relies on libc */

#include <stdlib.h>

typedef struct _GTinyList GTinyList;

struct _GTinyList
{
  gpointer data;
  GTinyList *next;
};

static void
g_tinylist_free (GTinyList *list)
{
  while (list)
    {
      GTinyList *next = list->next;
      free (list);
      list = next;
    }
}

static GTinyList *
g_tinylist_prepend (GTinyList *list,
        gpointer   data)
{
  GTinyList *new_list;

  new_list = malloc (sizeof (GTinyList));
  new_list->data = data;
  new_list->next = list;

  return new_list;
}

static GTinyList *
g_tinylist_remove (GTinyList  *list,
       gconstpointer data)
{
  GTinyList *tmp, *prev = NULL;

  tmp = list;
  while (tmp)
    {
      if (tmp->data == data)
        {
          if (prev)
            prev->next = tmp->next;
          else
            list = tmp->next;

          free (tmp);
          break;
        }
      prev = tmp;
      tmp = prev->next;
    }

  return list;
}

static void
g_tinylist_foreach (GTinyList *list,
        GFunc  func,
        gpointer   user_data)
{
  while (list)
    {
      GTinyList *next = list->next;
      (*func) (list->data, user_data);
      list = next;
    }
}