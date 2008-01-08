#ifndef __TEST_SUPPORT_H__
#define __TEST_SUPPORT_H__

#define DEBUG

#ifdef DEBUG
#define TRACEMSG(fmt,args...)  printk(fmt,##args)
#else  /* DEBUG */
#define TRACEMSG(fmt,args...)
#endif /* DEBUG */

#define SET_BITS(reg, field, value)    (((value) << reg.field.offset) & \
                                        reg.field.mask)

#define GET_BITS(field, value)    (((value) & regBUSCTRL.field.mask) >> \
                                   regBUSCTRL.field.offset)

#include "asm/arch/pmic_status.h"
#include "asm/arch/pmic_external.h"

typedef const struct
{
    unsigned char offset;
    unsigned int  mask;
} REGFIELD;

typedef const struct
{   
    REGFIELD FSENB;           /*!< USB Full Speed Enable                            */
    REGFIELD USB_SUSPEND;     /*!< USB Suspend Mode Enable                          */
    REGFIELD USB_PU;          /*!< USB Pullup Enable                                */
    REGFIELD UDP_PD;          /*!< USB Data Plus Pulldown Enable                    */
    REGFIELD UDM_PD;          /*!< USB 150K UDP Pullup Enable                       */
    REGFIELD DP150K_PU;       /*!< USB Pullup/Pulldown Override Enable              */
    REGFIELD VBUSPDENB;       /*!< USB VBUS Pulldown NMOS Switch Enable             */
    REGFIELD CURRENT_LIMIT;   /*!< USB Regulator Current Limit Setting-3 bits       */
    REGFIELD DLP_SRP;         /*!< USB Data Line Pulsing Timer Enable               */
    REGFIELD SE0_CONN;        /*!< USB Pullup Connect When SE0 Detected             */
    REGFIELD USBXCVREN;       /*!< USB Transceiver Enabled When INTERFACE_MODE[2:0]=000 and RESETB=high*/
    REGFIELD PULLOVR;         /*!< 1K5 Pullup and UDP/UDM Pulldown Disable When UTXENB=Low             */ 
    REGFIELD INTERFACE_MODE;  /*!< Connectivity Interface Mode Select-3 Bits        */
    REGFIELD DATSE0;          /*!< USB Single or Differential Mode Select           */
    REGFIELD BIDIR;           /*!< USB Unidirectional/Bidirectional Transmission    */
    REGFIELD USBCNTRL;        /*!< USB Mode of Operation controlled By USBEN/SPI Pin*/  
    REGFIELD IDPD;            /*!< USB UID Pulldown Enable                          */
    REGFIELD IDPULSE;         /*!< USB Pulse to Gnd on UID Line Generated           */  
    REGFIELD IDPUCNTRL;       /*!< USB UID Pin pulled high By 5ua Curr Source       */
    REGFIELD DMPULSE;         /*!< USB Positive pulse on the UDM Line Generated     */
} USBCNTRL_REG_0;

typedef const struct 
{
    REGFIELD VUSBIN;   /*!< Controls The Input Source For VUSB             */ 
    REGFIELD VUSB;     /*!< VUSB Output Voltage Select-High=3.3V Low=2.775V*/
    REGFIELD VUSBEN;   /*!< VUSB Output Enable-                            */
    REGFIELD VBUSEN;   /*!< VBUS Output Enable-                           */
    REGFIELD RSPOL;    /*!< Low=RS232 TX on UDM, RX on UDP
                            High= RS232 TX on UDP, RX on UDM               */ 
    REGFIELD RSTRI;    /*!< TX Forced To Tristate in RS232 Mode Only       */
    REGFIELD ID100kPU; /*!< 100k UID Pullup Enabled                        */
} USBCNTRL_REG_1;

typedef const struct
{
    REGFIELD ADCDONEI;
    REGFIELD ADCBISDONEI;
    REGFIELD TSI;
    REGFIELD WHIGHI;
    REGFIELD WLOWI;
    REGFIELD CHGDETI;
    REGFIELD CHGOVI;
    REGFIELD CHGREVI;
    REGFIELD CHGSHORTI;
    REGFIELD CCCVI;
    REGFIELD CHGCURRI;
    REGFIELD BPONI;
    REGFIELD LOBATLI;
    REGFIELD LOBATHI;
    REGFIELD USBI;
    REGFIELD IDI;
    REGFIELD E1I;
    REGFIELD CKDETI;
} ATLAS_REG_ISR;


typedef const struct 
{
    REGFIELD ADCDONEM;
    REGFIELD ADCBISDONEM;
    REGFIELD TSM;
    REGFIELD WHIGHM;
    REGFIELD WLOWM;
    REGFIELD CHGDETM;
    REGFIELD CHGOVM;
    REGFIELD CHGREVM;
    REGFIELD CHGSHORTM;
    REGFIELD CCCVM;
    REGFIELD CHGCURRM;
    REGFIELD BPONM;
    REGFIELD LOBATLM;
    REGFIELD LOBATHM;
    REGFIELD USBM;
    REGFIELD IDM;
    REGFIELD SE1M;
    REGFIELD CKDETM;
} ATLAS_REG_IMR;

typedef const struct
{
    REGFIELD CHGDETS;
    REGFIELD CHGOVS;
    REGFIELD CHGREVS;
    REGFIELD CHGSHORTS;
    REGFIELD CCCVS;
    REGFIELD CHGCURRS;
    REGFIELD BPONS;
    REGFIELD LOBATLS;
    REGFIELD LOBATHS;
    REGFIELD USB4V4S;
    REGFIELD USB2V0S;
    REGFIELD USB0V8S;
    REGFIELD IDFLOATS;
    REGFIELD IDGNDS;
    REGFIELD SE1S;
    REGFIELD CKDETS;
} ATLAS_REG_PSTAT;


typedef enum
{
    COMPARE_EQUAL,
    COMPARE_NOT_EQUAL,
    COMPARE_GREATER_THAN,
    COMPARE_GREATER_THAN_OR_EQUAL,
    COMPARE_LESS_THAN,
    COMPARE_LESS_THAN_OR_EQUAL,
    COMPARE_CUSTOM,
    DONT_CARE
} COMPARE_VALUE;

typedef PMIC_STATUS (*const COMPARE_FUNC)(const unsigned int actual,
                                          const unsigned int mask,
                                          const unsigned int expected);

extern USBCNTRL_REG_0 regUSB0;
extern USBCNTRL_REG_1 regUSB1;
extern ATLAS_REG_ISR  regISR;
extern ATLAS_REG_IMR  regIMR;
extern ATLAS_REG_PSTAT regPSTAT;
/*
void read_register(const t_prio priority,
		   const mc13783 registerName,
		   const char *const moduleID,
		   const char *const description);

PMIC_STATUS read_register_compare(const t_prio priority,
			          const mc13783_reg registerName,
			          const COMPARE_VALUE compareType,
			          COMPARE_FUNC compareFunc,
			          const unsigned int mask,
			          const unsigned int expectedValue,
			          const char *const moduleID,
			          const char *const description);*/

#endif /* __TEST_SUPPORT_H__ */
