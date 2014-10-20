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

#include "edi_private.h"

#define EDITOR_FONT "DEFAULT='font=Monospace font_size=12'"

#define Edi_Color const char *

static Edi_Color EDI_COLOR_FOREGROUND = "+ color=#ffffff";
static Edi_Color EDI_COLOR_COMMENT = "+ color=#3399ff";
static Edi_Color EDI_COLOR_STRING = "+ color=#ff3a35";
static Edi_Color EDI_COLOR_NUMBER = "+ color=#D4D42A font_weight=Bold";
static Edi_Color EDI_COLOR_BRACE = "+ color=#656565";
static Edi_Color EDI_COLOR_TYPE = "+ color=#3399ff";
static Edi_Color EDI_COLOR_CLASS = "+ color=#72AAD4 font_weight=Bold";
static Edi_Color EDI_COLOR_FUNCTION = "+ color=#72AAD4 font_weight=Bold";
static Edi_Color EDI_COLOR_PARAM = "+ color=#ffffff";
static Edi_Color EDI_COLOR_KEYWORD = "+ color=#ff9900 font_weight=Bold";
static Edi_Color EDI_COLOR_PREPROCESSOR = "+ color=#00B000";

static Edi_Color EDI_COLOR_BACKGROUND = "+ backing_color=#000000";
static Edi_Color EDI_COLOR_SEVIRITY_IGNORED = "+ backing_color=#000000";
static Edi_Color EDI_COLOR_SEVIRITY_NOTE = "+ backing_color=#ff9900";
static Edi_Color EDI_COLOR_SEVIRITY_WARNING = "+ backing_color=#ff9900";

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

#if HAVE_LIBCLANG
Evas_Textblock_Cursor *_format_cursor;
#endif

