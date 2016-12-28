#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Elementary.h>
#include <Evas.h>

#include "edi_editor.h"
#include "edi_private.h"

typedef struct
{
   Eina_Strbuf *title;
   Eina_Strbuf *detail;
   Eina_Strbuf *param;
   Eina_Strbuf *ret;
   Eina_Strbuf *see;
} Edi_Document;

static void
_edi_doc_free(Edi_Document *doc)
{
   if (!doc) return;

   eina_strbuf_free(doc->title);
   eina_strbuf_free(doc->detail);
   eina_strbuf_free(doc->param);
   eina_strbuf_free(doc->ret);
   eina_strbuf_free(doc->see);
}

#if HAVE_LIBCLANG
static void
_edi_doc_init(Edi_Document *doc)
{
   doc->title = eina_strbuf_new();
   doc->detail = eina_strbuf_new();
   doc->param = eina_strbuf_new();
   doc->ret = eina_strbuf_new();
   doc->see = eina_strbuf_new();
}

static Eina_Bool
_edi_doc_newline_check(Eina_Strbuf *strbuf)
{
   const char *str;

   str = eina_strbuf_string_get(strbuf);

   if (strlen(str) < 4)
     return EINA_TRUE;

   str = str + strlen(str) - 4;

   if (!strcmp(str, "<br>"))
     return EINA_FALSE;
   else
     return EINA_TRUE;
}

static void
_edi_doc_trim(Eina_Strbuf *strbuf)
{
   const char *str;
   int cmp_strlen, ori_strlen;

   str = eina_strbuf_string_get(strbuf);
   ori_strlen = strlen(str);

   if (strlen(str) < 8)
     return;

   cmp_strlen = strlen(str) - 8;
   str += cmp_strlen;

   if (!strcmp(str, "<br><br>"))
     {
        eina_strbuf_remove(strbuf, cmp_strlen, ori_strlen);
        _edi_doc_trim(strbuf);
     }
   else
     return;
}

static void
_edi_doc_title_get(CXCursor cursor, Eina_Strbuf *strbuf)
{
   CXCompletionString str;
   int chunk_num;

   str = clang_getCursorCompletionString(cursor);
   chunk_num = clang_getNumCompletionChunks(str);

   for (int i = 0; i < chunk_num; i++)
     {
        enum CXCompletionChunkKind kind = clang_getCompletionChunkKind(str, i);
        switch (kind)
          {
           case CXCompletionChunk_ResultType:
             eina_strbuf_append_printf(strbuf, "<color=#31d12f><b>%s</b></color><br>",
                        clang_getCString(clang_getCompletionChunkText(str, i)));
             break;
           case CXCompletionChunk_Placeholder:
             eina_strbuf_append_printf(strbuf, "<color=#edd400><b>%s</b></color>",
                        clang_getCString(clang_getCompletionChunkText(str, i)));
             break;
           default:
             eina_strbuf_append(strbuf,
                        clang_getCString(clang_getCompletionChunkText(str, i)));
             break;
          }
     }
}

static void
_edi_doc_dump(Edi_Document *doc, CXComment comment, Eina_Strbuf *strbuf)
{
   const char *str ,*tag;
   enum CXCommentKind kind = clang_Comment_getKind(comment);

   if (kind == CXComment_Null) return;

   switch (kind)
     {
      case CXComment_Text:
        str = clang_getCString(clang_TextComment_getText(comment));

        if (doc->see == strbuf)
          {
             eina_strbuf_append_printf(strbuf, "   %s", str);
             break;
          }
        if (clang_Comment_isWhitespace(comment))
          {
             if (_edi_doc_newline_check(strbuf))
               {
                  if (strbuf == doc->detail)
                    eina_strbuf_append(strbuf, "<br><br>");
                  else
                    eina_strbuf_append(strbuf, "<br>");
               }
             break;
          }
        eina_strbuf_append(strbuf, str);
        break;
      case CXComment_InlineCommand:
        str = clang_getCString(clang_InlineCommandComment_getCommandName(comment));

        if (str[0] == 'p')
          eina_strbuf_append_printf(strbuf, "<font_style=italic>%s</font_style>",
             clang_getCString(clang_InlineCommandComment_getArgText(comment, 0)));
        else if (str[0] == 'c')
          eina_strbuf_append_printf(strbuf, "<b>%s</b>",
             clang_getCString(clang_InlineCommandComment_getArgText(comment, 0)));
        else
          eina_strbuf_append_printf(strbuf, "@%s", str);
        break;
      case CXComment_BlockCommand:
        tag = clang_getCString(clang_BlockCommandComment_getCommandName(comment));

        if (!strcmp(tag, "return"))
          strbuf = doc->ret;
        else if (!strcmp(tag, "see"))
          strbuf = doc->see;

        break;
      case CXComment_ParamCommand:
        str = clang_getCString(clang_ParamCommandComment_getParamName(comment));
        strbuf = doc->param;

        eina_strbuf_append_printf(strbuf, "<color=#edd400><b>   %s</b></color>",
                                  str);
        break;
      case CXComment_VerbatimBlockLine:
        str = clang_getCString(clang_VerbatimBlockLineComment_getText(comment));

        if (str[0] == 10)
          {
             eina_strbuf_append(strbuf, "<br>");
             break;
          }
        eina_strbuf_append_printf(strbuf, "%s<br>", str);
        break;
      case CXComment_VerbatimLine:
        str = clang_getCString(clang_VerbatimLineComment_getText(comment));

        if (doc->see == strbuf)
          eina_strbuf_append(strbuf, str);
        break;
      default:
        break;
     }
   for (unsigned i = 0; i < clang_Comment_getNumChildren(comment); i++)
     _edi_doc_dump(doc, clang_Comment_getChild(comment, i), strbuf);
}

