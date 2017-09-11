#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>

#include "edi_language_provider.h"

#include "edi_config.h"

#include "edi_private.h"

void
_edi_language_rust_add(Edi_Editor *editor EINA_UNUSED)
{
}

void
_edi_language_rust_refresh(Edi_Editor *editor EINA_UNUSED)
{
}

void
_edi_language_rust_del(Edi_Editor *editor EINA_UNUSED)
{
}

const char *
_edi_language_rust_mime_name(const char *mime)
{
   if (!strcasecmp(mime, "text/rust"))
     return _("Rust source");

   return NULL;
}

const char *
_edi_language_rust_snippet_get(const char *key)
{
   if (!strcmp(key, "ret"))
     return "return";
   if (!strcmp(key, "if"))
     return
"if ()\n" \
"  {\n" \
"  }";
   if (!strcmp(key, "ifel"))
     return
"if ()\n" \
"  {\n" \
"  }\n" \
"else\n" \
"  {\n" \
"  }";

   return NULL;
}

