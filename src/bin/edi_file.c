#include "Edi.h"
#include "edi_file.h"
#include "edi_private.h"

Eina_Bool
edi_file_path_hidden(const char *path)
{
   Edi_Build_Provider *provider;

   provider = edi_build_provider_for_project_get();
   if (provider && provider->file_hidden_is(path))
     return EINA_TRUE;

   if (ecore_file_file_get(path)[0] == '.')
     return EINA_TRUE;

   return EINA_FALSE;
}

void
edi_file_text_replace(const char *path, const char *search, const char *replace)
{
   char *map, *found;
   FILE *tempfile;
   Eina_File *f;
   char tempfilepath[PATH_MAX];
   unsigned long long len, idx;
   unsigned int slen;

   f = eina_file_open(path, EINA_FALSE);
   if (!f) return;

   map = eina_file_map_all(f, EINA_FILE_POPULATE);
   if (!map)
     {
        eina_file_close(f);
        return;
     }

   len = eina_file_size_get(f);
   if (!len)
     {
        eina_file_close(f);
        return;
     }

   snprintf(tempfilepath, sizeof(tempfilepath), "/tmp/%s.tmp", ecore_file_file_get(path));

   tempfile = fopen(tempfilepath, "wb");
   if (!tempfile) goto done;

   idx = 0;

   found = strstr(&map[idx], search);
   if (!found)
     {
        fclose(tempfile);
        unlink(tempfilepath);
        goto done;
     }

   slen = strlen(search);

   while (idx < len)
     {
         if (!found || &map[idx] < found)
           {
              fprintf(tempfile, "%c", map[idx]);
              idx++;
           }
         else
           {
              fprintf(tempfile, "%s", replace);
              idx += slen;
              found = strstr(&map[idx], search);
           }
     }

   fclose(tempfile);
   ecore_file_mv(tempfilepath, path);
done:
   eina_file_map_free(f, map);
   eina_file_close(f);
}

static void
_edi_file_text_replace_all(const char *directory, const char *search, const char *replace)
{
   char *path, *file;
   Eina_List *files;

   files = ecore_file_ls(directory);

   EINA_LIST_FREE(files, file)
     {
        path = edi_path_append(directory, file);
        if (!edi_file_path_hidden(path))
          {
             if (ecore_file_is_dir(path))
               {
                  _edi_file_text_replace_all(path, search, replace);
               }
             else
               {
                  edi_file_text_replace(path, search, replace);
               }
          }
        free(path);
        free(file);
     }

   if (files)
     eina_list_free(files);
}

void
edi_file_text_replace_all(const char *search, const char *replace)
{
   _edi_file_text_replace_all(edi_project_get(), search, replace);
}
