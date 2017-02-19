#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if HAVE_LIBCLANG
#include <clang-c/Index.h>
#include <clang-c/CXCompilationDatabase.h>
#endif

#include <Eina.h>
#include <Elementary.h>

#include "edi_language_provider.h"

#include "edi_config.h"

#include "edi_private.h"

#if HAVE_LIBCLANG

static void
_clang_commands_fallback_get(const char ***args, unsigned int *argc)
{
   const char *argstr;

   argstr = "-I/usr/include/ " EFL_CFLAGS " " CLANG_INCLUDES " -Wall -Wextra";
   *args = (const char **) eina_str_split_full(argstr, " ", 0, argc);
}

static void
_clang_commands_get(const char *path, const char ***args, unsigned int *argc)
{
   CXCompilationDatabase_Error error;
   CXCompilationDatabase database = NULL;
   CXCompileCommands commands;
   CXCompileCommand command;
   const char** arguments;
   unsigned int i, numargs, ignored = 0;

   if (edi_project_file_exists("compile_commands.json"))
     {
        database = clang_CompilationDatabase_fromDirectory(edi_project_get(), &error);
     }
   else if (edi_project_file_exists("build/compile_commands.json"))
     {
        char *build = edi_project_file_path_get("build");
        database = clang_CompilationDatabase_fromDirectory(build, &error);
        free(build);
     }

   if (database == NULL || error == CXCompilationDatabase_CanNotLoadDatabase)
     {
        INF("Could not load compile_commands.json in %s", edi_project_get());
        _clang_commands_fallback_get(args, argc);
        return;
     }

   commands = clang_CompilationDatabase_getCompileCommands(database, path);
   command = clang_CompileCommands_getCommand(commands, 0);
   numargs = clang_CompileCommand_getNumArgs(command);

   if (numargs == 0)
     {
        INF("File %s not found in compile_commands.json", path);
        _clang_commands_fallback_get(args, argc);
        return;
     }

   arguments = malloc(sizeof(char*) * (numargs + 1));
   INF("Loading clang parameters for %s", path);

   arguments[0] = CLANG_INCLUDES;
   for(i = 1; i <= numargs; i++ )
     {
        const char *argstr;
        CXString argument = clang_CompileCommand_getArg(command, i + 1);
        argstr = clang_getCString(argument);

        if (argstr && strlen(argstr) > 2 && argstr[0] == '-' &&
            (argstr[1] == 'I' || argstr[1] == 'D'))
          arguments[i - ignored] = strdup(argstr);
        else
          ignored++;

        clang_disposeString(argument);
     }

   *args = arguments;
   *argc = numargs + 1 - ignored;

   clang_CompilationDatabase_dispose(database);
}

static void
_clang_autosuggest_setup(Edi_Editor *editor)
{
   Elm_Code *code;
   const char *path;
   const char **args;
   unsigned int argc;

   code = elm_code_widget_code_get(editor->entry);
   path = elm_code_file_path_get(code->file);

   //Initialize Clang
   _clang_commands_get(path, &args, &argc);
   editor->clang_idx = clang_createIndex(0, 0);
   editor->clang_unit = clang_parseTranslationUnit(editor->clang_idx, path,
                                  args, argc, NULL, 0,
                                  clang_defaultEditingTranslationUnitOptions() | CXTranslationUnit_DetailedPreprocessingRecord);
}

static void
_clang_autosuggest_dispose(Edi_Editor *editor)
{
   clang_disposeTranslationUnit(editor->clang_unit);
   clang_disposeIndex(editor->clang_idx);
}
#endif

void
_edi_language_c_add(Edi_Editor *editor)
{
#if HAVE_LIBCLANG
   _clang_autosuggest_setup(editor);
#endif
}

void
_edi_language_c_refresh(Edi_Editor *editor)
{
#if HAVE_LIBCLANG
   _clang_autosuggest_dispose(editor);
   _clang_autosuggest_setup(editor);
#endif
}

