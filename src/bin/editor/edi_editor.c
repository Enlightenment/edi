#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <libgen.h>

#include <Eina.h>
#include <Elementary.h>
#include <Elm_Code.h>

#include "edi_editor.h"

#define CLANG_DEBUG 0
#if CLANG_DEBUG
#include "clang_debug.h"
#endif

#include "mainview/edi_mainview.h"
#include "edi_config.h"

#include "edi_private.h"

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
_reset_highlight(Edi_Editor *editor);

void
edi_editor_save(Edi_Editor *editor)
{
   if (!editor->modified)
     return;

   editor->save_time = time(NULL);
   edi_mainview_save();
   _reset_highlight(editor);

   editor->modified = EINA_FALSE;

   ecore_timer_del(editor->save_timer);
   editor->save_timer = NULL;
}

static Eina_Bool
_edi_editor_autosave_cb(void *data)
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)data;

   edi_editor_save(editor);
   return ECORE_CALLBACK_CANCEL;
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
/*
TODO move this code into elm_code for undo/redo
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
*/
   editor->modified = EINA_TRUE;

   if (editor->save_timer)
     ecore_timer_reset(editor->save_timer);
   else if (_edi_cfg->autosave)
     editor->save_timer = ecore_timer_add(EDI_CONTENT_SAVE_TIMEOUT, _edi_editor_autosave_cb, editor);
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

   if (!item)
     return;

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
             edi_editor_save(editor);
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

static void
_edit_cursor_moved(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Elm_Code_Widget *widget;
   char buf[30];
   unsigned int line;
   unsigned int col;

   widget = (Elm_Code_Widget *)obj;
   eo_do(widget,
         elm_code_widget_cursor_position_get(&col, &line));

   snprintf(buf, sizeof(buf), "Line:%d, Column:%d", line, col);
   elm_object_text_set((Evas_Object *)data, buf);
}

