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

#define EDI_CONFIG_NAME PACKAGE_NAME
#define EDI_PROJECT_CONFIG_NAME "project"

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

#  define EDI_CONFIG_FILE_EPOCH 0x0003
#  define EDI_CONFIG_FILE_GENERATION 0x000c
#  define EDI_CONFIG_FILE_VERSION \
   ((EDI_CONFIG_FILE_EPOCH << 16) | EDI_CONFIG_FILE_GENERATION)

#  define EDI_PROJECT_CONFIG_FILE_EPOCH 0x0002
#  define EDI_PROJECT_CONFIG_FILE_GENERATION 0x0004
#  define EDI_PROJECT_CONFIG_FILE_VERSION \
   ((EDI_PROJECT_CONFIG_FILE_EPOCH << 16) | EDI_PROJECT_CONFIG_FILE_GENERATION)

typedef Eet_Data_Descriptor Edi_Config_DD;
typedef Eet_Data_Descriptor Edi_Project_Config_DD;

/* local variables */
static Edi_Config_DD *_edi_cfg_edd = NULL;
static Edi_Config_DD *_edi_cfg_proj_edd = NULL;
static Edi_Config_DD *_edi_cfg_mime_edd = NULL;

static Edi_Project_Config_DD *_edi_proj_cfg_edd = NULL;
static Edi_Project_Config_DD *_edi_proj_cfg_tab_edd = NULL;
static Edi_Project_Config_DD *_edi_proj_cfg_panel_edd = NULL;

/* external variables */
Edi_Config *_edi_config = NULL;
Edi_Project_Config *_edi_project_config = NULL;
int EDI_EVENT_CONFIG_CHANGED;

const char *
_edi_config_dir_get(void)
{
   static char dir[PATH_MAX];

   if (!dir[0])
     snprintf(dir, sizeof(dir), "%s/edi", efreet_config_home_get());

   return dir;
}

const char *
_edi_project_config_dir_get(void)
{
   static char dir[PATH_MAX];

   if (!dir[0] && edi_project_get())
     snprintf(dir, sizeof(dir), "%s/edi/%s", efreet_config_home_get(), edi_project_name_get());

   return dir;
}

const char *
_edi_project_config_debug_command_get(void)
{
   return _edi_project_config->debug_command;
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
   Edi_Config_Mime_Association *mime;

   EINA_LIST_FREE(_edi_config->projects, proj)
     {
        if (proj->name) eina_stringshare_del(proj->name);
        if (proj->path) eina_stringshare_del(proj->path);
        free(proj);
     }

   EINA_LIST_FREE(_edi_config->mime_assocs, mime)
     {
        if (mime->id) eina_stringshare_del(mime->id);
        if (mime->mime) eina_stringshare_del(mime->mime);
        free(mime);
     }

   free(_edi_config);
   _edi_config = NULL;
}

static void
_edi_project_config_cb_free(void)
{
   Edi_Project_Config_Panel *panel;
   Edi_Project_Config_Tab *tab;

   if (!_edi_project_config)
     return;

   EINA_LIST_FREE(_edi_project_config->panels, panel)
     {
        EINA_LIST_FREE(panel->tabs, tab)
          {
             if (tab->path) eina_stringshare_del(tab->path);
             if (tab->type) eina_stringshare_del(tab->type);
             free(tab);
          }
     }

   EINA_LIST_FREE(_edi_project_config->windows, tab)
     {
        if (tab->path) eina_stringshare_del(tab->path);
        if (tab->type) eina_stringshare_del(tab->type);
        free(tab);
     }

   free(_edi_project_config);
   _edi_project_config = NULL;
}

