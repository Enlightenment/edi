#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libgen.h>

#include <Eina.h>
#include <Elementary.h>

#include "edi_editor.h"

#define CLANG_DEBUG 0
#if CLANG_DEBUG
#include "clang_debug.h"
#endif

#include "mainview/edi_mainview.h"
#include "edi_config.h"

#include "edi_private.h"

#define Edi_Color const char *

static Edi_Color EDI_COLOR_FOREGROUND = "<color=#ffffff>";
static Edi_Color EDI_COLOR_COMMENT = "<color=#3399ff>";
static Edi_Color EDI_COLOR_STRING = "<color=#ff5a35>";
static Edi_Color EDI_COLOR_NUMBER = "<color=#D4D42A>";// font_weight=Bold";
static Edi_Color EDI_COLOR_BRACE = "<color=#656565>";
static Edi_Color EDI_COLOR_TYPE = "<color=#3399ff>";
static Edi_Color EDI_COLOR_CLASS = "<color=#72AAD4>";// font_weight=Bold";
static Edi_Color EDI_COLOR_FUNCTION = "<color=#72AAD4>";// font_weight=Bold";
//static Edi_Color EDI_COLOR_PARAM = "<color=#ffffff>";
static Edi_Color EDI_COLOR_KEYWORD = "<color=#ff9900>";// font_weight=Bold";
static Edi_Color EDI_COLOR_PREPROCESSOR = "<color=#00B000>";

static Edi_Color EDI_COLOR_BACKGROUND = "<backing_color=#000000>";
static Edi_Color EDI_COLOR_SEVIRITY_IGNORED = "<backing_color=#000000>";
static Edi_Color EDI_COLOR_SEVIRITY_NOTE = "<backing_color=#ff9900>";
static Edi_Color EDI_COLOR_SEVIRITY_WARNING = "<backing_color=#ff9900>";

static char *_default_font;

typedef struct
{
   unsigned int line;
   unsigned int col;
} Edi_Location;

typedef struct
{
   Edi_Location start;
   Edi_Location end;
} Edi_Range;

static void
_update_lines(Edi_Editor *editor);

static void
_update_highlight(Edi_Editor *editor);

static void
_reset_highlight(Edi_Editor *editor);

static const char *
_edi_editor_font_get()
{
   char *format;

   if (_default_font)
     return _default_font;

   format = "DEFAULT='font=Monospace font_size=%d'";

   _default_font = malloc(sizeof(char) * (strlen(format) + 4));
   snprintf(_default_font, strlen(format) + 4, format, _edi_cfg->font.size);
   return _default_font;
}

static void
_undo_do(Edi_Editor *editor, Elm_Entry_Change_Info *inf)
{
   if (inf->insert)
     {
        const Evas_Object *tb = elm_entry_textblock_get(editor->entry);
        Evas_Textblock_Cursor *mcur, *end;
        mcur = (Evas_Textblock_Cursor *) evas_object_textblock_cursor_get(tb);
        end = evas_object_textblock_cursor_new(tb);

        if (inf->insert)
          {
             elm_entry_cursor_pos_set(editor->entry, inf->change.insert.pos);
             evas_textblock_cursor_pos_set(end, inf->change.insert.pos +
                   inf->change.insert.plain_length);
          }
        else
          {
             elm_entry_cursor_pos_set(editor->entry, inf->change.del.start);
             evas_textblock_cursor_pos_set(end, inf->change.del.end);
          }

        evas_textblock_cursor_range_delete(mcur, end);
        evas_textblock_cursor_free(end);
        elm_entry_calc_force(editor->entry);
     }
   else
     {
        if (inf->insert)
          {
             elm_entry_cursor_pos_set(editor->entry, inf->change.insert.pos);
             elm_entry_entry_insert(editor->entry, inf->change.insert.content);
          }
        else
          {
             size_t start;
             start = (inf->change.del.start < inf->change.del.end) ?
                inf->change.del.start : inf->change.del.end;

             elm_entry_cursor_pos_set(editor->entry, start);
             elm_entry_entry_insert(editor->entry, inf->change.insert.content);
             elm_entry_cursor_pos_set(editor->entry, inf->change.del.end);
          }
     }
}

