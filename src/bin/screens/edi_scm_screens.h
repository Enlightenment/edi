#ifndef __EDI_SCM_SCREENS_H__
#define __EDI_SCM_SCREENS_H__

#include <Elementary.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines used for managing Edi SCM actions from UI.
 */

/**
 * @brief Scm management functions.
 * @defgroup Scm 
 *
 * @{
 *
 * Management of SCM with the UI
 *
 */

/**
 * Create a SCM commit dialogue in the parent obj.
 *
 * @param parent The object into which the UI will load.
 * @ingroup Scm
 */
void edi_scm_screens_commit(Evas_Object *parent);

/**
 * SCM binary is not installed, show dialogue.
 *
 * @param parent The object into which the UI will load.
 * @param binary The name of the missing binary.
 *
 * @ingroup Scm
 */
void edi_scm_screens_binary_missing(Evas_Object *parent, const char *binary);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif



#endif
