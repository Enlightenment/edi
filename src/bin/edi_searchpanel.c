#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eo.h>
#include <Eina.h>
#include <Elementary.h>
#include <Efreet_Mime.h>

#include <string.h>
#include "edi_file.h"
#include "edi_searchpanel.h"
#include "edi_theme.h"
#include "edi_config.h"
#include "mainview/edi_mainview.h"

#include "edi_private.h"

static Evas_Object *_info_widget, *_tasks_widget, *_button_search;
static Elm_Code *_elm_code, *_tasks_code;

static Ecore_Thread *_search_thread = NULL;
static Eina_Bool _searching = EINA_FALSE;
static char *_search_text = NULL;

static Eina_Bool
_edi_searchpanel_config_changed_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   elm_code_widget_font_set(_info_widget, _edi_project_config->font.name, _edi_project_config->font.size);
   edi_theme_elm_code_set(_info_widget, _edi_project_config->gui.theme);
   edi_theme_elm_code_alpha_set(_info_widget);

   return ECORE_CALLBACK_RENEW;
}

void
edi_searchpanel_stop(void)
{
   if (_search_thread)
     ecore_thread_cancel(_search_thread);
}

static void
_edi_searchpanel_keypress_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Evas_Object *entry;
   Evas_Event_Key_Down *event;
   const char *text_markup;
   char *text;

   event = event_info;
   entry = obj;

   if (!event) return;

   if (!event->key) return;

   if (!strcmp(event->key, "Return"))
     {
        edi_searchpanel_stop();
        text_markup = elm_object_part_text_get(entry, NULL);
        text = elm_entry_markup_to_utf8(text_markup);
        if (text)
          {
             edi_searchpanel_find(text);
             free(text);
          }
        elm_object_part_text_set(entry, NULL, "");
     }
}

static void
_edi_searchpanel_button_clicked_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                                   void *event_info EINA_UNUSED)
{
   const char *text_markup;
   char *text;
   Evas_Object *entry;

   entry = data;

   if (_search_thread)
     {
        edi_searchpanel_stop();
     }
   else
     {
        text_markup = elm_object_part_text_get(entry, NULL);
        text = elm_entry_markup_to_utf8(text_markup);
        if (text)
          {
             edi_searchpanel_find(text);
             free(text);
          }
        elm_object_part_text_set(entry, NULL, "");
     }
}

static void
_edi_searchpanel_line_clicked_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Code_Line *line;
   const char *content;
   unsigned int length;
   int numlen;
   char *path, *filename_end;
   char *line_start, *line_end, *numstr;

   line = (Elm_Code_Line *) event->info;
   filename_end = line_start = NULL;

   path = strdup(line->data);
   if (!path) return;

   content = elm_code_line_text_get(line, &length);
   if (!content)
     return;

   filename_end = strchr(content, ':');
   if (!filename_end)
     return;

   line_start = filename_end + 1;
   line_end = strchr(line_start, ' ');
   if (!line_end)
     return;

   numlen = line_end - line_start;
   numstr = malloc((numlen + 1) * sizeof(char));
   strncpy(numstr, line_start, numlen);
   numstr[numlen] = '\0';

   edi_mainview_open_path(path);
   edi_mainview_goto(atoi(numstr));

   free(numstr);
}

static char *
edi_searchpanel_line_render(const Eina_File_Line *line, const char *path, size_t *length)
{
   Eina_Strbuf *buf = eina_strbuf_new();
   char *r;
   const char *text = line->start;
   const char *end = line->end;

   while (text < end)
     {
        if (*text != ' ' && *text != '\t' && *text != '\n')
          break;
        text++;
     }

   eina_strbuf_append_printf(buf, "%s:%d ->\t", ecore_file_file_get(path), line->index);
   eina_strbuf_append_length(buf, text, end - text - 1);

   *length = eina_strbuf_length_get(buf);
   r = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);

   return r;
}

typedef struct _Eina_Iterator_Search Eina_Iterator_Search;

struct _Eina_Iterator_Search
{
   Eina_Iterator iterator;

   Eina_File *fp;
   const char *map;
   const char *end;

   Eina_Stringshare *term;

   Eina_File_Line current;

   int boundary;
};

