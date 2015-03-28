#ifndef EDI_H_
# define EDI_H_

#include <Eina.h>
#include <Eio.h>

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
# ifdef EFL_EDI_BUILD
#  ifdef DLL_EXPORT
#   define EAPI __declspec(dllexport)
#  else
#   define EAPI
#  endif /* ! DLL_EXPORT */
# else
#  define EAPI __declspec(dllimport)
# endif /* ! EFL_EDI_BUILD */
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif /* ! _WIN32 */

#ifdef __cplusplus
extern "C" {
#endif

#include <edi_create.h>
#include <edi_builder.h>
#include <edi_path.h>

/**
 * @file
 * @brief These routines are used for Edi library interaction.
 */

/**
 * @brief Init / shutdown functions.
 * @defgroup Init  Init / Shutdown
 *
 * @{
 *
 * Functions of obligatory usage, handling proper initialization
 * and shutdown routines.
 *
 * Before the usage of any other function, Edi should be properly
 * initialized with @ref edi_init() and the last call to Edi's
 * functions should be @ref edi_shutdown(), so everything will
 * be correctly freed.
 *
 * Edi logs everything with Eina Log, using the "edi" log domain.
 *
 */

/**
 * Initialize Edi.
 *
 * Initializes Edi, its dependencies and modules. Should be the first
 * function of Edi to be called.
 *
 * @return The init counter value.
 *
 * @see edi_shutdown().
 *
 * @ingroup Init
 */
EAPI int edi_init(void);

/**
 * Shutdown Edi
 *
 * Shutdown Edi. If init count reaches 0, all the internal structures will
 * be freed. Any Edi library call after this point will leads to an error.
 *
 * @return Edi's init counter value.
 *
 * @see edi_init().
 *
 * @ingroup Init
 */
EAPI int edi_shutdown(void);

/**
 * @}
 */

/**
 * @brief Main group API that manages Edi projects
 * @defgroup Main Project level functions
 *
 * @{
 *
 * Set the current edi project that is loaded.
 * Any directory is deemed a valid project.
 *
 * @param path The path to the current project being loaded.
 * @return EINA_TRUE if the path represented a valid project,
 *   EINA_FALSE otherwise
 *
 * @ingroup Main
 */
EAPI Eina_Bool edi_project_set(const char *path);

/**
 * Get the current edi project that is loaded.
 *
 * @return the project that Edi is current working with.
 *
 * @ingroup Main
 */
EAPI const char *edi_project_get(void);

/**
 * Get the path to a file within the current project.
 *
 * @param file The file within a project to get the absolute path for.
 *
 * @return the full path to the requested file
 *
 * @ingroup Main
 */
EAPI const char *edi_project_file_path_get(const char *file);

/**
 * Find if a requested file exists within the current project.
 *
 * @param file The filename to check for existance within the current project.
 *
 * @return whether or not the requested file exists within the current project.
 *
 * @ingroup Main
 */
EAPI Eina_Bool edi_project_file_exists(const char *file);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_H_ */
