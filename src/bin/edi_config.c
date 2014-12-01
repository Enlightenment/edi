#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eet.h>
#include <Eina.h>
#include <Ecore_File.h>
#include <Efreet.h>
#include <Elementary.h>

#include "edi_config.h"

#include "edi_private.h"

# define EDI_CONFIG_LIMIT(v, min, max) \
   if (v > max) v = max; else if (v < min) v = min;

# define EDI_CONFIG_DD_NEW(str, typ) \
   _edi_config_descriptor_new(str, sizeof(typ))

# define EDI_CONFIG_DD_FREE(eed) \
   if (eed) { eet_data_descriptor_free(eed); (eed) = NULL; }

# define EDI_CONFIG_VAL(edd, type, member, dtype) \
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, type, #member, member, dtype)

# define EDI_CONFIG_SUB(edd, type, member, eddtype) \
   EET_DATA_DESCRIPTOR_ADD_SUB(edd, type, #member, member, eddtype)

# define EDI_CONFIG_LIST(edd, type, member, eddtype) \
   EET_DATA_DESCRIPTOR_ADD_LIST(edd, type, #member, member, eddtype)

# define EDI_CONFIG_HASH(edd, type, member, eddtype) \
   EET_DATA_DESCRIPTOR_ADD_HASH(edd, type, #member, member, eddtype)

#  define EDI_CONFIG_FILE_EPOCH 0x0001
#  define EDI_CONFIG_FILE_GENERATION 0x000a
#  define EDI_CONFIG_FILE_VERSION \
   ((EDI_CONFIG_FILE_EPOCH << 16) | EDI_CONFIG_FILE_GENERATION)

typedef Eet_Data_Descriptor Edi_Config_DD;

/* local variables */
static Edi_Config_DD *_edi_cfg_edd = NULL;
static Edi_Config_DD *_edi_cfg_proj_edd = NULL;

/* external variables */
Edi_Config *_edi_cfg = NULL;

const char *
_edi_config_dir_get(void)
{
   static char dir[PATH_MAX];

   if (!dir[0])
     snprintf(dir, sizeof(dir), "%s/edi", efreet_config_home_get());

   return dir;
}

/* local functions */
static Edi_Config_DD *
_edi_config_descriptor_new(const char *name, int size)
{
   Eet_Data_Descriptor_Class eddc;

   if (!eet_eina_stream_data_descriptor_class_set(&eddc, sizeof(eddc), 
                                                  name, size))
     return NULL;

   return (Edi_Config_DD *)eet_data_descriptor_stream_new(&eddc);
}

static void
_edi_config_cb_free(void)
{
   Edi_Config_Project *proj;

   EINA_LIST_FREE(_edi_cfg->projects, proj)
     {
        if (proj->name) eina_stringshare_del(proj->name);
        if (proj->path) eina_stringshare_del(proj->path);
        free(proj);
     }

   free(_edi_cfg);
   _edi_cfg = NULL;
}

static void *
_edi_config_domain_load(const char *domain, Edi_Config_DD *edd)
{
   Eet_File *ef;
   char buff[PATH_MAX];

   if (!domain) return NULL;
   snprintf(buff, sizeof(buff),
            "%s/%s.cfg", _edi_config_dir_get(), domain);
   ef = eet_open(buff, EET_FILE_MODE_READ);
   if (ef)
     {
        void *data;

        data = eet_data_read(ef, edd, "config");
        eet_close(ef);
        if (data) return data;
     }
   return NULL;
}

static Eina_Bool 
_edi_config_domain_save(const char *domain, Edi_Config_DD *edd, const void *data)
{
   Eet_File *ef;
   char buff[PATH_MAX];
   const char *configdir;

   if (!domain) return 0;
   configdir = _edi_config_dir_get();
   snprintf(buff, sizeof(buff), "%s/", configdir);
   if (!ecore_file_exists(buff)) ecore_file_mkpath(buff);
   snprintf(buff, sizeof(buff), "%s/%s.tmp", configdir, domain);
   ef = eet_open(buff, EET_FILE_MODE_WRITE);
   if (ef)
     {
        char buff2[PATH_MAX];

        snprintf(buff2, sizeof(buff2), "%s/%s.cfg", configdir, domain);
        if (!eet_data_write(ef, edd, "config", data, 1))
          {
             eet_close(ef);
             return EINA_FALSE;
          }
        if (eet_close(ef) > 0) return EINA_FALSE;
        if (!ecore_file_mv(buff, buff2)) return EINA_FALSE;
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

/* external functions */
Eina_Bool 
_edi_config_init(void)
{
   elm_need_efreet();
   if (!efreet_init()) return EINA_FALSE;

   _edi_cfg_proj_edd = EDI_CONFIG_DD_NEW("Config_Project", Edi_Config_Project);
   #undef T
   #undef D
   #define T Edi_Config_Project
   #define D _edi_cfg_proj_edd
   EDI_CONFIG_VAL(D, T, name, EET_T_STRING);
   EDI_CONFIG_VAL(D, T, path, EET_T_STRING);

   _edi_cfg_edd = EDI_CONFIG_DD_NEW("Config", Edi_Config);
   #undef T
   #undef D
   #define T Edi_Config
   #define D _edi_cfg_edd
   EDI_CONFIG_VAL(D, T, version, EET_T_INT);
   EDI_CONFIG_VAL(D, T, font.size, EET_T_INT);
   EDI_CONFIG_VAL(D, T, gui.translucent, EET_T_UCHAR);
   EDI_CONFIG_LIST(D, T, projects, _edi_cfg_proj_edd);

   _edi_config_load();

   return EINA_TRUE;
}

Eina_Bool 
_edi_config_shutdown(void)
{
   _edi_config_cb_free();

   EDI_CONFIG_DD_FREE(_edi_cfg_proj_edd);
   EDI_CONFIG_DD_FREE(_edi_cfg_edd);

   efreet_shutdown();

   return EINA_TRUE;
}

void 
_edi_config_load(void)
{
   Eina_Bool save = EINA_FALSE;

   _edi_cfg = _edi_config_domain_load(PACKAGE_NAME, _edi_cfg_edd);
   if (_edi_cfg)
     {
        Eina_Bool reload = EINA_FALSE;

        if ((_edi_cfg->version >> 16) < EDI_CONFIG_FILE_EPOCH)
          {
             /* config too old */
             reload = EINA_TRUE;
          }
        else if (_edi_cfg->version > EDI_CONFIG_FILE_VERSION)
          {
             /* config too new, WTF ? */
             reload = EINA_TRUE;
          }

        /* if too old or too new, clear it so we can create new */
        if (reload) _edi_config_cb_free();
     }

   if (!_edi_cfg) 
     {
        _edi_cfg = malloc(sizeof(Edi_Config));
        save = EINA_TRUE;
     }

   /* define some convenient macros */
#define IFCFG(v) if ((_edi_cfg->version & 0xffff) < (v)) {
#define IFCFGELSE } else {
#define IFCFGEND }

   /* setup defaults */
   IFCFG(0x000a);

   _edi_cfg->font.size = 12;
   _edi_cfg->gui.translucent = EINA_TRUE;
   _edi_cfg->projects = NULL;
   IFCFGEND;

   /* limit config values so they are sane */
   EDI_CONFIG_LIMIT(_edi_cfg->font.size, 8, 96);

   _edi_cfg->version = EDI_CONFIG_FILE_VERSION;

   if (save) _edi_config_save();
}

void 
_edi_config_save(void)
{
   if (_edi_config_domain_save(PACKAGE_NAME, _edi_cfg_edd, _edi_cfg))
     {
        /* perform any UI updates required */
     }
}

void
_edi_config_project_add(const char *path)
{
   Edi_Config_Project *project;
   Eina_List *list, *next;

   EINA_LIST_FOREACH_SAFE(_edi_cfg->projects, list, next, project)
     {
        if (!strncmp(project->path, path, strlen(project->path)))
          _edi_cfg->projects = eina_list_remove_list(_edi_cfg->projects, list);
     }

   project = malloc(sizeof(*project));
   project->path = eina_stringshare_add(path);
   project->name = eina_stringshare_add(basename((char*) path));
   _edi_cfg->projects = eina_list_prepend(_edi_cfg->projects, project);
   _edi_config_save();
}
