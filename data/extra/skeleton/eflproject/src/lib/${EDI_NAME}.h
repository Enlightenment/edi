#ifndef ${EDI_NAME}_H_
# define ${EDI_NAME}_H_

#include <Elementary.h>

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
# ifdef EFL_${EDI_NAME}_BUILD
#  ifdef DLL_EXPORT
#   define EAPI __declspec(dllexport)
#  else
#   define EAPI
#  endif /* ! DLL_EXPORT */
# else
#  define EAPI __declspec(dllimport)
# endif /* ! EFL_${EDI_NAME}_BUILD */
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
 * @brief These routines are used for ${EDI_NAME} library interaction.
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
 * Before the usage of any other function, ${EDI_NAME} should be properly
 * initialized with @ref ${edi_name}_init() and the last call to ${EDI_NAME}'s
 * functions should be @ref ${edi_name}_shutdown(), so everything will
 * be correctly freed.
 *
 * ${EDI_NAME} logs everything with Eina Log, using the "${edi_name}" log domain.
 *
 */

/**
 * Initialize ${EDI_NAME}.
 *
 * Initializes ${EDI_NAME}, its dependencies and modules. Should be the first
 * function of ${EDI_NAME} to be called.
 *
 * @return The init counter value.
 *
 * @see ${edi_name}_shutdown().
 *
 * @ingroup Init
 */
EAPI int ${edi_name}_init(void);

/**
 * Shutdown ${EDI_NAME}
 *
 * Shutdown ${EDI_NAME}. If init count reaches 0, all the internal structures will
 * be freed. Any ${EDI_NAME} library call after this point will leads to an error.
 *
 * @return ${EDI_NAME}'s init counter value.
 *
 * @see ${edi_name}_init().
 *
 * @ingroup Init
 */
EAPI int ${edi_name}_shutdown(void);

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
EAPI void ${edi_name}_library_call(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ${EDI_NAME}_H_ */