static void
_update_lines(Edi_Editor *editor);

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
_smart_cb_key_down(void *data, Evas *e EINA_UNUSED,
                   Evas_Object *obj EINA_UNUSED, void *event)
{
   Eina_Bool ctrl, alt, shift;
   Evas_Event_Key_Down *ev = event;

   ctrl = evas_key_modifier_is_set(ev->modifiers, "Control");
   alt = evas_key_modifier_is_set(ev->modifiers, "Alt");
   shift = evas_key_modifier_is_set(ev->modifiers, "Shift");

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
          }
        else if (!strcmp(ev->key, "f"))
          {
             edi_mainview_search();
          }
        else if (!strcmp(ev->key, "z"))
          {
             _undo_cb(data, obj, event);
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

   elm_scroller_region_get(editor->entry, NULL, &y, NULL, NULL);
   elm_scroller_region_get(editor->lines, NULL, NULL, NULL, &h);
   elm_scroller_region_show(editor->lines, 0, y, 10, h);
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
static void
_edi_range_color_set(Edi_Editor *editor, Edi_Range range, Edi_Color color)
{
   char *format;
   format = strdup(color);

   evas_textblock_cursor_line_set(_format_cursor, range.start.line - 1);
   evas_textblock_cursor_pos_set(_format_cursor, evas_textblock_cursor_pos_get(_format_cursor) + range.start.col - 1);
   evas_textblock_cursor_format_prepend(_format_cursor, format);

   format[0] = '-';
   evas_textblock_cursor_line_set(_format_cursor, range.end.line - 1);
   evas_textblock_cursor_pos_set(_format_cursor, evas_textblock_cursor_pos_get(_format_cursor) + range.end.col - 1);
   evas_textblock_cursor_format_append(_format_cursor, format);

   free(format);
}

static void
_clang_load_highlighting(const char *path, Edi_Editor *editor)
{
   CXToken *tokens = NULL;
   unsigned int n = 0;
   unsigned int i = 0;
   CXCursor *cursors = NULL;

     {
        CXFile cfile = clang_getFile(editor->tx_unit, path);
        int tgridw = 0, tgridh = 0;
        evas_object_textblock_size_native_get(elm_entry_textblock_get(editor->entry), &tgridw, &tgridh);

#if 0
        /* FIXME: Should be used, I don't know why tokenize doesn't work in mid
         * comment cases and etc. */
        int range_start, range_end;
        range_start = editor->offset;
        range_end = editor->offset + tgridh;
        CXSourceRange range = clang_getRange(clang_getLocation(editor->tx_unit, cfile, range_start, 1), clang_getLocation(editor->tx_unit, cfile, range_end, tgridw));
#else
        CXSourceRange range = clang_getRange(
              clang_getLocationForOffset(editor->tx_unit, cfile, 0),
              clang_getLocationForOffset(editor->tx_unit, cfile, eina_file_size_get(eina_file_open(path, EINA_FALSE))));
#endif
        clang_tokenize(editor->tx_unit, range, &tokens, &n);
        /* FIXME: We should use annotate tokens and then use a lot more
         * color classes. I don't know why it's broken ATM... :( */
        cursors = (CXCursor *) malloc(n * sizeof(CXCursor));
        clang_annotateTokens(editor->tx_unit, tokens, n, cursors);
     }

   for (i = 0 ; i < n ; i++)
     {
        Edi_Range range;
        Edi_Color color = EDI_COLOR_FOREGROUND;

        CXSourceRange tkrange = clang_getTokenExtent(editor->tx_unit, tokens[i]);
        clang_getSpellingLocation(clang_getRangeStart(tkrange), NULL,
              &range.start.line, &range.start.col, NULL);
        clang_getSpellingLocation(clang_getRangeEnd(tkrange), NULL,
              &range.end.line, &range.end.col, NULL);
        /* FIXME: Should probably do something fancier, this is only a limited
         * number of types. */
        switch (clang_getTokenKind(tokens[i]))
          {
             case CXToken_Punctuation:
                if (i < n - 1 && range.start.col == 1 &&
                    (clang_getTokenKind(tokens[i + 1]) == CXToken_Identifier && (cursors[i + 1].kind == CXCursor_MacroDefinition ||
                    cursors[i + 1].kind == CXCursor_InclusionDirective || cursors[i + 1].kind == CXCursor_PreprocessingDirective)))
                  color = EDI_COLOR_PREPROCESSOR;
                else
                  color = EDI_COLOR_BRACE;
                break;
             case CXToken_Identifier:
                if (cursors[i].kind < CXCursor_FirstRef)
                  {
                      color = EDI_COLOR_CLASS;
                      break;
                  }
                switch (cursors[i].kind)
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
                switch (cursors[i].kind)
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
                color = EDI_COLOR_NUMBER;
                break;
             case CXToken_Comment:
                color = EDI_COLOR_COMMENT;
                break;
          }

        _edi_range_color_set(editor, range, color);

#if CLANG_DEBUG
        const char *kind = NULL;
        switch (clang_getTokenKind(tokens[i])) {
           case CXToken_Punctuation: kind = "Punctuation"; break;
           case CXToken_Keyword: kind = "Keyword"; break;
           case CXToken_Identifier: kind = "Identifier"; break;
           case CXToken_Literal: kind = "Literal"; break;
           case CXToken_Comment: kind = "Comment"; break;
        }

        printf("%s ", kind);
        PrintToken(editor->tx_unit, tokens[i]);

        if (!clang_isInvalid(cursors[i].kind)) {
             printf(" ");
             PrintCursor(cursors[i]);
        }

        printf("\n");
#endif
     }

   free(cursors);
   clang_disposeTokens(editor->tx_unit, tokens, n);
}

static void
_clang_load_errors(const char *path, Edi_Editor *editor)
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

static void
_edi_clang_setup(const char *path, Edi_Editor *editor)
{
   Evas_Object *textblock;

   /* Clang */
   /* FIXME: index should probably be global. */
   const char const *clang_argv[] = {"-I/usr/lib/clang/3.1/include/", "-Wall", "-Wextra"};
   int clang_argc = sizeof(clang_argv) / sizeof(*clang_argv);

   editor->idx = clang_createIndex(0, 0);

   /* FIXME: Possibly activate more options? */
   editor->tx_unit = clang_parseTranslationUnit(editor->idx, path, clang_argv, clang_argc, NULL, 0, clang_defaultEditingTranslationUnitOptions() | CXTranslationUnit_DetailedPreprocessingRecord);

   textblock = elm_entry_textblock_get(editor->entry);
   _format_cursor = evas_object_textblock_cursor_new(textblock);
   _clang_load_errors(path, editor);
   _clang_load_highlighting(path, editor);
   evas_textblock_cursor_free(_format_cursor);
}

static void
_edi_clang_dispose(Edi_Editor *editor)
{
   clang_disposeTranslationUnit(editor->tx_unit);
   clang_disposeIndex(editor->idx);
}
#endif

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

   elm_entry_text_style_user_push(lines, EDITOR_FONT);
   evas_object_color_set(lines, 127, 127, 127, 255);

   evas_object_size_hint_weight_set(lines, 0.052, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(lines, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, lines);
   evas_object_show(lines);

   txt = elm_entry_add(box);
   elm_entry_editable_set(txt, EINA_TRUE);
   elm_entry_scrollable_set(txt, EINA_TRUE);
   elm_entry_line_wrap_set(txt, EINA_FALSE);
   elm_entry_text_style_user_push(txt, EDITOR_FONT);
   elm_entry_file_set(txt, item->path, ELM_TEXT_FORMAT_PLAIN_UTF8);
   elm_entry_autosave_set(txt, EDI_CONTENT_AUTOSAVE);
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(txt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(txt);
   elm_box_pack_end(box, txt);

   editor = calloc(1, sizeof(*editor));
   editor->entry = txt;
   editor->lines = lines;
   evas_object_event_callback_add(txt, EVAS_CALLBACK_KEY_DOWN,
                                  _smart_cb_key_down, editor);
   evas_object_smart_callback_add(txt, "changed,user", _changed_cb, editor);
   evas_object_smart_callback_add(txt, "scroll", _scroll_cb, editor);
   evas_object_smart_callback_add(txt, "undo,request", _undo_cb, editor);

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
   _update_lines(editor);

#if HAVE_LIBCLANG
   _edi_clang_setup(item->path, editor);
#endif

   return vbox;
}
