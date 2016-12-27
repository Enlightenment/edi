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
typedef struct
{
   enum CXCursorKind kind;
   char             *ret;
   char             *name;
   char             *param;
   Eina_Bool         is_param_cand;
} _Clang_Suggest_Item;

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

static const char *
_suggest_item_return_get(Edi_Editor_Suggest_Item *item)
{
#if HAVE_LIBCLANG
   if (((_Clang_Suggest_Item *)item)->ret)
     return ((_Clang_Suggest_Item *)item)->ret;
#endif

   return "";
}

static const char *
_suggest_item_parameter_get(Edi_Editor_Suggest_Item *item)
{
#if HAVE_LIBCLANG
   if (((_Clang_Suggest_Item *)item)->param)
     return ((_Clang_Suggest_Item *)item)->param;
#endif

   return "";
}

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
                                                 editor->entry, 1, 1, col, row);
   unsaved_file.Length = strlen(unsaved_file.Contents);

   res = clang_codeCompleteAt(editor->as_unit, path, row, col,
                              &unsaved_file, 1,
                              CXCodeComplete_IncludeMacros |
                              CXCodeComplete_IncludeCodePatterns);

   clang_sortCodeCompletionResults(res->Results, res->NumResults);

   for (unsigned int i = 0; i < res->NumResults; i++)
     {
        const CXCompletionString str = res->Results[i].CompletionString;
        _Clang_Suggest_Item *suggest_it;
        Eina_Strbuf *buf = NULL;

        suggest_it = calloc(1, sizeof(_Clang_Suggest_Item));
        suggest_it->kind = res->Results[i].CursorKind;
        if (suggest_it->kind == CXCursor_OverloadCandidate)
          suggest_it->is_param_cand = EINA_TRUE;

        for (unsigned int j = 0; j < clang_getNumCompletionChunks(str); j++)
          {
             enum CXCompletionChunkKind ch_kind;
             const CXString str_out = clang_getCompletionChunkText(str, j);

             ch_kind = clang_getCompletionChunkKind(str, j);

             switch (ch_kind)
               {
                case CXCompletionChunk_ResultType:
                   suggest_it->ret = strdup(clang_getCString(str_out));
                   break;
                case CXCompletionChunk_TypedText:
                case CXCompletionChunk_Text:
                   suggest_it->name = strdup(clang_getCString(str_out));
                   break;
                case CXCompletionChunk_LeftParen:
                case CXCompletionChunk_Placeholder:
                case CXCompletionChunk_Comma:
                case CXCompletionChunk_CurrentParameter:
                   if (!buf)
                     buf = eina_strbuf_new();
                   eina_strbuf_append(buf, clang_getCString(str_out));
                   break;
                case CXCompletionChunk_RightParen:
                   eina_strbuf_append(buf, clang_getCString(str_out));
                   suggest_it->param = eina_strbuf_string_steal(buf);
                   eina_strbuf_free(buf);
                   buf = NULL;
                   break;
                default:
                   break;
               }
          }
        list = eina_list_append(list, suggest_it);
     }
   clang_disposeCodeCompleteResults(res);
#endif

   return list;
}

const char *
_edi_editor_suggest_c_summary_get(Edi_Editor *editor EINA_UNUSED, Edi_Editor_Suggest_Item *item)
{
#if HAVE_LIBCLANG
   return ((_Clang_Suggest_Item *)item)->name;
#else
   return "";
#endif
}

static void
_edi_editor_suggest_c_item_free(Edi_Editor_Suggest_Item *item)
{
#if HAVE_LIBCLANG
   _Clang_Suggest_Item *clang_item = (_Clang_Suggest_Item *)item;

   if (clang_item->ret) free(clang_item->ret);
   if (clang_item->name) free(clang_item->name);
   if (clang_item->param) free(clang_item->param);
   free(clang_item);
#endif
}

char *
_edi_editor_suggest_c_detail_get(Edi_Editor *editor, Edi_Editor_Suggest_Item *item)
{
   char *format, *display;
   const char *font, *term_str, *ret_str, *param_str;
   int font_size, displen;

   elm_code_widget_font_get(editor->entry, &font, &font_size);

   term_str = _edi_editor_suggest_c_summary_get(editor, item);
   ret_str = _suggest_item_return_get(item);
   param_str = _suggest_item_parameter_get(item);

   format = "<align=left><font=\"%s\"><font_size=%d>%s<br><b>%s</b><br>  %s</font_size></font></align>";
   displen = strlen(ret_str) + strlen(param_str) + strlen(term_str)
             + strlen(format) + strlen(font);
   display = malloc(sizeof(char) * displen);
   snprintf(display, displen, format, font, font_size, ret_str, term_str, param_str);

   return display;
}