// Return the starting of the last line found and update the count
static inline const char *
edi_count_line(const char *start, unsigned int length, const char *line, unsigned int *count)
{
   const char *cr;
   const char *lf;
   const char *end;

   if (!length) return line;

   lf = memchr(start, '\r', length);
   cr = memchr(start, '\n', length);

   if (!cr && !lf) return start;

   end = lf + 1;
   (*count)++;

   // \r\n
   if (lf && cr == lf + 1)
     {
        end = cr;
     }
   // \n
   else if (cr)
     {
        end = cr;
     }
   // \r
   else if (lf)
     {
        end = lf;
     }

   length = length - (end - start);
   if (length == 0) return start;

   return edi_count_line(end + 1, length - 1, end, count);
}

static inline const char *
edi_end_of_line(const char *start, int boundary, const char *end)
{
   const char *cr;
   const char *lf;
   unsigned long long chunk;

   while (start < end)
     {
        chunk = start + boundary < end ? boundary : end - start;
        lf = memchr(start, '\r', chunk);
        cr = memchr(start, '\n', chunk);

        // \r\n
        if (lf && cr == lf + 1)
          return cr + 1;
        // \n
        if (cr)
          return cr + 1;
        // \r
        if (lf)
          return lf + 1;

        start += chunk;
        boundary = 4096;
     }

   return end;
}

static inline const char *
edi_search_term(const char *start, const char *end, int boundary,
                Eina_Stringshare *term, Eina_File_Line *line)
{
   char end_of_block = 0;

   while (start < end)
     {
        const char *lookup;
        const char *count;
        unsigned long long chunk, cchunk;
        const char *search = start;

        cchunk = chunk = start + boundary < end ? boundary : end - start;
        do
          {
             if ((search + cchunk) > end)
               {
                  cchunk = end - search;
               }
             lookup = memchr(search, *term, cchunk);

             // Did we found the right word or not ?
             if (!lookup)
               break;
             else if (!memcmp(lookup, term, eina_stringshare_strlen(term)))
               break;

             // We didn't, start looking from where we are at
             cchunk -= lookup - search;
             search = lookup + 1;
          }
        while (cchunk > 0);

        // If not found, we want to count starting from the end all the
        // line in this chunk.
        count = lookup ? lookup : start + chunk;

        line->start = edi_count_line(start, count - start, line->start, &line->index);

        // Here we post adjust the counter as we may have double counted a line
        // if \r\n is exactly at the boundary of a chunk. This also only happen
        // when we haven't found what we are looking for yet.
        if (end_of_block == '\r' && *start == '\n')
          line->index--;

        if (lookup) return lookup;

        end_of_block = *(start + chunk - 1);
        start += chunk;
        boundary = 4096;
     }

   return end;
}

static Eina_Bool
edi_search_file_iterator_next(Eina_Iterator_Search *it, void **data)
{
   const char *lookup;
   int line_boundary;

   if (it->end == it->current.end) return EINA_FALSE;

   // We are starting counting at the end of the line, so we will forget
   // to account for the line where we found the term we were looking for,
   // manually adjust for it.
   // This also work to adjust for the first line as we start at zero
   it->current.index++;

   // Account for first iteration when end == NULL
   lookup = edi_search_term(it->current.end ? it->current.end : it->current.start,
                            it->end, it->boundary, it->term, &it->current);

   if (lookup == it->end) return EINA_FALSE;

   line_boundary = (uintptr_t) lookup & 0x3FF;
   if (!line_boundary) line_boundary = 4096;

   it->current.end = edi_end_of_line(lookup, line_boundary, it->end);
   // We need to adjust the end position of the line for '\r\n',
   // in case it is on a cluster boundary.
   if (*it->current.end == '\r')
     {
        if (it->current.end + 1 < it->end &&
            *(it->current.end + 1) == '\n')
          it->current.end += 1;
     }

   it->current.length = it->current.end - it->current.start;

   it->boundary = (uintptr_t) it->current.end & 0x3FF;
   if (!it->boundary) it->boundary = 4096;

   *data = &it->current;
   return EINA_TRUE;
}

static Eina_File *
edi_search_file_iterator_container(Eina_Iterator_Search *it)
{
   return it->fp;
}