static void
_edi_doc_font_set(Edi_Document *doc, const char *font, int font_size)
{
   char *format = "<align=left><font=\'%s\'><font_size=%d>";
   char *font_head;
   char *font_tail = "</font_size></font></align>";
   int displen;

   displen = strlen(format) + strlen(font);
   font_head = malloc(sizeof(char) * displen);
   snprintf(font_head, displen, format, font, font_size);

   if (strlen(eina_strbuf_string_get(doc->title)) > 0)
     {
        eina_strbuf_prepend(doc->title, font_head);
        eina_strbuf_append(doc->title, font_tail);
     }

   if (strlen(eina_strbuf_string_get(doc->detail)) > 0)
     {
        eina_strbuf_prepend(doc->detail, font_head);
        eina_strbuf_append(doc->detail, font_tail);
     }

   if (strlen(eina_strbuf_string_get(doc->param)) > 0)
     {
        eina_strbuf_prepend(doc->param, font_head);
        eina_strbuf_append(doc->param, font_tail);
     }

   if (strlen(eina_strbuf_string_get(doc->ret)) > 0)
     {
        eina_strbuf_prepend(doc->ret, font_head);
        eina_strbuf_append(doc->ret, font_tail);
     }

   if (strlen(eina_strbuf_string_get(doc->see)) > 0)
     {
        eina_strbuf_prepend(doc->see, font_head);
        eina_strbuf_append(doc->see, font_tail);
     }

   free(font_head);
}

static void
_edi_doc_tag_name_set(Edi_Document *doc)
{
   if (strlen(eina_strbuf_string_get(doc->param)) > 0)
     eina_strbuf_prepend(doc->param, "<b><br><br> Parameters<br></b>");

   if (strlen(eina_strbuf_string_get(doc->ret)) > 0)
     eina_strbuf_prepend(doc->ret, "<b><br><br> Returns<br></b>   ");

   if (strlen(eina_strbuf_string_get(doc->see)) > 0)
     eina_strbuf_prepend(doc->see, "<b><br><br> See also<br></b>");
}

static CXCursor
_edi_doc_cursor_get(Edi_Editor *editor, CXIndex idx, CXTranslationUnit unit)
{
   CXFile cxfile;
   CXSourceLocation location;
   CXCursor cursor;
   struct CXUnsavedFile unsaved_file;
   Elm_Code *code;
   const char *path, *args;
   char **clang_argv;
   unsigned int clang_argc, row, col, end_row, end_col;

   code = elm_code_widget_code_get(editor->entry);
   path = elm_code_file_path_get(code->file);
   elm_code_widget_cursor_position_get(editor->entry, &row, &col);

   end_row = elm_code_file_lines_get(code->file);
   end_col = elm_code_file_line_get(code->file, end_row)->length;

   unsaved_file.Filename = path;
   unsaved_file.Contents = elm_code_widget_text_between_positions_get(
                                         editor->entry, 1, 1, end_row, end_col);
   unsaved_file.Length = strlen(unsaved_file.Contents);

   //Initialize Clang
   args = "-I/usr/inclue/ " EFL_CFLAGS " " CLANG_INCLUDES " -Wall -Wextra";
   clang_argv = eina_str_split_full(args, " ", 0, &clang_argc);

   idx = clang_createIndex(0, 0);

   unit = clang_parseTranslationUnit(idx, path, (const char *const *)clang_argv,
                                  (int)clang_argc, &unsaved_file, 1,
                                  clang_defaultEditingTranslationUnitOptions());

   cxfile = clang_getFile(unit, path);
   location = clang_getLocation(unit, cxfile, row, col);
   cursor = clang_getCursor(unit, location);

   return clang_getCursorReferenced(cursor);
}
#endif