void
_edi_language_c_del(Edi_Editor *editor)
{
#if HAVE_LIBCLANG
   _clang_autosuggest_dispose(editor);
#endif
}

const char *
_edi_language_c_mime_name(const char *mime)
{
   if (!strcasecmp(mime, "text/x-chdr"))
     return "C header";
   if (!strcasecmp(mime, "text/x-csrc"))
     return "C source";

   return NULL;
}

#if HAVE_LIBCLANG
char *
_edi_suggest_c_detail_get(Edi_Editor *editor, const char *term_str,
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
_edi_language_c_lookup(Edi_Editor *editor, unsigned int row, unsigned int col)
{
   Eina_List *list = NULL;

#if HAVE_LIBCLANG
   CXCodeCompleteResults *res;
   struct CXUnsavedFile unsaved_file;
   Elm_Code *code;
   const char *path = NULL;

   if (!editor->clang_unit)
     return list;

   code = elm_code_widget_code_get(editor->entry);
   if (code->file->file)
     path = elm_code_file_path_get(code->file);

   unsaved_file.Filename = path;
   unsaved_file.Contents = elm_code_widget_text_between_positions_get(
                                                 editor->entry, 1, 1, row, col);
   unsaved_file.Length = strlen(unsaved_file.Contents);

   res = clang_codeCompleteAt(editor->clang_unit, path, row, col,
                              &unsaved_file, 1,
                              CXCodeComplete_IncludeMacros |
                              CXCodeComplete_IncludeCodePatterns);

   clang_sortCodeCompletionResults(res->Results, res->NumResults);

   for (unsigned int i = 0; i < res->NumResults; i++)
     {
        const CXCompletionString str = res->Results[i].CompletionString;
        const char *name = NULL, *ret = NULL;
        char *param = NULL;
        Edi_Language_Suggest_Item *suggest_it;
        Eina_Strbuf *buf = NULL;

        suggest_it = calloc(1, sizeof(Edi_Language_Suggest_Item));

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
        suggest_it->detail = _edi_suggest_c_detail_get(editor, name, ret?ret:"", param?param:"");
        if (param)
          free(param);

        list = eina_list_append(list, suggest_it);
     }
   clang_disposeCodeCompleteResults(res);
#endif

   return list;
}

#if HAVE_LIBCLANG
static void
_edi_doc_init(Edi_Language_Document *doc)
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
_edi_doc_dump(Edi_Language_Document *doc, CXComment comment, Eina_Strbuf *strbuf)
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

static CXCursor
_edi_doc_cursor_get(Edi_Editor *editor, unsigned int row, unsigned int col)
{
   CXFile cxfile;
   CXSourceLocation location;
   CXCursor cursor;
   Elm_Code *code;
   const char *path;

   code = elm_code_widget_code_get(editor->entry);
   path = elm_code_file_path_get(code->file);

   cxfile = clang_getFile(editor->clang_unit, path);
   location = clang_getLocation(editor->clang_unit, cxfile, row, col);
   cursor = clang_getCursor(editor->clang_unit, location);

   return clang_getCursorReferenced(cursor);
}
#endif

static Edi_Language_Document *
_edi_language_c_lookup_doc(Edi_Editor *editor, unsigned int row, unsigned int col)
{
   Edi_Language_Document *doc = NULL;
#if HAVE_LIBCLANG
   CXCursor cursor;
   CXComment comment;

   cursor = _edi_doc_cursor_get(editor, row, col);
   comment = clang_Cursor_getParsedComment(cursor);

   if (clang_Comment_getKind(comment) == CXComment_Null)
     {
        return NULL;
     }

   doc = malloc(sizeof(Edi_Language_Document));

   _edi_doc_init(doc);
   _edi_doc_dump(doc, comment, doc->detail);
   _edi_doc_title_get(cursor, doc->title);
   _edi_doc_trim(doc->detail);
#endif
   return doc;
}

