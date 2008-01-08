/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file module_test/pmic_convity_test/test_support.c
 * @brief Support functions for the unit and functional tests.
 *
 * @ingroup PMIC
 */

#include <linux/kernel.h>
#include "test_support.h"   /* For unit test support framework. */

/* Create two structures that maps to the Atlas's control registers
 * and which will allow us to read/write each field.
 */
USBCNTRL_REG_0  regUSB0 = {
    {  0, 0x000001 }, /*!< FSENB        */
    {  1, 0x000002 }, /*!< USB_SUSPEND  */
    {  2, 0x000004 }, /*!< USB_PU       */
    {  3, 0x000008 }, /*!< UDP_PD       */
    {  4, 0x000010 }, /*!< UDM_PD       */
    {  5, 0x000020 }, /*!< DP150K_PU    */
    {  6, 0x000040 }, /*!< VBUSPDENB    */
    {  7, 0x000380 }, /*!< CURRENT_LIMIT*/
    { 10, 0x000400 }, /*!< DLP_SRP      */
    { 11, 0x000800 }, /*!< SE0_CONN     */
    { 12, 0x001000 }, /*!< USBXCVREN    */
    { 13, 0x002000 }, /*!< PULLOVR      */
    { 14, 0x01c000 }, /*!< INTERFACE_MODE*/
    { 17, 0x020000 }, /*!< DATSE0       */
    { 18, 0x040000 }, /*!< BIDIR        */
    { 19, 0x080000 }, /*!< USBCNTRL     */
    { 20, 0x100000 }, /*!< IDPD         */
    { 21, 0x200000 }, /*!< IDPULSE      */
    { 22, 0x400000 }, /*!< IDPUCNTRL    */ 
    { 23, 0x800000 }  /*!< DMPULSE      */
    
};


USBCNTRL_REG_1  regUSB1 = {
    {  0, 0x000003 }, /*!< VUSBIN-2 Bits  */
    {  2, 0x000004 }, /*!< VUSB           */
    {  3, 0x000008 }, /*!< VUSBEN         */
   // {  4, 0x000010 }, /*!< Reserved     */
    {  4, 0x000020 }, /*!< VBUSEN         */
    {  5, 0x000040 }, /*!< RSPOL          */
    {  6, 0x000080 }, /*!< RSTRI          */
    {  7, 0x000100 }  /*!< ID100kPU       */
                      /*!< 9-23 Unused    */     
};  

ATLAS_REG_ISR regISR = {
    {  0, 0x000001 }, /* ADCDONEI     */
    {  1, 0x000002 }, /* ADCBISDONEI  */
    {  2, 0x000004 }, /* TSI          */
    {  3, 0x000008 }, /* WHIGHI       */
    {  4, 0x000010 }, /* WLOWI        */
    {  5, 0x000040 }, /*CHGDETI       */
    {  6, 0x000080 }, /* CHGOVI       */
    {  7, 0x000100 }, /*CHGREVI       */
    {  8, 0x000200 }, /* CHGSHORTI    */
    {  9, 0x000400 }, /* CCCVI        */
    { 10, 0x000800 }, /*CHGCURRI      */
    { 11, 0x001000 }, /* BPONI        */
    { 12, 0x002000 }, /*LOBATLI       */
    { 13, 0x004000 }, /* LOBATHI      */
    { 14, 0x010000 }, /* USBI         */
    { 15, 0x080000 }, /* IDI          */
    { 18, 0x400000 }  /* CKDETI       */
   
};

ATLAS_REG_IMR regIMR = {
    {  0, 0x000001 }, /* ADCDONEM     */
    {  1, 0x000002 }, /* ADCBISDONEM  */
    {  2, 0x000004 }, /* TSM          */
    {  3, 0x000008 }, /* WHIGHM       */
    {  4, 0x000010 }, /* WLOWM        */
    {  5, 0x000040 }, /* CHGDETM      */
    {  6, 0x000080 }, /* CHGOVM       */
    {  7, 0x000100 }, /* CHGREVM      */
    {  8, 0x000200 }, /* CHGSHORTM    */
    {  9, 0x000400 }, /* CCCVM        */
    { 10, 0x000800 }, /* CHGCURRM     */
    { 11, 0x001000 }, /* BPONM        */
    { 12, 0x002000 }, /* LOBATLM      */
    { 14, 0x004000 }, /* LOBATHM      */
    { 15, 0x010000 }, /* USBM         */
    { 16, 0x080000 }, /* IDM          */
    { 18, 0x200000 }, /* SE1M         */
    { 19, 0x400000 }  /* CKDETM       */
   
};

ATLAS_REG_PSTAT regPSTAT = {
    {  6, 0x000040 }, /* CHGDETS      */
    {  7, 0x000080 }, /* CHGOVS       */
    {  8, 0x000100 }, /* CHGREVS      */
    {  9, 0x000200 }, /* CHGSHORTS    */
    { 10, 0x000400 }, /* CCCVS        */
    { 11, 0x000800 }, /* CHGCURRS     */
    { 12, 0x001000 }, /* BPONS        */
    { 14, 0x002000 }, /* LOBATLS      */
    { 16, 0x004000 }, /* LOBATHS      */
    { 17, 0x010000 }, /* USB4V4S      */
    { 18, 0x020000 }, /* USB2V0S      */
    { 20, 0x040000 }, /* USB0V8S      */
    { 21, 0x080000 },  /* IDFLOATS     */
    { 21, 0x100000 },  /*IDGNDS        */   
    { 21, 0x200000 },  /* SE1S         */
    { 21, 0x400000 }  /*CKDETS        */
};
