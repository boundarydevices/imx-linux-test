/*
 * Copyright (C) 2006-2012 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */


#if !defined(__MEMTOOLS_REGISTER_INFO_H__)
#define __MEMTOOLS_REGISTER_INFO_H__

#include <stdbool.h>

/*!
 * @brief Definition of a module instance.
 *
 * Instance numbers for modules always start at 1. Multi-instance modules will have the
 * instance number appended as part of the name, i.e. "GPIO3".
 */
typedef struct module {
    const char * name; //!< The module's name.
    unsigned instance; //!< Module instance number.
    unsigned base_address; //!< Base address for registers of this module instance.
    unsigned reg_count; //!< Number of registers defined in @a regs array.
    const struct reg * regs; //!< Pointer to array of registers for this module.
} module_t;

/*!
 * @brief Definition of one register.
 */
typedef struct reg {
    const char * name; //!< Register name.
    const char * description; //!< Short description of the register.
    unsigned width; //!< Size in bytes of the register.
    unsigned offset; //!< Offset in bytes from the module's base address.
    bool is_readable; //!< True if the register can be read from.
    bool is_writable; //!< True if the register can be written to.
    unsigned field_count; //!< Number of bitfields in @a fields array.
    const struct field * fields; //!< Pointer to array of bitfields in this register.
} reg_t;

/*!
 * @brief Definition of a bitfield in a register.
 */
typedef struct field {
    const char * name; //!< Bitfield name.
    const char * description; //!< Short description of the bitfield.
    unsigned lsb; //!< Least significant bit of the bitfield.
    unsigned msb; //!< Most significant bit of the bitfield.
    bool is_readable; //!< True if the bitfield can be read. If a bitfield is not readable but is in a readable register, it usually means the bitfield value is always returned as zero.
    bool is_writable; //!< True if the bitfield can be modified.
} field_t;

//! @brief Array of modules available in the chip.
//!
//! This array is terminated with a null entry.
extern const module_t mx6[];

#endif // __MEMTOOLS_REGISTER_INFO_H__
////////////////////////////////////////////////////////////////////////////////