static void
edi_search_file_iterator_free(Eina_Iterator_Search *it)
{
   eina_file_map_free(it->fp, (void*) it->map);
   eina_file_close(it->fp);

   EINA_MAGIC_SET(&it->iterator, 0);
   free(it);
}

static Eina_Iterator *
edi_search_file(Eina_File *file, const char *term)
{
   Eina_Iterator_Search *it;
   size_t length;

   if (!file || !term || strlen(term) == 0) return NULL;

   length = eina_file_size_get(file);

   if (!length) return NULL;

   it = calloc(1, sizeof (Eina_Iterator_Search));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->map = eina_file_map_all(file, EINA_FILE_SEQUENTIAL);
   if (!it->map)
     {
        free(it);
        return NULL;
     }

   it->fp = eina_file_dup(it->fp);
   it->current.start = it->map;
   it->current.end = NULL;
   it->current.index = 0;
   it->end = it->map + length;
   it->term = eina_stringshare_add(term);
   it->boundary = 4096;

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(edi_search_file_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(edi_search_file_iterator_container);
   it->iterator.free = FUNC_ITERATOR_FREE(edi_search_file_iterator_free);

   return &it->iterator;
}

typedef struct {
   char *text;
   size_t length;
} Async_Item;

typedef struct {
   Elm_Code *logger;
   Eina_File *f;

   Eina_Inarray texts;
} Async_Log;

static Eina_Spinlock logs_lock;
static unsigned int logs_count = 0;
static Eina_Trash *logs = NULL;

static void
main_loop_line_append_async(void *data)
{
   Async_Log *log = data;
   Async_Item *item;

   while ((item = eina_inarray_pop(&log->texts)))
     {
        elm_code_file_line_append(log->logger->file, item->text, item->length,
                                  strdup(eina_file_filename_get(log->f)));
        free(item->text);
        item->text = NULL;
     }

   eina_file_close(log->f);
   log->f = NULL;
   log->logger = NULL;
   // We are keeping the texts array as it won't be touched by Eina_Trash

   eina_spinlock_take(&logs_lock);
   if (logs_count < 8)
     {
        logs_count++;
        eina_trash_push(&logs, log);
        log = NULL;
     }
   eina_spinlock_release(&logs_lock);

   if (log)
     {
        eina_inarray_flush(&log->texts);
        free(log);
     }
}

void
_edi_searchpanel_search_project_file(const char *path, const char *search_term, Elm_Code *logger)
{
   Eina_Iterator *it;
   Eina_File_Line *l;
   Async_Log *log;
   Eina_File *f;

   f = eina_file_open(path, EINA_FALSE);
   if (!f) return ;

   // If the file looks big.
   if (eina_file_size_get(f) > 2 * 1024 * 1024)
     {
        eina_file_close(f);
        return ;
     }

   eina_spinlock_take(&logs_lock);
   log = eina_trash_pop(&logs);
   if (log) logs_count--;
   eina_spinlock_release(&logs_lock);

   if (!log)
     {
        log = calloc(1, sizeof (Async_Log));
        eina_inarray_step_set(&log->texts, sizeof (log->texts),
                              sizeof (Async_Item), 4);
     }

   log->f = eina_file_dup(f);
   log->logger = logger;

   it = edi_search_file(f, search_term);
   EINA_ITERATOR_FOREACH(it, l)
     {
        Async_Item *item = eina_inarray_grow(&log->texts, 1);

        item->text = edi_searchpanel_line_render(l, path, &item->length);
     }
   eina_iterator_free(it);

   if (eina_inarray_count(&log->texts) == 0)
     {
        eina_inarray_flush(&log->texts);
        eina_file_close(log->f);
        free(log);
        log = NULL;
     }

   if (log) ecore_main_loop_thread_safe_call_async(main_loop_line_append_async, log);

   eina_file_close(f);
}

Eina_Bool
_file_ignore(const char *filename)
{
   if ((eina_str_has_extension(filename, ".png")   ||
        eina_str_has_extension(filename, ".PNG")   ||
        eina_str_has_extension(filename, ".jpg")   ||
        eina_str_has_extension(filename, ".jpeg")  ||
        eina_str_has_extension(filename, ".JPG")   ||
        eina_str_has_extension(filename, ".JPEG")  ||
        eina_str_has_extension(filename, ".bmp")   ||
        eina_str_has_extension(filename, ".dds")   ||
        eina_str_has_extension(filename, ".tgv")   ||
        eina_str_has_extension(filename, ".eet")   ||
        eina_str_has_extension(filename, ".edj")   ||
        eina_str_has_extension(filename, ".gz")    ||
        eina_str_has_extension(filename, ".bz2")   ||
        eina_str_has_extension(filename, ".xz")    ||
        eina_str_has_extension(filename, ".lzma")  ||
        eina_str_has_extension(filename, ".core")  ||
        eina_str_has_extension(filename, ".zip")
       ))
     return EINA_TRUE;

   return EINA_FALSE;
}

void
_edi_searchpanel_search_project(const char *directory, const char *search_term, Elm_Code *logger)
{
   Eina_List *dirs;
   char *dir;

   dirs = eina_list_append(NULL, strdup(directory));

   EINA_LIST_FREE(dirs, dir)
     {
        Eina_File_Direct_Info *info;
        Eina_Iterator *it;

        it = eina_file_stat_ls(dir);
        EINA_ITERATOR_FOREACH(it, info)
          {
             if (_file_ignore(info->path + info->name_start))
               continue ;

             if (edi_file_path_hidden(info->path))
               continue ;

             switch (info->type)
               {
                case EINA_FILE_REG:
                  {
                     if (ecore_thread_check(_search_thread)) return;

                     const char *mime = edi_mime_type_get(info->path);

                     if (strncmp(mime, "text/", 5))
                       break;

                     _edi_searchpanel_search_project_file(info->path, search_term, logger);
                     break;
                  }
                case EINA_FILE_DIR:
                  {
                     dirs = eina_list_append(dirs, strdup(info->path));
                     break;
                  }
                default:
                   // Ignore all other type
                   break;
               }

             if (ecore_thread_check(_search_thread)) break;
          }
        eina_iterator_free(it);

        if (ecore_thread_check(_search_thread)) break;
        free(dir);
     }

   // Cleanup in case of interuption
   EINA_LIST_FREE(dirs, dir)
     free(dir);
}

static void
_search_end_cb(void *data EINA_UNUSED, Ecore_Thread *thread EINA_UNUSED)
{
   elm_object_text_set(_button_search, _("Search"));

   _search_thread = NULL;
   _searching = EINA_FALSE;
}

static void
_search_begin_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   const char *path = data;

   _edi_searchpanel_search_project(path, _search_text, _elm_code);
}