static void *
_edi_config_domain_load(const char *dir, const char *domain, Eet_Data_Descriptor *edd)
{
   Eet_File *ef;
   char buff[PATH_MAX];

   if (!domain) return NULL;
   snprintf(buff, sizeof(buff),
            "%s/%s.cfg", dir, domain);
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
_edi_config_domain_save(const char *dir, const char *domain, Eet_Data_Descriptor *edd, const void *data)
{
   Eet_File *ef;
   char buff[PATH_MAX];

   if (!domain) return 0;
   snprintf(buff, sizeof(buff), "%s/", dir);
   if (!ecore_file_exists(buff)) ecore_file_mkpath(buff);
   snprintf(buff, sizeof(buff), "%s/%s.tmp", dir, domain);
   ef = eet_open(buff, EET_FILE_MODE_WRITE);
   if (ef)
     {
        char buff2[PATH_MAX];

        snprintf(buff2, sizeof(buff2), "%s/%s.cfg", dir, domain);
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
   EDI_EVENT_CONFIG_CHANGED = ecore_event_type_new();

   _edi_cfg_proj_edd = EDI_CONFIG_DD_NEW("Config_Project", Edi_Config_Project);
   #undef T
   #undef D
   #define T Edi_Config_Project
   #define D _edi_cfg_proj_edd
   EDI_CONFIG_VAL(D, T, name, EET_T_STRING);
   EDI_CONFIG_VAL(D, T, path, EET_T_STRING);

   _edi_cfg_mime_edd = EDI_CONFIG_DD_NEW("Config_Mime", Edi_Config_Mime_Association);
   #undef T
   #undef D
   #define T Edi_Config_Mime_Association
   #define D _edi_cfg_mime_edd
   EDI_CONFIG_VAL(D, T, id, EET_T_STRING);
   EDI_CONFIG_VAL(D, T, mime, EET_T_STRING);

   _edi_cfg_edd = EDI_CONFIG_DD_NEW("Config", Edi_Config);
   #undef T
   #undef D
   #define T Edi_Config
   #define D _edi_cfg_edd
   EDI_CONFIG_VAL(D, T, version, EET_T_INT);
   EDI_CONFIG_VAL(D, T, autosave, EET_T_UCHAR);
   EDI_CONFIG_VAL(D, T, trim_whitespace, EET_T_UCHAR);

   EDI_CONFIG_LIST(D, T, projects, _edi_cfg_proj_edd);
   EDI_CONFIG_LIST(D, T, mime_assocs, _edi_cfg_mime_edd);

   _edi_proj_cfg_tab_edd = EDI_CONFIG_DD_NEW("Project_Config_Tab", Edi_Project_Config_Tab);
   #undef T
   #undef D
   #define T Edi_Project_Config_Tab
   #define D _edi_proj_cfg_tab_edd
   EDI_CONFIG_VAL(D, T, path, EET_T_STRING);
   EDI_CONFIG_VAL(D, T, type, EET_T_STRING);

   _edi_proj_cfg_panel_edd = EDI_CONFIG_DD_NEW("Project_Config_Panel", Edi_Project_Config_Panel);
   #undef T
   #undef D
   #define T Edi_Project_Config_Panel
   #define D _edi_proj_cfg_panel_edd
   EDI_CONFIG_LIST(D, T, tabs, _edi_proj_cfg_tab_edd);
   EDI_CONFIG_VAL(D, T, current_tab, EET_T_UINT);

   _edi_proj_cfg_edd = EDI_CONFIG_DD_NEW("Project_Config", Edi_Project_Config);
   #undef T
   #undef D
   #define T Edi_Project_Config
   #define D _edi_proj_cfg_edd
   EDI_CONFIG_VAL(D, T, version, EET_T_INT);
   EDI_CONFIG_VAL(D, T, font.name, EET_T_STRING);
   EDI_CONFIG_VAL(D, T, font.size, EET_T_INT);
   EDI_CONFIG_VAL(D, T, gui.translucent, EET_T_UCHAR);
   EDI_CONFIG_VAL(D, T, gui.width, EET_T_INT);
   EDI_CONFIG_VAL(D, T, gui.height, EET_T_INT);
   EDI_CONFIG_VAL(D, T, gui.leftsize, EET_T_DOUBLE);
   EDI_CONFIG_VAL(D, T, gui.leftopen, EET_T_UCHAR);
   EDI_CONFIG_VAL(D, T, gui.bottomsize, EET_T_DOUBLE);
   EDI_CONFIG_VAL(D, T, gui.bottomopen, EET_T_UCHAR);
   EDI_CONFIG_VAL(D, T, gui.bottomtab, EET_T_INT);
   EDI_CONFIG_VAL(D, T, gui.show_whitespace, EET_T_UCHAR);
   EDI_CONFIG_VAL(D, T, gui.width_marker, EET_T_UINT);
   EDI_CONFIG_VAL(D, T, gui.tabstop, EET_T_UINT);
   EDI_CONFIG_VAL(D, T, gui.toolbar_hidden, EET_T_UCHAR);
   EDI_CONFIG_VAL(D, T, gui.tab_inserts_spaces, EET_T_UCHAR);

   EDI_CONFIG_VAL(D, T, launch.path, EET_T_STRING);
   EDI_CONFIG_VAL(D, T, launch.args, EET_T_STRING);
   EDI_CONFIG_VAL(D, T, debug_command, EET_T_STRING);
   EDI_CONFIG_VAL(D, T, user_fullname, EET_T_STRING);
   EDI_CONFIG_VAL(D, T, user_email, EET_T_STRING);

   EDI_CONFIG_LIST(D, T, panels, _edi_proj_cfg_panel_edd);
   EDI_CONFIG_LIST(D, T, windows, _edi_proj_cfg_tab_edd);

   _edi_config_load();

   return EINA_TRUE;
}

Eina_Bool
_edi_config_shutdown(void)
{
   _edi_config_cb_free();
   _edi_project_config_cb_free();

   EDI_CONFIG_DD_FREE(_edi_cfg_proj_edd);
   EDI_CONFIG_DD_FREE(_edi_cfg_mime_edd);
   EDI_CONFIG_DD_FREE(_edi_cfg_edd);

   EDI_CONFIG_DD_FREE(_edi_proj_cfg_edd);
   EDI_CONFIG_DD_FREE(_edi_proj_cfg_tab_edd);

   efreet_shutdown();

   return EINA_TRUE;
}

void
_edi_config_load(void)
{
   Eina_Bool save = EINA_FALSE;

   _edi_config = _edi_config_domain_load(_edi_config_dir_get(), EDI_CONFIG_NAME, _edi_cfg_edd);
   if (_edi_config)
     {
        Eina_Bool reload = EINA_FALSE;

        if ((_edi_config->version >> 16) < EDI_CONFIG_FILE_EPOCH)
          {
             /* config too old */
             reload = EINA_TRUE;
          }
        else if (_edi_config->version > EDI_CONFIG_FILE_VERSION)
          {
             /* config too new, WTF ? */
             reload = EINA_TRUE;
          }

        /* if too old or too new, clear it so we can create new */
        if (reload) _edi_config_cb_free();
     }

   if (!_edi_config)
     {
        _edi_config = calloc(1, sizeof(Edi_Config));
        save = EINA_TRUE;
     }

   /* define some convenient macros */
#define IFCFG(v) if ((_edi_config->version & 0xffff) < (v)) {
#define IFCFGELSE } else {
#define IFCFGEND }

   /* setup defaults */
   IFCFG(0x000c);

   _edi_config->autosave = EINA_TRUE;
   _edi_config->trim_whitespace = EINA_TRUE;
   _edi_config->projects = NULL;
   _edi_config->mime_assocs = NULL;
   IFCFGEND;

   _edi_config->version = EDI_CONFIG_FILE_VERSION;

   if (save) _edi_config_save();
}

void
_edi_config_save(void)
{
   if (!_edi_config_domain_save(_edi_config_dir_get(), EDI_CONFIG_NAME, _edi_cfg_edd, _edi_config))
     ERR("Could not save configuration");
}

void
_edi_config_project_add(const char *path)
{
   Edi_Config_Project *project;
   Eina_List *list, *next;

   EINA_LIST_FOREACH_SAFE(_edi_config->projects, list, next, project)
     {
        if (strlen(project->path) == strlen(path) &&
            !strncmp(project->path, path, strlen(project->path)))
          _edi_config->projects = eina_list_remove_list(_edi_config->projects, list);
     }

   project = malloc(sizeof(*project));
   project->path = eina_stringshare_add(path);
   project->name = eina_stringshare_add(basename((char*) path));
   _edi_config->projects = eina_list_prepend(_edi_config->projects, project);
   _edi_config_save();
}

void
_edi_config_project_remove(const char *path)
{
   Edi_Config_Project *project;
   Eina_List *list, *next;

   EINA_LIST_FOREACH_SAFE(_edi_config->projects, list, next, project)
     {
        if (!strncmp(project->path, path, strlen(project->path)))
          break;
     }

   _edi_config->projects = eina_list_remove(_edi_config->projects, project);
   _edi_config_save();

   eina_stringshare_del(project->path);
   eina_stringshare_del(project->name);
   free(project);
}

void
_edi_config_mime_add(const char *mime, const char *id)
{
   Edi_Config_Mime_Association *mime_assoc;

   mime_assoc = malloc(sizeof(*mime_assoc));
   mime_assoc->id = eina_stringshare_add(id);
   mime_assoc->mime = eina_stringshare_add(mime);
   _edi_config->mime_assocs = eina_list_prepend(_edi_config->mime_assocs, mime_assoc);
   _edi_config_save();
}

const char *
_edi_config_mime_search(const char *mime)
{
   Edi_Config_Mime_Association *mime_assoc;
   Eina_List *list, *next;

   EINA_LIST_FOREACH_SAFE(_edi_config->mime_assocs, list, next, mime_assoc)
     {
        if (!strncmp(mime_assoc->mime, mime, strlen(mime_assoc->mime)))
          {
             return mime_assoc->id;
          }
     }
   return NULL;
}

static Edi_Project_Config_Panel *
_panel_add()
{
   Edi_Project_Config_Panel *panel;

   panel = calloc(1, sizeof(Edi_Project_Config_Panel));

   _edi_project_config->panels = eina_list_append(_edi_project_config->panels, panel);
   return panel;
}

Eina_Bool
_edi_project_config_save_no_notify()
{
   return _edi_config_domain_save(_edi_project_config_dir_get(), EDI_PROJECT_CONFIG_NAME, _edi_proj_cfg_edd, _edi_project_config);
}

void
_edi_project_config_save()
{
   if (_edi_project_config_save_no_notify())
     ecore_event_add(EDI_EVENT_CONFIG_CHANGED, NULL, NULL, NULL);
}

void
_edi_project_config_load()
{
   Eina_Bool save = EINA_FALSE;

   _edi_project_config = _edi_config_domain_load(_edi_project_config_dir_get(), EDI_PROJECT_CONFIG_NAME, _edi_proj_cfg_edd);
   if (_edi_project_config)
     {
        Eina_Bool reload = EINA_FALSE;

        if ((_edi_project_config->version >> 16) < EDI_PROJECT_CONFIG_FILE_EPOCH)
          {
             /* config too old */
             reload = EINA_TRUE;
          }
        else if (_edi_project_config->version > EDI_PROJECT_CONFIG_FILE_VERSION)
          {
             /* config too new, WTF ? */
             reload = EINA_TRUE;
          }

        /* if too old or too new, clear it so we can create new */
        if (reload) _edi_project_config_cb_free();
     }

   if (!_edi_project_config)
     {
        _edi_project_config = calloc(1, sizeof(Edi_Project_Config));
        save = EINA_TRUE;
     }

   /* define some convenient macros */
#define IFPCFG(v) if ((_edi_project_config->version & 0xffff) < (v)) {
#define IFPCFGELSE } else {
#define IFPCFGEND }

   /* setup defaults */
   IFPCFG(0x0001);
   _edi_project_config->gui.translucent = EINA_TRUE;
   _edi_project_config->gui.width = 640;
   _edi_project_config->gui.height = 480;
   _edi_project_config->gui.leftsize = 0.25;
   _edi_project_config->gui.leftopen = EINA_TRUE;
   _edi_project_config->gui.bottomsize = 0.2;
   _edi_project_config->gui.bottomopen = EINA_FALSE;
   _edi_project_config->gui.bottomtab = 0;

   _edi_project_config->gui.width_marker = 80;
   _edi_project_config->gui.tabstop = 8;
   _edi_project_config->gui.toolbar_hidden = EINA_FALSE;
   IFPCFGEND;

   IFPCFG(0x0002);
   _edi_project_config->font.name = eina_stringshare_add("Mono");
   _edi_project_config->font.size = 12;
   IFPCFGEND;

   IFPCFG(0x0003);
   _edi_project_config->gui.tab_inserts_spaces = EINA_TRUE;
   IFPCFGEND;

   /* limit config values so they are sane */
   EDI_CONFIG_LIMIT(_edi_project_config->font.size, EDI_FONT_MIN, EDI_FONT_MAX);
   EDI_CONFIG_LIMIT(_edi_project_config->gui.width, 150, 10000);
   EDI_CONFIG_LIMIT(_edi_project_config->gui.height, 100, 8000);
   EDI_CONFIG_LIMIT(_edi_project_config->gui.leftsize, 0.0, 1.0);
   EDI_CONFIG_LIMIT(_edi_project_config->gui.bottomsize, 0.0, 1.0);
   EDI_CONFIG_LIMIT(_edi_project_config->gui.tabstop, 1, 32);

   _edi_project_config->version = EDI_PROJECT_CONFIG_FILE_VERSION;

   IFPCFG(0x0004);
   _edi_project_config->panels = NULL;
   _panel_add();
   _edi_project_config->windows = NULL;
   IFPCFGEND;

   if (save) _edi_project_config_save_no_notify();
}

Eina_List **
_tablist_get(Eina_Bool windowed, int panel_id)
{
   Edi_Project_Config_Panel *panel;

   if (windowed)
     return &_edi_project_config->windows;

   panel = eina_list_nth(_edi_project_config->panels, panel_id);
   while (!panel)
     {
        _panel_add();
        panel = eina_list_nth(_edi_project_config->panels, panel_id);
     }

   return &panel->tabs;
}

void
_edi_project_config_tab_current_set(int panel_id, int tab_id)
{
   Edi_Project_Config_Panel *panel;

   panel = eina_list_nth(_edi_project_config->panels, panel_id);
   while (!panel)
     {
        _panel_add();
        panel = eina_list_nth(_edi_project_config->panels, panel_id);
     }

   panel->current_tab = tab_id;
}

void
_edi_project_config_tab_add(const char *path, const char *type,
                            Eina_Bool windowed, int panel_id)
{
   Edi_Project_Config_Tab *tab;
   Eina_List **tabs;

   tab = malloc(sizeof(*tab));

   // let's keep paths relative
   if (!strncmp(path, edi_project_get(), strlen(edi_project_get())))
     tab->path = eina_stringshare_add(path + strlen(edi_project_get()) + 1);
   else
     tab->path = eina_stringshare_add(path);
   tab->type = eina_stringshare_add(type);

   tabs = _tablist_get(windowed, panel_id);
   *tabs = eina_list_append(*tabs, tab);

   _edi_project_config_save_no_notify();
}

void
_edi_project_config_tab_remove(const char *path, Eina_Bool windowed, int panel_id)
{
   Edi_Project_Config_Tab *tab;
   Eina_List *list, *next, **tabs;

   tabs = _tablist_get(windowed, panel_id);
   EINA_LIST_FOREACH_SAFE(*tabs, list, next, tab)
     {
        if (!strncmp(tab->path, path, strlen(tab->path)))
          break;
        if (!strncmp(path, edi_project_get(), strlen(edi_project_get())) &&
            !strncmp(path + strlen(edi_project_get()) + 1, tab->path, strlen(tab->path)))
          break;
     }

   *tabs = eina_list_remove(*tabs, tab);
   _edi_project_config_save_no_notify();

   eina_stringshare_del(tab->path);
   if (tab->type)
     eina_stringshare_del(tab->type);
   free(tab);
}

void
_edi_project_config_panel_remove(int panel_id)
{
   Edi_Project_Config_Tab *tab;
   Edi_Project_Config_Panel *panel = eina_list_nth(_edi_project_config->panels, panel_id);

   if (!panel)
     return;

   _edi_project_config->panels = eina_list_remove(_edi_project_config->panels, panel);

   EINA_LIST_FREE(panel->tabs, tab)
     {
        if (tab->path)
          eina_stringshare_del(tab->path);
        if (tab->type)
          eina_stringshare_del(tab->type);
        free(tab);
     }

   free(panel);

   _edi_project_config_save_no_notify();
}
