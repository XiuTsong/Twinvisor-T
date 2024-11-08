/*
 * Created on 2024/11/06
 */

#ifndef __SVISOR_ASM_H__
#define __SVISOR_ASM_H__

#define BEGIN_FUNC(_name)  \
    .global     _name;  \
    .type       _name, @function;       \
_name:

#define END_FUNC(_name)  \
    .size _name, .-_name

#define TITANIUM_ENTRY(_name) \
    .global     _name; \
    .type       _name, @function; \
    .balign     4; \
_name:

#define LOCAL_TITANIUM_ENTRY(_name) \
    .type       _name, @function; \
    .balign     4; \
_name:

#define TITANIUM_ENTRY_END(_name)  \
    .type       _name, @function; \
    .size       _name, . - _name

#define CPU_XREG_OFFSET(x)	(8 * x)

#endif