static void
_edi_editor_statusbar_add(Evas_Object *panel, Edi_Editor *editor, Edi_Mainview_Item *item)
{
   Evas_Object *position, *mime, *lines;
   Elm_Code *code;

   elm_box_horizontal_set(panel, EINA_TRUE);

   mime = elm_label_add(panel);
   if (item->mimetype)
     elm_object_text_set(mime, item->mimetype);
   else
     elm_object_text_set(mime, item->editortype);
   evas_object_size_hint_align_set(mime, 0.0, 0.5);
   evas_object_size_hint_weight_set(mime, 0.1, 0.0);
   elm_box_pack_end(panel, mime);
   evas_object_show(mime);
   elm_object_disabled_set(mime, EINA_TRUE);

   lines = elm_label_add(panel);
   eo_do(editor->entry,
         code = elm_code_widget_code_get());
   if (elm_code_file_line_ending_get(code->file) == ELM_CODE_FILE_LINE_ENDING_WINDOWS)
     elm_object_text_set(lines, "WIN");
   else
     elm_object_text_set(lines, "UNIX");
   evas_object_size_hint_align_set(lines, 0.0, 0.5);
   evas_object_size_hint_weight_set(lines, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(panel, lines);
   evas_object_show(lines);
   elm_object_disabled_set(lines, EINA_TRUE);

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
_edi_range_color_set(Edi_Editor *editor, Edi_Range range, Elm_Code_Token_Type type)
{
   Elm_Code *code;
   Elm_Code_Line *line, *extra_line;
   unsigned int number;

   eo_do(editor->entry,
         code = elm_code_widget_code_get());
   line = elm_code_file_line_get(code->file, range.start.line);

   ecore_thread_main_loop_begin();

   elm_code_line_token_add(line, range.start.col, range.end.col - 1,
                           range.end.line - range.start.line + 1, type);

   eo_do(editor->entry,
         elm_code_widget_line_refresh(line));
   for (number = line->number + 1; number <= range.end.line; number++)
     {
        extra_line = elm_code_file_line_get(code->file, number);
        eo_do(editor->entry,
              elm_code_widget_line_refresh(extra_line));
     }

   ecore_thread_main_loop_end();
}

static void
_edi_line_status_set(Edi_Editor *editor, unsigned int number, Elm_Code_Status_Type status,
                     const char *text)
{
   Elm_Code *code;
   Elm_Code_Line *line;

   eo_do(editor->entry,
         code = elm_code_widget_code_get());
   line = elm_code_file_line_get(code->file, number);

   ecore_thread_main_loop_begin();

   elm_code_line_status_set(line, status);
   if (text)
     line->status_text = strdup(text);

   eo_do(editor->entry,
         elm_code_widget_line_refresh(line));

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
_clang_show_highlighting(Edi_Editor *editor)
{
   unsigned int i = 0;

   for (i = 0 ; i < editor->token_count ; i++)
     {
        Edi_Range range;
        Elm_Code_Token_Type type = ELM_CODE_TOKEN_TYPE_DEFAULT;

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
                  type = ELM_CODE_TOKEN_TYPE_PREPROCESSOR;
                else
                  type = ELM_CODE_TOKEN_TYPE_BRACE;
                break;
             case CXToken_Identifier:
                if (editor->cursors[i].kind < CXCursor_FirstRef)
                  {
                      type = ELM_CODE_TOKEN_TYPE_CLASS;
                      break;
                  }
                switch (editor->cursors[i].kind)
                  {
                   case CXCursor_DeclRefExpr:
                      /* Handle different ref kinds */
                      type = ELM_CODE_TOKEN_TYPE_FUNCTION;
                      break;
                   case CXCursor_MacroDefinition:
                   case CXCursor_InclusionDirective:
                   case CXCursor_PreprocessingDirective:
                      type = ELM_CODE_TOKEN_TYPE_PREPROCESSOR;
                      break;
                   case CXCursor_TypeRef:
                      type = ELM_CODE_TOKEN_TYPE_TYPE;
                      break;
                   case CXCursor_MacroExpansion:
                      type = ELM_CODE_TOKEN_TYPE_PREPROCESSOR;//_MACRO_EXPANSION;
                      break;
                   default:
                      break;
                  }
                break;
             case CXToken_Keyword:
                switch (editor->cursors[i].kind)
                  {
                   case CXCursor_PreprocessingDirective:
                      type = ELM_CODE_TOKEN_TYPE_PREPROCESSOR;
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
//                      type = ELM_CODE_TOKEN_TYPE_KEYWORD_STMT;
                      break;
                   default:
                      type = ELM_CODE_TOKEN_TYPE_KEYWORD;
                      break;
                  }
                break;
             case CXToken_Literal:
                if (editor->cursors[i].kind == CXCursor_StringLiteral || editor->cursors[i].kind == CXCursor_CharacterLiteral)
                  type = ELM_CODE_TOKEN_TYPE_STRING;
                else
                  type = ELM_CODE_TOKEN_TYPE_NUMBER;
                break;
             case CXToken_Comment:
                type = ELM_CODE_TOKEN_TYPE_COMMENT;
                break;
          }

        _edi_range_color_set(editor, range, type);

# if CLANG_DEBUG
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
# endif
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
        unsigned int line;

        // the parameter after line would be a caret position but we're just highlighting for now
        clang_getSpellingLocation(clang_getDiagnosticLocation(diag), NULL, &line, NULL, NULL);

        /* FIXME: Also handle ranges and fix suggestions. */
        Elm_Code_Status_Type status = ELM_CODE_STATUS_TYPE_DEFAULT;

        switch (clang_getDiagnosticSeverity(diag))
          {
           case CXDiagnostic_Ignored:
              status = ELM_CODE_STATUS_TYPE_IGNORED;
              break;
           case CXDiagnostic_Note:
              status = ELM_CODE_STATUS_TYPE_NOTE;
              break;
           case CXDiagnostic_Warning:
              status = ELM_CODE_STATUS_TYPE_WARNING;
              break;
           case CXDiagnostic_Error:
              status = ELM_CODE_STATUS_TYPE_ERROR;
              break;
           case CXDiagnostic_Fatal:
              status = ELM_CODE_STATUS_TYPE_FATAL;
              break;
          }
        CXString str = clang_getDiagnosticSpelling(diag);
        _edi_line_status_set(editor, line, status, clang_getCString(str));
        clang_disposeString(str);

        clang_disposeDiagnostic(diag);
     }
}

static void *
_edi_clang_setup(void *data)
{
   Edi_Editor *editor;
   Elm_Code *code;
   const char *path;

   editor = (Edi_Editor *)data;
   eo_do(editor->entry,
         code = elm_code_widget_code_get());
   path = elm_code_file_path_get(code->file);

   /* Clang */
   /* FIXME: index should probably be global. */
   const char *clang_argv[] = {"-I/usr/lib/clang/3.1/include/", "-Wall", "-Wextra"};
   int clang_argc = sizeof(clang_argv) / sizeof(*clang_argv);

   editor->idx = clang_createIndex(0, 0);

   /* FIXME: Possibly activate more options? */
   editor->tx_unit = clang_parseTranslationUnit(editor->idx, path, clang_argv, clang_argc, NULL, 0, clang_defaultEditingTranslationUnitOptions() | CXTranslationUnit_DetailedPreprocessingRecord);

   _clang_load_errors(path, editor);
   _clang_load_highlighting(path, editor);
   _clang_show_highlighting(editor);

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
_reset_highlight(Edi_Editor *editor)
{
   if (!editor->show_highlight)
     return;

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
_unfocused_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)data;

   if (_edi_cfg->autosave)
     edi_editor_save(editor);
}

static void
_gutter_clicked_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                   void *event_info)
{
   Elm_Code_Line *line;

   line = (Elm_Code_Line *)event_info;

   if (line->status_text)
     printf("CLANG %s\n", line->status_text);
}

static void
_edi_editor_parse_file_cb(Elm_Code_File *file EINA_UNUSED, void *data)
{
   Edi_Editor *editor;

   editor = (Edi_Editor *)data;
   _reset_highlight(editor);
}

static Eina_Bool
_edi_editor_config_changed(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   Elm_Code_Widget *widget;

   widget = (Elm_Code_Widget *) data;
   eo_do(widget,
         elm_code_widget_font_size_set(_edi_cfg->font.size),
         elm_code_widget_show_whitespace_set(_edi_cfg->gui.show_whitespace));

   return ECORE_CALLBACK_RENEW;
}

Evas_Object *
edi_editor_add(Evas_Object *parent, Edi_Mainview_Item *item)
{
   Evas_Object *vbox, *box, *searchbar, *statusbar;
   Evas_Modifier_Mask ctrl, shift, alt;
   Evas *e;

   Elm_Code *code;
   Elm_Code_Widget *widget;
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

   code = elm_code_create();
   widget = eo_add(ELM_CODE_WIDGET_CLASS, vbox,
         elm_code_widget_code_set(code));
   eo_do(widget,
         elm_code_widget_font_size_set(_edi_cfg->font.size),
         elm_code_widget_editable_set(EINA_TRUE),
         elm_code_widget_line_numbers_set(EINA_TRUE),
         elm_code_widget_line_width_marker_set(80),
         elm_code_widget_show_whitespace_set(_edi_cfg->gui.show_whitespace));

   editor = calloc(1, sizeof(*editor));
   editor->entry = widget;
   editor->show_highlight = !strcmp(item->editortype, "code");
   evas_object_event_callback_add(widget, EVAS_CALLBACK_KEY_DOWN,
                                  _smart_cb_key_down, editor);
   evas_object_smart_callback_add(widget, "changed,user", _changed_cb, editor);
   evas_object_smart_callback_add(widget, "line,gutter,clicked", _gutter_clicked_cb, editor);
/*
   evas_object_smart_callback_add(txt, "undo,request", _undo_cb, editor);
*/
   evas_object_smart_callback_add(widget, "unfocused", _unfocused_cb, editor);

   elm_code_parser_add(code, NULL, _edi_editor_parse_file_cb, editor);
   elm_code_file_open(code, item->path);

   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);
   elm_box_pack_end(box, widget);

   edi_editor_search_add(searchbar, editor);
   _edi_editor_statusbar_add(statusbar, editor, item);

   e = evas_object_evas_get(widget);
   ctrl = evas_key_modifier_mask_get(e, "Control");
   alt = evas_key_modifier_mask_get(e, "Alt");
   shift = evas_key_modifier_mask_get(e, "Shift");

   (void)!evas_object_key_grab(widget, "Prior", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(widget, "Next", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(widget, "s", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(widget, "f", ctrl, shift | alt, 1);
   (void)!evas_object_key_grab(widget, "z", ctrl, shift | alt, 1);

   evas_object_data_set(vbox, "editor", editor);
   ecore_event_handler_add(EDI_EVENT_CONFIG_CHANGED, _edi_editor_config_changed, widget);

   return vbox;
}
