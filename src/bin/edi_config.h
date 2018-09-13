#ifndef _EDI_CONFIG_H_
# define _EDI_CONFIG_H_ 1

#include <Eina.h>

#include "Edi.h"

#define EDI_FONT_MIN 6
#define EDI_FONT_MAX 48

#ifdef __cplusplus
extern "C" {
#endif

extern int EDI_EVENT_CONFIG_CHANGED;

typedef struct _Edi_Config_Project Edi_Config_Project;
typedef struct _Edi_Config_Mime_Association Edi_Config_Mime_Association;
typedef struct _Edi_Config Edi_Config;

typedef struct _Edi_Project_Config Edi_Project_Config;
typedef struct _Edi_Project_Config_Panel Edi_Project_Config_Panel;
typedef struct _Edi_Project_Config_Tab Edi_Project_Config_Tab;
typedef struct _Edi_Project_Config_Launch Edi_Project_Config_Launch;

struct _Edi_Config_Project
{
   const char *name;
   const char *path;
};

struct _Edi_Config_Mime_Association
{
   const char *id;
   const char *mime;
};

struct _Edi_Config
{
   int version;

   Eina_Bool autosave;
   Eina_Bool trim_whitespace;
   Eina_Bool show_hidden;

   Eina_List *projects;
   Eina_List *mime_assocs;
};

struct _Edi_Project_Config_Panel
{
   Eina_List *tabs;
   unsigned int current_tab;
};

struct _Edi_Project_Config_Tab
{
   const char *path;
   const char *fullpath;
   const char *type;
   int split_views;
};

struct _Edi_Project_Config_Launch
{
   const char *path;
   const char *args;
};

struct _Edi_Project_Config
{
   int version;

   struct
     {
        const char *name;
        int size;
     } font;

   struct
     {
        Eina_Bool translucent;
        int alpha;
        const char *theme;
        int width, height, bottomtab;
        double leftsize, bottomsize;
        Eina_Bool leftopen, bottomopen;
        Eina_Bool show_whitespace;
        unsigned int width_marker, tabstop;

        Eina_Bool toolbar_hidden;
        Eina_Bool tab_inserts_spaces;
     } gui;

   Edi_Project_Config_Launch launch;
   Eina_Stringshare *debug_command;
   Eina_Stringshare *user_fullname;
   Eina_Stringshare *user_email;

   Eina_List *panels;
   Eina_List *windows;
};

extern Edi_Config *_edi_config;
extern Edi_Project_Config *_edi_project_config;

// General configuration management

Eina_Bool _edi_config_init(void);
Eina_Bool _edi_config_shutdown(void);
const char *_edi_config_dir_get(void);
const char *_edi_project_config_debug_command_get(void);

// Global configuration handling

void _edi_config_load(void);
void _edi_config_save(void);

void _edi_config_project_add(const char *path);
void _edi_config_project_remove(const char *path);

void _edi_config_mime_add(const char *mime, const char* id);
const char* _edi_config_mime_search(const char *mime);

// Project based configuration handling

void _edi_project_config_load(void);
void _edi_project_config_save(void);

void _edi_project_config_tab_add(const char *path, const char *type,
                                 Eina_Bool windowed, int panel_id);
void _edi_project_config_tab_remove(const char *path, Eina_Bool windowed, int panel_id);
void _edi_project_config_tab_current_set(int panel_id, int tab_id);
void _edi_project_config_panel_remove(int panel_id);
void _edi_project_config_tab_split_view_count_set(const char *path, int panel_id, int count);


#ifdef __cplusplus
}
#endif

#endif /* _EDI_CONFIG_H_ */