static Edi_Document *
_edi_doc_get(Edi_Editor *editor)
{
   Edi_Document *doc = NULL;
#if HAVE_LIBCLANG
   CXIndex idx = NULL;
   CXTranslationUnit unit = NULL;
   CXCursor cursor;
   CXComment comment;
   const char *font;
   int font_size;

   cursor = _edi_doc_cursor_get(editor, idx, unit);
   comment = clang_Cursor_getParsedComment(cursor);

   if (clang_Comment_getKind(comment) == CXComment_Null)
     {
        clang_disposeTranslationUnit(unit);
        clang_disposeIndex(idx);
        return NULL;
     }

   doc = malloc(sizeof(Edi_Document));

   _edi_doc_init(doc);
   _edi_doc_dump(doc, comment, doc->detail);
   _edi_doc_title_get(cursor, doc->title);
   _edi_doc_trim(doc->detail);

   _edi_doc_tag_name_set(doc);
   elm_code_widget_font_get(editor->entry, &font, &font_size);
   _edi_doc_font_set(doc, font, font_size);

   clang_disposeTranslationUnit(unit);
   clang_disposeIndex(idx);
#endif
   return doc;
}

static void
_edi_doc_popup_cb_block_clicked(void *data EINA_UNUSED, Evas_Object *obj,
                                void *event_info EINA_UNUSED)
{
   evas_object_del(obj);
}

static void
_edi_doc_popup_cb_btn_clicked(void *data, Evas_Object *obj EINA_UNUSED,
                              void *event_info EINA_UNUSED)
{
   evas_object_del(data);
}

static void
_edi_doc_popup_cb_key_down(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                           Evas_Object *obj, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;

   if (!strcmp(ev->key, "Escape"))
     evas_object_del(obj);
}

static void
_edi_doc_popup_cb_del(void *data, Evas *e EINA_UNUSED,
                     Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Document *doc = data;

   _edi_doc_free(doc);
}

void
edi_editor_doc_open(Edi_Editor *editor)
{
   Edi_Document *doc;
   const char *detail, *param, *ret, *see;
   char *display;
   int displen;
   Evas_Coord w, h;

   doc = _edi_doc_get(editor);

   //Popup
   editor->doc_popup = elm_popup_add(editor->entry);
   evas_object_smart_callback_add(editor->doc_popup, "block,clicked",
                                  _edi_doc_popup_cb_block_clicked, NULL);
   evas_object_event_callback_add(editor->doc_popup, EVAS_CALLBACK_KEY_DOWN,
                                  _edi_doc_popup_cb_key_down, NULL);
   evas_object_event_callback_add(editor->doc_popup, EVAS_CALLBACK_DEL,
                                  _edi_doc_popup_cb_del, doc);
   if (!doc)
     {
        elm_popup_timeout_set(editor->doc_popup, 1.5);
        elm_object_style_set(editor->doc_popup, "transparent");
        elm_object_text_set(editor->doc_popup, "No help available for this term");
        evas_object_show(editor->doc_popup);
        return;
     }

   detail = eina_strbuf_string_get(doc->detail);
   param = eina_strbuf_string_get(doc->param);
   ret = eina_strbuf_string_get(doc->ret);
   see = eina_strbuf_string_get(doc->see);

   displen = strlen(detail) + strlen(param) + strlen(ret) + strlen(see);
   display = malloc(sizeof(char) * (displen + 1));
   snprintf(display, displen, "%s%s%s%s", detail, param, ret, see);

   //Close button
   Evas_Object *btn = elm_button_add(editor->doc_popup);
   elm_object_text_set(btn, "Close");
   evas_object_smart_callback_add(btn, "clicked", _edi_doc_popup_cb_btn_clicked,
                                  editor->doc_popup);
   elm_object_part_content_set(editor->doc_popup, "button1", btn);
   evas_object_show(btn);

   //Grid
   Evas_Object *grid = elm_grid_add(editor->doc_popup);
   elm_grid_size_set(grid, 100, 100);
   evas_object_size_hint_weight_set(grid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_geometry_get(editor->entry, NULL, NULL, &w, &h);
   evas_object_size_hint_min_set(grid, w * 0.8 * elm_config_scale_get(),
                                 h * 0.8 * elm_config_scale_get());
   elm_object_content_set(editor->doc_popup, grid);
   evas_object_show(grid);

   //Background
   Evas_Object *bg = elm_bg_add(grid);
   elm_bg_color_set(bg, 53, 53, 53);
   elm_grid_pack(grid, bg, 1, 1, 98, 98);
   evas_object_show(bg);

   //Box
   Evas_Object *box = elm_box_add(grid);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_grid_pack(grid, box, 1, 1, 98, 98);
   evas_object_show(box);

   //Title
   Evas_Object *title = elm_entry_add(box);
   elm_entry_editable_set(title, EINA_FALSE);
   elm_object_text_set(title, eina_strbuf_string_get(doc->title));
   evas_object_size_hint_weight_set(title, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(title, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(title);
   elm_box_pack_end(box, title);

   //Entry
   Evas_Object *entry = elm_entry_add(box);
   elm_object_text_set(entry, display);
   elm_entry_editable_set(entry, EINA_FALSE);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   elm_scroller_policy_set(entry, ELM_SCROLLER_POLICY_OFF,
                           ELM_SCROLLER_POLICY_AUTO);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(entry);
   elm_box_pack_end(box, entry);

   evas_object_show(editor->doc_popup);

   free(display);
}