void
edi_searchpanel_find(const char *text)
{
   const char *path;

   if (!text || strlen(text) == 0) return;

   if (_searching)
     {
        ecore_thread_cancel(_search_thread);
        while ((ecore_thread_wait(_search_thread, 0.1)) != EINA_TRUE);
     }

   if (_search_text) free(_search_text);
   _search_text = strdup(text);

   path = edi_project_get();

   elm_code_file_clear(_elm_code->file);
   elm_object_text_set(_button_search, _("Cancel"));

   _searching = EINA_TRUE;
   _search_thread = ecore_thread_feedback_run(_search_begin_cb, NULL,
                                              _search_end_cb, _search_end_cb,
                                              path, EINA_FALSE);
}

void
edi_searchpanel_add(Evas_Object *parent)
{
   Evas_Object *frame, *entry, *box, *hbox, *button;
   Elm_Code_Widget *widget;
   Elm_Code *code;

   frame = elm_frame_add(parent);
   elm_object_text_set(frame, _("Search"));
   evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(frame);

   box = elm_box_add(parent);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);

   hbox = elm_box_add(parent);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   evas_object_show(hbox);

   entry = elm_entry_add(parent);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_entry_editable_set(entry, EINA_TRUE);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_event_callback_add(entry, EVAS_CALLBACK_KEY_DOWN, _edi_searchpanel_keypress_cb, NULL);
   evas_object_show(entry);

   _button_search = button = elm_button_add(parent);
   evas_object_size_hint_weight_set(button, 0.05, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(button, _("Search"));
   elm_box_horizontal_set(button, EINA_TRUE);
   evas_object_smart_callback_add(button, "clicked", _edi_searchpanel_button_clicked_cb, entry);
   evas_object_show(button);

   code = elm_code_create();
   widget = elm_code_widget_add(parent, code);
   edi_theme_elm_code_set(widget, _edi_project_config->gui.theme);
   elm_code_widget_font_set(widget, _edi_project_config->font.name, _edi_project_config->font.size);
   elm_code_widget_gravity_set(widget, 0.0, 1.0);
   efl_event_callback_add(widget, EFL_UI_CODE_WIDGET_EVENT_LINE_CLICKED, _edi_searchpanel_line_clicked_cb, NULL);
   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   _elm_code = code;
   _info_widget = widget;
   eina_spinlock_new(&logs_lock);

   elm_box_pack_end(hbox, entry);
   elm_box_pack_end(hbox, button);

   elm_box_pack_end(box, widget);
   elm_box_pack_end(box, hbox);

   elm_object_content_set(frame, box);
   elm_box_pack_end(parent, frame);

   ecore_event_handler_add(EDI_EVENT_CONFIG_CHANGED, _edi_searchpanel_config_changed_cb, NULL);
}

