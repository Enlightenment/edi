#ifndef EDI_CREATE_H_
# define EDI_CREATE_H_

#include <Elementary.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for creating new projects.
 */

typedef void (*Edi_Create_Cb)(const char *path, Eina_Bool success);

typedef struct _Edi_Create
{
   char *path, *name;
   char *url, *user, *email;

   Edi_Create_Cb callback;
   Ecore_Event_Handler *handler;

   int filters;
} Edi_Create;

/**
 * @brief Main builder management
 * @defgroup Creation
 *
 * @{
 *
 * Functions of project creation from skeletons.
 *
 */

/**
 * Create a new standard EFL project.
 *
 * @ingroup Creation
 */
EAPI void
edi_create_efl_project(const char *parentdir, const char *name, const char *url,
                       const char *user, const char *email, Edi_Create_Cb func);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_CREATE_H_ */
