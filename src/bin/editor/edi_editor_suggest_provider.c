#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "edi_editor_suggest_provider.h"

#include "edi_config.h"

#include "edi_private.h"

#include "edi_editor_suggest_provider_c.c"

static Edi_Editor_Suggest_Provider _edi_editor_suggest_provider_registry[] =
{
   {
      "c", _edi_editor_sugggest_c_add, _edi_editor_sugget_c_del,
      _edi_editor_suggest_c_lookup, _edi_editor_suggest_c_lookup_doc
   },

   {NULL, NULL, NULL, NULL, NULL}
};

Edi_Editor_Suggest_Provider *edi_editor_suggest_provider_get(Edi_Editor *editor)
{
   Edi_Editor_Suggest_Provider *provider;
   const char *mime = editor->mimetype;
   const char *id = NULL;

   if (!mime)
     return NULL;

   if (!strcasecmp(mime, "text/x-chdr") || !strcasecmp(mime, "text/x-csrc")
       || !strcasecmp(mime, "text/x-modelica"))
     id = "c";

   if (!id)
     return NULL;

   provider = _edi_editor_suggest_provider_registry;
   while (provider != NULL && provider->id != NULL)
     {
        if (!strncmp(id, provider->id, strlen(provider->id)))
          return provider;

        provider++;
     }

   return NULL;
}

Eina_Bool
edi_editor_suggest_provider_has(Edi_Editor *editor)
{
   return !!edi_editor_suggest_provider_get(editor);
}

void
edi_editor_suggest_item_free(Edi_Editor_Suggest_Item *item)
{
   free((char *)item->summary);
   free((char *)item->detail);

   free(item);
}

void
edi_editor_suggest_doc_free(Edi_Editor_Suggest_Document *doc)
{
   if (!doc) return;

   eina_strbuf_free(doc->title);
   eina_strbuf_free(doc->detail);
   eina_strbuf_free(doc->param);
   eina_strbuf_free(doc->ret);
   eina_strbuf_free(doc->see);
}

