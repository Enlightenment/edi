#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if HAVE_LIBCLANG
#include <clang-c/Index.h>
#endif

#include <Eina.h>
#include <Elementary.h>

#include "edi_editor_suggest_provider.h"

#include "edi_config.h"

#include "edi_private.h"

#if HAVE_LIBCLANG
static void
_clang_autosuggest_setup(Edi_Editor *editor)
{
   Elm_Code *code;
   const char *path;
   char **clang_argv;
   const char *args;
   unsigned int clang_argc;

   code = elm_code_widget_code_get(editor->entry);
   path = elm_code_file_path_get(code->file);

   //Initialize Clang
   args = "-I/usr/inclue/ " EFL_CFLAGS " " CLANG_INCLUDES " -Wall -Wextra";
   clang_argv = eina_str_split_full(args, " ", 0, &clang_argc);

   editor->as_idx = clang_createIndex(0, 0);

   editor->as_unit = clang_parseTranslationUnit(editor->as_idx, path,
                                  (const char *const *)clang_argv,
                                  (int)clang_argc, NULL, 0,
                                  clang_defaultEditingTranslationUnitOptions());
}

static void
_clang_autosuggest_dispose(Edi_Editor *editor)
{
   clang_disposeTranslationUnit(editor->as_unit);
   clang_disposeIndex(editor->as_idx);
}
#endif

void
_edi_editor_sugggest_c_add(Edi_Editor *editor)
{
#if HAVE_LIBCLANG
   _clang_autosuggest_setup(editor);
#endif
}

void
_edi_editor_sugget_c_del(Edi_Editor *editor)
{
#if HAVE_LIBCLANG
   _clang_autosuggest_dispose(editor);
#endif
}

#if HAVE_LIBCLANG
char *
_edi_editor_suggest_c_detail_get(Edi_Editor *editor, const char *term_str,
                                 const char *ret_str, const char *param_str)
{
   char *format, *display;
   const char *font;
   int font_size, displen;
   unsigned int row, col;
   Evas_Coord w;

   elm_code_widget_font_get(editor->entry, &font, &font_size);
   elm_code_widget_cursor_position_get(editor->entry, &row, &col);
   elm_code_widget_geometry_for_position_get(editor->entry, row, col,
                                             NULL, NULL, &w, NULL);

   format = "<left_margin=%d><align=left><font='%s'><font_size=%d>%s<br><b>%s</b><br>%s</font_size></font></align></left_margin>";
   displen = strlen(ret_str) + strlen(param_str) + strlen(term_str)
             + strlen(format) + strlen(font);
   display = malloc(sizeof(char) * displen);
   snprintf(display, displen, format, w, font, font_size, ret_str, term_str, param_str);

   return display;
}
#endif

Eina_List *
_edi_editor_suggest_c_lookup(Edi_Editor *editor, unsigned int row, unsigned int col)
{
   Eina_List *list = NULL;

#if HAVE_LIBCLANG
   CXCodeCompleteResults *res;
   struct CXUnsavedFile unsaved_file;
   Elm_Code *code;
   const char *path = NULL;

   if (!editor->as_unit)
     return list;

   code = elm_code_widget_code_get(editor->entry);
   if (code->file->file)
     path = elm_code_file_path_get(code->file);

   unsaved_file.Filename = path;
   unsaved_file.Contents = elm_code_widget_text_between_positions_get(
                                                 editor->entry, 1, 1, row, col);
   unsaved_file.Length = strlen(unsaved_file.Contents);

   res = clang_codeCompleteAt(editor->as_unit, path, row, col,
                              &unsaved_file, 1,
                              CXCodeComplete_IncludeMacros |
                              CXCodeComplete_IncludeCodePatterns);

   clang_sortCodeCompletionResults(res->Results, res->NumResults);

   for (unsigned int i = 0; i < res->NumResults; i++)
     {
        const CXCompletionString str = res->Results[i].CompletionString;
        const char *name = NULL, *ret = NULL;
        char *param = NULL;
        Edi_Editor_Suggest_Item *suggest_it;
        Eina_Strbuf *buf = NULL;

        suggest_it = calloc(1, sizeof(Edi_Editor_Suggest_Item));

        for (unsigned int j = 0; j < clang_getNumCompletionChunks(str); j++)
          {
             enum CXCompletionChunkKind ch_kind;
             const CXString str_out = clang_getCompletionChunkText(str, j);

             ch_kind = clang_getCompletionChunkKind(str, j);

             switch (ch_kind)
               {
                case CXCompletionChunk_ResultType:
                   ret = clang_getCString(str_out);
                   break;
                case CXCompletionChunk_TypedText:
                case CXCompletionChunk_Text:
                   name = clang_getCString(str_out);
                   break;
                case CXCompletionChunk_LeftParen:
                  // todo buf == eina_strbuf_new();
                case CXCompletionChunk_Placeholder:
                case CXCompletionChunk_Comma:
                case CXCompletionChunk_CurrentParameter:
                   if (!buf)
                     buf = eina_strbuf_new();
                   eina_strbuf_append(buf, clang_getCString(str_out));
                   break;
                case CXCompletionChunk_RightParen:
                   eina_strbuf_append(buf, clang_getCString(str_out));
                   param = eina_strbuf_string_steal(buf);
                   eina_strbuf_free(buf);
                   buf = NULL;
                   break;
                default:
                   break;
               }
          }

        if (name)
          suggest_it->summary = strdup(name);
        suggest_it->detail = _edi_editor_suggest_c_detail_get(editor, name, ret?ret:"", param?param:"");
        if (param)
          free(param);

        list = eina_list_append(list, suggest_it);
     }
   clang_disposeCodeCompleteResults(res);
#endif

   return list;
}

