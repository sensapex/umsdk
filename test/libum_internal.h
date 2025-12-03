/**
 * @file    libum_internal.h
 * @brief   Internal functions exposed for unit testing only.
 */

#ifndef LIBUM_INTERNAL_H
#define LIBUM_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate axis drive order value
 *
 * Checks that the order contains each axis (0-3) exactly once.
 *
 * @param   order   Axis drive order value
 * @return  1 if valid, 0 if invalid
 */
int is_valid_axis_drive_order(const int order);

#ifdef __cplusplus
}
#endif

#endif // LIBUM_INTERNAL_H
