#ifndef EDI_BUILDER_H_
# define EDI_BUILDER_H_

#include <Ecore.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for Edi building.
 */

/**
 * @brief Main builder management
 * @defgroup Builder
 *
 * @{
 *
 * Functions of build management and execution.
 *
 */

/**
 * Check if Edi can build the current project.
 *
 * @return Whether or not the current project can be built.
 *
 * @see edi_builder_build().
 *
 * @ingroup Builder
 */
EAPI Eina_Bool
edi_builder_can_build(void);

/**
 * Run a build for the current project.
 *
 * @see edi_builder_can_build().
 *
 * @ingroup Builder
 */
EAPI void
edi_builder_build(void);

/**
 * Run a test build for the current project.
 *
 * @see edi_builder_can_build().
 *
 * @ingroup Builder
 */
EAPI void
edi_builder_test(void);

/**
 * Run a clean for the current project.
 *
 * @see edi_builder_can_build().
 *
 * @ingroup Builder
 */
EAPI void
edi_builder_clean(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_BUILDER_H_ */
