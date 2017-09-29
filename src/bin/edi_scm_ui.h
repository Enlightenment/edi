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
 * Check the path is not hidden according to project rules.
 * 
 * @param path The file path to check.
 * @ingroup Lookup
 */
void edi_scm_ui_add(Evas_Object *parent);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif



#endif