static void
_undo_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Elm_Entry_Change_Info *change;
   Edi_Editor *editor = data;

   if (!eina_list_next(editor->undo_stack))
      return;

   change = eina_list_data_get(editor->undo_stack);
   _undo_do(editor, change);
   editor->undo_stack = eina_list_next(editor->undo_stack);
}

static void
_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Entry_Change_Info *change;
   Edi_Editor *editor = data;

   change = calloc(1, sizeof(*change));
   memcpy(change, event_info, sizeof(*change));
   if (change->insert)
     {
        eina_stringshare_ref(change->change.insert.content);
     }
   else
     {
        eina_stringshare_ref(change->change.del.content);
     }

   editor->undo_stack = eina_list_prepend(editor->undo_stack, change);
   _update_lines(editor);
}

static void
_smart_cb_key_down(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                   Evas_Object *obj EINA_UNUSED, void *event)
{
   Edi_Mainview_Item *item;
   Edi_Editor *editor;
   Evas_Object *content;
   Eina_Bool ctrl, alt, shift;
   Evas_Event_Key_Down *ev = event;

   ctrl = evas_key_modifier_is_set(ev->modifiers, "Control");
   alt = evas_key_modifier_is_set(ev->modifiers, "Alt");
   shift = evas_key_modifier_is_set(ev->modifiers, "Shift");

   item = edi_mainview_item_current_get();
   content = elm_object_item_content_get(item->view);
   editor = (Edi_Editor *)evas_object_data_get(content, "editor");

   if ((!alt) && (ctrl) && (!shift))
     {
        if (!strcmp(ev->key, "Prior"))
          {
             edi_mainview_item_prev();
          }
        else if (!strcmp(ev->key, "Next"))
          {
             edi_mainview_item_next();
          }
        else if (!strcmp(ev->key, "s"))
          {
             edi_mainview_save();
             _reset_highlight(editor);
          }
        else if (!strcmp(ev->key, "f"))
          {
             edi_mainview_search();
          }
        else if (!strcmp(ev->key, "z"))
          {
             _undo_cb(editor, obj, event);
          }
     }
}

static int
_get_lines_in_textblock(Evas_Object *textblock)
{
   int lines;
   Evas_Textblock_Cursor *cursor;

   cursor = evas_object_textblock_cursor_new(textblock);
   evas_textblock_cursor_paragraph_last(cursor);
   lines = evas_textblock_cursor_geometry_get(cursor, NULL, NULL, NULL, NULL, NULL, EVAS_TEXTBLOCK_CURSOR_BEFORE);

   evas_textblock_cursor_free(cursor);
   return lines + 1;
}

static void
_update_lines(Edi_Editor *editor)
{
   Eina_Strbuf *content;
   int lines, i;

   lines = _get_lines_in_textblock(elm_entry_textblock_get(editor->entry));

   content = eina_strbuf_new();
   eina_strbuf_append(content, "<align=right>");
   for (i = 1; i <= lines; i++)
     {
        eina_strbuf_append_printf(content, "%d<br>", i);
     }
   eina_strbuf_append(content, "<br><br></align>");
   elm_object_text_set(editor->lines, eina_strbuf_string_get(content));
   eina_strbuf_free(content);
}

static void
_scroll_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Editor *editor = data;
   Evas_Coord y, h;

// TODO ignore y scrolls - just return;
   elm_scroller_region_get(editor->entry, NULL, &y, NULL, NULL);
   elm_scroller_region_get(editor->lines, NULL, NULL, NULL, &h);
   elm_scroller_region_show(editor->lines, 0, y, 10, h);

   _update_highlight(editor);
}

