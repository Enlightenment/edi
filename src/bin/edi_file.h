#ifndef __EDI_FILE_H__
#define __EDI_FILE_H__

#include <Elementary.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines used for managing Edi file actions.
 */

/**
 * @brief UI management functions.
 * @defgroup UI
 *
 * @{
 *
 * Management of file/dir actions. 
 *
 */

/**
 * Check the path is not hidden according to project rules.
 * 
 * @param path The file path to check.
 * @ingroup Lookup
 */

Eina_Bool edi_file_path_hidden(const char *path);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif



#endif
