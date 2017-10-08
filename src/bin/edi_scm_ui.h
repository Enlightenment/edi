#ifndef __EDI_SCM_UI_H__
#define __EDI_SCM_UI_H__

#include <Elementary.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines used for managing Edi SCM UI actions.
 */

/**
 * @brief SCM management functions.
 * @defgroup SCM
 *
 * @{
 *
 * Management of SCM UI actions. 
 *
 */

/**
 * Create the commit dialog UI.
 * 
 * @param parent Parent object to add the commit UI to.
 * @ingroup SCM
 */
void edi_scm_ui_add(Evas_Object *parent);

char *_edi_scm_ui_workdir_get(void);
/**
 * @}
 */


#ifdef __cplusplus
}
#endif



#endif
