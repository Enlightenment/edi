#ifndef __EDI_FILE_SCREENS_H__
#define __EDI_FILE_SCREENS_H__

#include <Elementary.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines used for managing Edi file actions from UI.
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
 * Create a file add dialogue and add it to the parent obj.
 *
 * @param parent The object into which the UI will load.
 * @param directory The directory root of which file is created.
 * @ingroup UI
 */

void edi_file_screens_create_file(Evas_Object *parent, const char *directory);

/**
 * Create a directory add dialogue and add it to the parent obj.
 *
 * @param parent The object into which the UI will load.
 * @param directory The directory root of which directory is created.
 * @ingroup UI
 */

void edi_file_screens_create_dir(Evas_Object *parent, const char *directory);

/**
 * Create a dialogue for renaming item and add it to the parent obj
 *
 * @param parent The object into which the UI will load
 * @param path The path of the file or directory to rename
 * @ingroup UI
 */

void edi_file_screens_rename(Evas_Object *parent, const char *path);
/**
 * @}
 */


#ifdef __cplusplus
}
#endif



#endif