static void
_edi_taskspanel_line_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Code_Line *line;

   line = (Elm_Code_Line *)event->info;

   line->status = ELM_CODE_STATUS_TYPE_TODO;
}

static void
_tasks_end_cb(void *data EINA_UNUSED, Ecore_Thread *thread EINA_UNUSED)
{
   _search_thread = NULL;
   _searching = EINA_FALSE;
}

static Eina_Bool
_edi_taskspanel_config_changed_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   elm_code_widget_font_set(_tasks_widget, _edi_project_config->font.name, _edi_project_config->font.size);
   edi_theme_elm_code_set(_tasks_widget, _edi_project_config->gui.theme);
   edi_theme_elm_code_alpha_set(_tasks_widget);

   return ECORE_CALLBACK_RENEW;
}

#define _edi_taskspanel_line_clicked_cb _edi_searchpanel_line_clicked_cb

static void
_tasks_begin_cb(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   const char *path = data;

   _edi_searchpanel_search_project(path, "TODO", _tasks_code);
   if (ecore_thread_check(_search_thread)) return;
   _edi_searchpanel_search_project(path, "FIXME", _tasks_code);
   if (ecore_thread_check(_search_thread)) return;
}

void
edi_taskspanel_find(void)
{
   const char *path;
   if (_searching) return;

   elm_code_file_clear(_tasks_code->file);

   path = edi_project_get();

   if (_searching)
     {
        ecore_thread_cancel(_search_thread);
        while ((ecore_thread_wait(_search_thread, 0.1)) != EINA_TRUE);
     }

   _search_thread = ecore_thread_feedback_run(_tasks_begin_cb, NULL,
                                              _tasks_end_cb, _tasks_end_cb,
                                              path, EINA_FALSE);
}

void
edi_taskspanel_add(Evas_Object *parent)
{
   Evas_Object *frame;
   Elm_Code_Widget *widget;
   Elm_Code *code;

   frame = elm_frame_add(parent);
   elm_object_text_set(frame, _("Tasks"));
   evas_object_size_hint_weight_set(frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(frame, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(frame);

   code = elm_code_create();
   widget = elm_code_widget_add(parent, code);
   edi_theme_elm_code_set(widget, _edi_project_config->gui.theme);
   elm_code_widget_font_set(widget, _edi_project_config->font.name, _edi_project_config->font.size);
   elm_code_widget_gravity_set(widget, 0.0, 1.0);
   efl_event_callback_add(widget, &ELM_CODE_EVENT_LINE_LOAD_DONE, _edi_taskspanel_line_cb, NULL);
   efl_event_callback_add(widget, EFL_UI_CODE_WIDGET_EVENT_LINE_CLICKED, _edi_taskspanel_line_clicked_cb, NULL);
   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   _tasks_code = code;
   _tasks_widget = widget;

   elm_object_content_set(frame, widget);
   elm_box_pack_end(parent, frame);

   ecore_event_handler_add(EDI_EVENT_CONFIG_CHANGED, _edi_taskspanel_config_changed_cb, NULL);

   edi_taskspanel_find();
}

