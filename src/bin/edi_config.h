#ifndef _EDI_CONFIG_H_
# define _EDI_CONFIG_H_ 1

#include <Eina.h>

#include "Edi.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int EDI_EVENT_CONFIG_CHANGED;

typedef struct _Edi_Config_Project Edi_Config_Project;
typedef struct _Edi_Config_Mime_Association Edi_Config_Mime_Association;
typedef struct _Edi_Config Edi_Config;

typedef struct _Edi_Project_Config Edi_Project_Config;
typedef struct _Edi_Project_Config_Tab Edi_Project_Config_Tab;

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

   Eina_List *projects;
   Eina_List *mime_assocs;
};

struct _Edi_Project_Config_Tab
{
   const char *path;
   Eina_Bool windowed;
};

struct _Edi_Project_Config 
{
   int version;

   struct 
     {
        int size;
     } font;

   struct 
     {
        Eina_Bool translucent;
        int width, height, bottomtab;
        double leftsize, bottomsize;
        Eina_Bool leftopen, bottomopen;
        Eina_Bool show_whitespace;
        unsigned int width_marker, tabstop;

        Eina_Bool toolbar_hidden;
     } gui;

   Eina_List *tabs;
};

extern Edi_Config *_edi_config;
extern Edi_Project_Config *_edi_project_config;

// General configuration management

Eina_Bool _edi_config_init(void);
Eina_Bool _edi_config_shutdown(void);

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

void _edi_project_config_tab_add(const char *path, Eina_Bool windowed);
void _edi_project_config_tab_remove(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* _EDI_CONFIG_H_ */