static void
_resize_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Editor *editor = data;

   _update_highlight(editor);
}

static void
_edit_cursor_moved(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   char buf[30];
   int line;
   int col;
   Evas_Object *tb;
   Evas_Textblock_Cursor *cur1, *cur2;

   tb = elm_entry_textblock_get(obj);

   cur1 = evas_object_textblock_cursor_get(tb);
   cur2 = evas_object_textblock_cursor_new(tb);
   line = evas_textblock_cursor_line_geometry_get(cur1, NULL, NULL, NULL, NULL) + 1;
   evas_textblock_cursor_copy(cur1, cur2);
   evas_textblock_cursor_line_char_first(cur2);
   col = evas_textblock_cursor_pos_get(cur1) -
      evas_textblock_cursor_pos_get(cur2) + 1;
   evas_textblock_cursor_free(cur2);

   snprintf(buf, sizeof(buf), "Line:%d, Column:%d", line, col);
   elm_object_text_set((Evas_Object *)data, buf);
}

static void
_edi_editor_statusbar_add(Evas_Object *panel, Edi_Editor *editor, Edi_Mainview_Item *item)
{
   Evas_Object *position, *mime;

   elm_box_horizontal_set(panel, EINA_TRUE);

   mime = elm_label_add(panel);
   if (item->mimetype)
     {
        elm_object_text_set(mime, item->mimetype);
     }
   else
     {
        elm_object_text_set(mime, item->editortype);
     }
   evas_object_size_hint_align_set(mime, 0.0, 0.5);
   evas_object_size_hint_weight_set(mime, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(panel, mime);
   evas_object_show(mime);
   elm_object_disabled_set(mime, EINA_TRUE);

   position = elm_label_add(panel);
   evas_object_size_hint_align_set(position, 1.0, 0.5);
   evas_object_size_hint_weight_set(position, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(panel, position);
   evas_object_show(position);
   elm_object_disabled_set(position, EINA_TRUE);

   _edit_cursor_moved(position, editor->entry, NULL);
   evas_object_smart_callback_add(editor->entry, "cursor,changed", _edit_cursor_moved, position);
}

#if HAVE_LIBCLANG
// TODO on any refresh heck mtime - then re-run clang if changed - it should be fast enough now...
static void
_edi_line_color_remove(Edi_Editor *editor, unsigned int line)
{
   Eina_List *formats;
   Evas_Object *textblock;
   Evas_Textblock_Cursor *start, *end;
   Evas_Object_Textblock_Node_Format *format;
   unsigned int i;

   textblock = elm_entry_textblock_get(editor->entry);
   start = evas_object_textblock_cursor_new(textblock);
   evas_textblock_cursor_line_set(start, line);
   evas_textblock_cursor_pos_set(start, evas_textblock_cursor_pos_get(start));
   end = evas_object_textblock_cursor_new(textblock);
   evas_textblock_cursor_line_set(end, line);
   evas_textblock_cursor_pos_set(end, evas_textblock_cursor_pos_get(end) - 1);

   i = 0;
   formats = evas_textblock_cursor_range_formats_get(start, end);
   while (eina_list_count(formats) > i)
     {
        format = eina_list_nth(formats, i);

        if (!strncmp("+ color", evas_textblock_node_format_text_get(format), 7))
          evas_textblock_node_format_remove_pair(textblock, format);
        else
          i++;

        formats = evas_textblock_cursor_range_formats_get(start, end);
     }

   evas_textblock_cursor_free(start);
   evas_textblock_cursor_free(end);
}

static void
_edi_range_color_set(Edi_Editor *editor, Edi_Range range, Edi_Color color)
{
   Evas_Textblock *textblock;
   Evas_Textblock_Cursor *cursor;

   ecore_thread_main_loop_begin();

   if (!((Evas_Coord)range.start.line > editor->format_end || (Evas_Coord)range.end.line < editor->format_start))
     {
        textblock = elm_entry_textblock_get(editor->entry);

        if (editor->format_line == -1)
          editor->format_line = range.start.line - 1;
        while (editor->format_line < (int) range.end.line)
          {
             _edi_line_color_remove(editor, ++editor->format_line);
          }

        cursor = evas_object_textblock_cursor_new(textblock);
        evas_textblock_cursor_line_set(cursor, range.start.line - 1);
        evas_textblock_cursor_pos_set(cursor, evas_textblock_cursor_pos_get(cursor) + range.start.col - 1);
        evas_textblock_cursor_format_prepend(cursor, color);

        evas_textblock_cursor_line_set(cursor, range.end.line - 1);
        evas_textblock_cursor_pos_set(cursor, evas_textblock_cursor_pos_get(cursor) + range.end.col - 1);
        evas_textblock_cursor_format_prepend(cursor, "</color>");

        evas_textblock_cursor_free(cursor);
     }
   ecore_thread_main_loop_end();
}

static void
_clang_load_highlighting(const char *path, Edi_Editor *editor)
{
        CXFile cfile = clang_getFile(editor->tx_unit, path);

        CXSourceRange range = clang_getRange(
              clang_getLocationForOffset(editor->tx_unit, cfile, 0),
              clang_getLocationForOffset(editor->tx_unit, cfile, eina_file_size_get(eina_file_open(path, EINA_FALSE))));

        clang_tokenize(editor->tx_unit, range, &editor->tokens, &editor->token_count);
        /* FIXME: We should use annotate tokens and then use a lot more
         * color classes. I don't know why it's broken ATM... :( */
        editor->cursors = (CXCursor *) malloc(editor->token_count * sizeof(CXCursor));
        clang_annotateTokens(editor->tx_unit, editor->tokens, editor->token_count, editor->cursors);
}

static void *
_clang_show_highlighting(void *data)
{
   Edi_Editor *editor;
   unsigned int i = 0;

   editor = (Edi_Editor *)data;
   editor->format_line = -1;
   for (i = 0 ; i < editor->token_count ; i++)
     {
        Edi_Range range;
        Edi_Color color = EDI_COLOR_FOREGROUND;

        CXSourceRange tkrange = clang_getTokenExtent(editor->tx_unit, editor->tokens[i]);
        clang_getSpellingLocation(clang_getRangeStart(tkrange), NULL,
              &range.start.line, &range.start.col, NULL);
        clang_getSpellingLocation(clang_getRangeEnd(tkrange), NULL,
              &range.end.line, &range.end.col, NULL);
        /* FIXME: Should probably do something fancier, this is only a limited
         * number of types. */
        switch (clang_getTokenKind(editor->tokens[i]))
          {
             case CXToken_Punctuation:
                if (i < editor->token_count - 1 && range.start.col == 1 &&
                    (clang_getTokenKind(editor->tokens[i + 1]) == CXToken_Identifier && (editor->cursors[i + 1].kind == CXCursor_MacroDefinition ||
                    editor->cursors[i + 1].kind == CXCursor_InclusionDirective || editor->cursors[i + 1].kind == CXCursor_PreprocessingDirective)))
                  color = EDI_COLOR_PREPROCESSOR;
                else
                  color = EDI_COLOR_BRACE;
                break;
             case CXToken_Identifier:
                if (editor->cursors[i].kind < CXCursor_FirstRef)
                  {
                      color = EDI_COLOR_CLASS;
                      break;
                  }
                switch (editor->cursors[i].kind)
                  {
                   case CXCursor_DeclRefExpr:
                      /* Handle different ref kinds */
                      color = EDI_COLOR_FUNCTION;
                      break;
                   case CXCursor_MacroDefinition:
                   case CXCursor_InclusionDirective:
                   case CXCursor_PreprocessingDirective:
                      color = EDI_COLOR_PREPROCESSOR;
                      break;
                   case CXCursor_TypeRef:
                      color = EDI_COLOR_TYPE;
                      break;
                   case CXCursor_MacroExpansion:
                      color = EDI_COLOR_PREPROCESSOR;//_MACRO_EXPANSION;
                      break;
                   default:
                      color = EDI_COLOR_FOREGROUND;
                      break;
                  }
                break;
             case CXToken_Keyword:
                switch (editor->cursors[i].kind)
                  {
                   case CXCursor_PreprocessingDirective:
                      color = EDI_COLOR_PREPROCESSOR;
                      break;
                   case CXCursor_CaseStmt:
                   case CXCursor_DefaultStmt:
                   case CXCursor_IfStmt:
                   case CXCursor_SwitchStmt:
                   case CXCursor_WhileStmt:
                   case CXCursor_DoStmt:
                   case CXCursor_ForStmt:
                   case CXCursor_GotoStmt:
                   case CXCursor_IndirectGotoStmt:
                   case CXCursor_ContinueStmt:
                   case CXCursor_BreakStmt:
                   case CXCursor_ReturnStmt:
                   case CXCursor_AsmStmt:
                   case CXCursor_ObjCAtTryStmt:
                   case CXCursor_ObjCAtCatchStmt:
                   case CXCursor_ObjCAtFinallyStmt:
                   case CXCursor_ObjCAtThrowStmt:
                   case CXCursor_ObjCAtSynchronizedStmt:
                   case CXCursor_ObjCAutoreleasePoolStmt:
                   case CXCursor_ObjCForCollectionStmt:
                   case CXCursor_CXXCatchStmt:
                   case CXCursor_CXXTryStmt:
                   case CXCursor_CXXForRangeStmt:
                   case CXCursor_SEHTryStmt:
                   case CXCursor_SEHExceptStmt:
                   case CXCursor_SEHFinallyStmt:
//                      color = EDI_COLOR_KEYWORD_STMT;
                      break;
                   default:
                      color = EDI_COLOR_KEYWORD;
                      break;
                  }
                break;
             case CXToken_Literal:
                if (editor->cursors[i].kind == CXCursor_StringLiteral || editor->cursors[i].kind == CXCursor_CharacterLiteral)
                  color = EDI_COLOR_STRING;
                else
                  color = EDI_COLOR_NUMBER;
                break;
             case CXToken_Comment:
                color = EDI_COLOR_COMMENT;
                break;
          }

        _edi_range_color_set(editor, range, color);

#if CLANG_DEBUG
        const char *kind = NULL;
        switch (clang_getTokenKind(editor->tokens[i])) {
           case CXToken_Punctuation: kind = "Punctuation"; break;
           case CXToken_Keyword: kind = "Keyword"; break;
           case CXToken_Identifier: kind = "Identifier"; break;
           case CXToken_Literal: kind = "Literal"; break;
           case CXToken_Comment: kind = "Comment"; break;
        }

        printf("%s ", kind);
        PrintToken(editor->tx_unit, editor->tokens[i]);

        if (!clang_isInvalid(editor->cursors[i].kind)) {
             printf(" ");
             PrintCursor(editor->cursors[i]);
        }

        printf("\n");
#endif
     }

// TODO clear these when we reset next time
//   free(editor->cursors);
//   clang_disposeTokens(editor->tx_unit, editor->tokens, editor->token_count);

   return NULL;
}

static void
_clang_load_errors(const char *path EINA_UNUSED, Edi_Editor *editor)
{
   unsigned n = clang_getNumDiagnostics(editor->tx_unit);
   unsigned i = 0;

   for(i = 0, n = clang_getNumDiagnostics(editor->tx_unit); i != n; ++i)
     {
        CXDiagnostic diag = clang_getDiagnostic(editor->tx_unit, i);
        Edi_Range range;

        clang_getSpellingLocation(clang_getDiagnosticLocation(diag), NULL, &range.start.line, &range.start.col, NULL);
        range.end = range.start;

        /* FIXME: Also handle ranges and fix suggestions. */

        Edi_Color color = EDI_COLOR_BACKGROUND;

        switch (clang_getDiagnosticSeverity(diag))
          {
           case CXDiagnostic_Ignored:
              color = EDI_COLOR_SEVIRITY_IGNORED;
              break;
           case CXDiagnostic_Note:
              color = EDI_COLOR_SEVIRITY_NOTE;
              break;
           case CXDiagnostic_Warning:
              color = EDI_COLOR_SEVIRITY_WARNING;
              break;
           case CXDiagnostic_Error:
//              color = EDI_COLOR_BACKGROUND_SEVIRITY_ERROR;
              break;
           case CXDiagnostic_Fatal:
//              color = EDI_COLOR_BACKGROUND_SEVIRITY_FATAL;
              break;
          }

        _edi_range_color_set(editor, range, color);

#if CLANG_DEBUG
        CXString str = clang_formatDiagnostic(diag, clang_defaultDiagnosticDisplayOptions());
        printf("DEBUG: Diag:%s\n", clang_getCString(str));
        clang_disposeString(str);
#endif

        clang_disposeDiagnostic(diag);
     }
}

static Eina_Bool
_edi_clang_render(void *data)
{
   pthread_attr_t attr;
   pthread_t thread_id;

   if (pthread_attr_init(&attr) != 0)
     perror("pthread_attr_init");
   if (pthread_create(&thread_id, &attr, _clang_show_highlighting, data) != 0)
     perror("pthread_create");

   return ECORE_CALLBACK_CANCEL;
}

static void *
_edi_clang_setup(void *data)
{
   Edi_Editor *editor;
   const char *path;

   editor = (Edi_Editor *)data;
   elm_entry_file_get(editor->entry, &path, NULL);

   /* Clang */
   /* FIXME: index should probably be global. */
   const char const *clang_argv[] = {"-I/usr/lib/clang/3.1/include/", "-Wall", "-Wextra"};
   int clang_argc = sizeof(clang_argv) / sizeof(*clang_argv);

   editor->idx = clang_createIndex(0, 0);

   /* FIXME: Possibly activate more options? */
   editor->tx_unit = clang_parseTranslationUnit(editor->idx, path, clang_argv, clang_argc, NULL, 0, clang_defaultEditingTranslationUnitOptions() | CXTranslationUnit_DetailedPreprocessingRecord);

   _clang_load_errors(path, editor);
   _clang_load_highlighting(path, editor);

   _edi_clang_render(editor);

   return NULL;
}

/*
TODO - USE ME!
static void
_edi_clang_dispose(Edi_Editor *editor)
{
   clang_disposeTranslationUnit(editor->tx_unit);
   clang_disposeIndex(editor->idx);
}
*/
#endif

static void
_update_highlight_window(Edi_Editor *editor)
{
   Evas_Object *textblock;
   Evas_Textblock_Cursor *begin, *limit;

   textblock = elm_entry_textblock_get(editor->entry);

   begin = evas_object_textblock_cursor_new(textblock);
   limit = evas_object_textblock_cursor_new(textblock);
   evas_textblock_cursor_visible_range_get(begin, limit);

   editor->format_start = evas_textblock_cursor_line_geometry_get(begin, NULL, NULL, NULL, NULL);
   editor->format_end = evas_textblock_cursor_line_geometry_get(limit, NULL, NULL, NULL, NULL);
   evas_textblock_cursor_free(begin);
   evas_textblock_cursor_free(limit);
}

static void
_reset_highlight(Edi_Editor *editor)
{
   _update_highlight_window(editor);

#if HAVE_LIBCLANG
   pthread_attr_t attr;
   pthread_t thread_id;

   if (pthread_attr_init(&attr) != 0)
     perror("pthread_attr_init");
   if (pthread_create(&thread_id, &attr, _edi_clang_setup, editor) != 0)
     perror("pthread_create");
#endif
}

static void
_update_highlight(Edi_Editor *editor)
{
   _update_highlight_window(editor);

#if HAVE_LIBCLANG
   if (editor->delay_highlight)
     ecore_timer_del(editor->delay_highlight);

   editor->delay_highlight = ecore_timer_add(0.25, _edi_clang_render, editor);
#endif
}

static void
_text_set_done(void *data, Evas_Object *obj EINA_UNUSED, void *source EINA_UNUSED)
{
   Edi_Editor *editor = (Edi_Editor *) data;

   _update_lines(editor);
   _reset_highlight(editor);
}

EAPI Evas_Object *edi_editor_add(Evas_Object *parent, Edi_Mainview_Item *item)
{
   Evas_Object *txt, *lines, *vbox, *box, *searchbar, *statusbar;
   Evas_Modifier_Mask ctrl, shift, alt;
   Evas *e;
   Edi_Editor *editor;

   vbox = elm_box_add(parent);
   evas_object_size_hint_weight_set(vbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(vbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(vbox);

   searchbar = elm_box_add(vbox);
   evas_object_size_hint_weight_set(searchbar, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(searchbar, EVAS_HINT_FILL, 0.0);
   elm_box_pack_end(vbox, searchbar);

   box = elm_box_add(vbox);
   elm_box_horizontal_set(box, EINA_TRUE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(vbox, box);
   evas_object_show(box);

   statusbar = elm_box_add(vbox);
   evas_object_size_hint_weight_set(statusbar, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(statusbar, EVAS_HINT_FILL, 0.0);
   elm_box_pack_end(vbox, statusbar);
   evas_object_show(statusbar);

   lines = elm_entry_add(box);
   elm_entry_scrollable_set(lines, EINA_TRUE);
   elm_scroller_policy_set(lines, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
   elm_entry_editable_set(lines, EINA_FALSE);

   elm_entry_text_style_user_push(lines, _edi_editor_font_get());
   evas_object_color_set(lines, 127, 127, 127, 255);

   evas_object_size_hint_weight_set(lines, 0.052, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(lines, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, lines);
   evas_object_show(lines);

   txt = elm_entry_add(box);
   elm_entry_editable_set(txt, EINA_TRUE);
   elm_entry_scrollable_set(txt, EINA_TRUE);
   elm_entry_line_wrap_set(txt, EINA_FALSE);
   elm_entry_text_style_user_push(txt, _edi_editor_font_get());

   editor = calloc(1, sizeof(*editor));
   editor->entry = txt;
   editor->lines = lines;
   evas_object_event_callback_add(txt, EVAS_CALLBACK_KEY_DOWN,
                                  _smart_cb_key_down, editor);
   evas_object_smart_callback_add(txt, "changed,user", _changed_cb, editor);
   evas_object_smart_callback_add(txt, "scroll", _scroll_cb, editor);
   evas_object_smart_callback_add(txt, "resize", _resize_cb, editor);
   evas_object_smart_callback_add(txt, "undo,request", _undo_cb, editor);
   evas_object_smart_callback_add(txt, "text,set,done", _text_set_done, editor);

   elm_entry_file_set(txt, item->path, ELM_TEXT_FORMAT_PLAIN_UTF8);

   elm_entry_autosave_set(txt, EDI_CONTENT_AUTOSAVE);
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(txt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(txt);
   elm_box_pack_end(box, txt);


   edi_editor_search_add(searchbar, editor);
   _edi_editor_statusbar_add(statusbar, editor, item);

   e = evas_object_evas_get(txt);
   ctrl = evas_key_modifier_mask_get(e, "Control");
   alt = evas_key_modifier_mask_get(e, "Alt");
   shift = evas_key_modifier_mask_get(e, "Shift");

   (void)!evas_object_key_grab(txt, "Prior", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(txt, "Next", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(txt, "s", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(txt, "f", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(txt, "z", ctrl, shift | alt, 1);

   evas_object_data_set(vbox, "editor", editor);

   return vbox;
}
