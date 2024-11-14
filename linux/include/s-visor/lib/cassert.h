/*
 * @Description: trusted-firmware-a cassert.h
 * @Date: 2024-11-11 19:28:15
 */

#ifndef __SVISOR_CASSERT_H
#define __SVISOR_CASSERT_H

/*******************************************************************************
 * Macro to flag a compile time assertion. It uses the preprocessor to generate
 * an invalid C construct if 'cond' evaluates to false.
 * The following compilation error is triggered if the assertion fails:
 * "error: size of array 'msg' is negative"
 * The 'unused' attribute ensures that the unused typedef does not emit a
 * compiler warning.
 ******************************************************************************/
#define CASSERT(cond, msg)	\
	typedef char msg[(cond) ? 1 : -1] __unused

#endif