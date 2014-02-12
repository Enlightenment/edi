#ifndef EDI_H_
# define EDI_H_

#include <Elementary.h>

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
 * @brief Main group API that wont do anything
 * @defgroup Main Main
 *
 * @{
 *
 * @brief A function that doesn't do any good nor any bad
 *
 * @ingroup Main
 */
EAPI void edi_library_call(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_H_ */
