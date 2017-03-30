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
 * Management of file/dir creation with the UI
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
 * Create a file add dialogue and add it to the parent obj.
 *
 * @param parent The object into which the UI will load.
 * @param directory The directory root of which file is created.
 * @ingroup UI
 */

void edi_file_create_file(Evas_Object *parent, const char *directory);

/**
 * Create a directory add dialogue and add it to the parent obj.
 *
 * @param parent The object into which the UI will load.
 * @param directory The directory root of which directory is created.
 * @ingroup UI
 */

void edi_file_create_dir(Evas_Object *parent, const char *directory);
/**
 * @}
 */


#ifdef __cplusplus
}
#endif



#endif
