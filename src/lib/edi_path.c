#ifdef HAVE_CONFIG
# include "config.h"
#endif

#include "Edi.h"
#include "edi_path.h"

#include "edi_private.h"

EAPI Edi_Path_Options *
edi_path_options_create(const char *input)
{
   Edi_Path_Options *ret;
   const char *path, *pos1, *pos2;
   int line = 0, col = 0;

   path = input;
   ret = calloc(1, sizeof(Edi_Path_Options));

   pos1 = strstr(path, ":");
   if (pos1)
     {
        ret->path = eina_stringshare_add_length(path, strlen(path) - strlen(pos1));
        pos1++;
        pos2 = strstr(pos1, ":");
        if (pos2)
          {
             line = atoi(eina_stringshare_add_length(pos1, strlen(pos1) - strlen(pos2)));

             col = atoi(pos2 + 1);
          }
        else
          {
             line = atoi(pos1);
          }
     }
   else
     ret->path = eina_stringshare_add(path);

   ret->line = line;
   ret->character = col;

   return ret;
}

EAPI const char *
edi_path_append(const char *path, const char *file)
{
   char *concat;
   int len;
   char separator = '/';
#ifdef WIN32
   separator = '\\';
#endif

   len = strlen(path) + strlen(file) + 2;
   concat = malloc(sizeof(char) * len);
   snprintf(concat, len, "%s%c%s", path, separator, file);

   return concat;
}

