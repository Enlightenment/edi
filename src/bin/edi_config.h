#ifndef _EDI_CONFIG_H_
# define _EDI_CONFIG_H_ 1

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Edi_Config_Project Edi_Config_Project;
typedef struct _Edi_Config Edi_Config;

struct _Edi_Config_Project
{
   const char *name;
   const char *path;
};

struct _Edi_Config 
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
     } gui;

   Eina_List *projects;
};

extern Edi_Config *_edi_cfg;

Eina_Bool _edi_config_init(void);
Eina_Bool _edi_config_shutdown(void);
void _edi_config_load(void);
void _edi_config_save(void);

void _edi_config_project_add(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* _EDI_CONFIG_H_ */
