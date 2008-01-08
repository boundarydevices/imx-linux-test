/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All rights reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 *
 * History :
 *	04/06/29 : Creation
 */
  
 /*!
  * @defgroup dbmx31_audio common definitions for the dbmx sound driver
  */
 
 /*!
  * @file dbmx31-ctrls.h
  * @brief common definitions for the dbmx sound driver
  * @ingroup dbmx31_audio
  */
  
#ifndef _MXC_CTRLS_H
#define _MXC_CTRLS_H

/*!
* Allows the user to add extra IOCTL to get access to DBMX registers
* For debug only
*/
#define DBMX_DBG_CFG
#ifdef DBMX_DBG_CFG
typedef struct _dbmx_cfg {
	int reg;	/*!< The register address (data sheet value) */
	int val;	/*!< The value to write, or returned by read */
} dbmx_cfg;

/*! 
* These controls are only debug purposes. 
*/
#define SNDCTL_DBMX_HW_SSI_CFG_W	_SIOW('Z', 0, dbmx_cfg)		/*!< debug only*/
#define SNDCTL_DBMX_HW_SSI_CFG_R	_SIOR('Z', 1, dbmx_cfg)		/*!< debug only*/
#define SNDCTL_DBMX_HW_SEL_OUT_DAC	_SIO('Z', 2)			/*!< debug only*/
#define SNDCTL_DBMX_HW_SEL_OUT_CODEC	_SIO('Z', 3)			/*!< debug only*/
#define SNDCTL_DBMX_HW_SEL_IN_CODEC	_SIO('Z', 14)			/*!< debug only*/
#define SNDCTL_MC13783_READ_OUT_MIXER	_SIOR('Z', 4, int)		/*!< debug only*/
#define SNDCTL_MC13783_WRITE_OUT_MIXER	_SIOWR('Z', 5, int)		/*!< debug only*/

/*!
* To set the adder configuration, use the audio control :
* SNDCTL_MC13783_WRITE_OUT_MIXER and SNDCTL_MC13783_READ_OUT_MIXER.
* Possible arguments are :
*	 MC13783_AUDIO_ADDER_STEREO
*	 MC13783_AUDIO_ADDER_STEREO_OPPOSITE
*	 MC13783_AUDIO_ADDER_MONO
*	 MC13783_AUDIO_ADDER_MONO_OPPOSITE
*/
#define SNDCTL_MC13783_WRITE_OUT_ADDER	_SIOWR('Z', 7, int)

/*!
* To get the adder configuration, use the audio control :
* SNDCTL_MC13783_WRITE_OUT_MIXER and SNDCTL_MC13783_READ_OUT_MIXER.
* Possible returned values are :
*	 MC13783_AUDIO_ADDER_STEREO
*	 MC13783_AUDIO_ADDER_STEREO_OPPOSITE
*	 MC13783_AUDIO_ADDER_MONO
*	 MC13783_AUDIO_ADDER_MONO_OPPOSITE
*/
#define SNDCTL_MC13783_READ_OUT_ADDER	_SIOR('Z', 6, int)

/*! Definition for SNDCTL_MC13783_WRITE_OUT_ADDER and SNDCTL_MC13783_READ_OUT_ADDER */
#define MC13783_AUDIO_ADDER_STEREO		1
/*! Definition for SNDCTL_MC13783_WRITE_OUT_ADDER and SNDCTL_MC13783_READ_OUT_ADDER */
#define MC13783_AUDIO_ADDER_STEREO_OPPOSITE	2
/*! Definition for SNDCTL_MC13783_WRITE_OUT_ADDER and SNDCTL_MC13783_READ_OUT_ADDER */
#define MC13783_AUDIO_ADDER_MONO			4
/*! Definition for SNDCTL_MC13783_WRITE_OUT_ADDER and SNDCTL_MC13783_READ_OUT_ADDER */
#define MC13783_AUDIO_ADDER_MONO_OPPOSITE		8

/*!
* To get the codec filter configuration, use the audio control :
* SNDCTL_MC13783_WRITE_OUT_BALANCE and SNDCTL_MC13783_READ_OUT_BALANCE.
* Range is 0 (-21 dB left) to 100 (-21 dB right), linear, 3dB step ; 50 is no balance.
*	Examples:
*	 0 : -21dB left		50 : balance deactivated 	100 : -21 dB right
*/
#define SNDCTL_MC13783_READ_OUT_BALANCE	_SIOR('Z', 8, int)

/*!
* To set the codec filter configuration, use the audio control :
* SNDCTL_MC13783_WRITE_OUT_BALANCE and SNDCTL_MC13783_READ_OUT_BALANCE.
* Range is 0 (-21 dB left) to 100 (-21 dB right), linear, 3dB step ; 50 is no balance.
*	Examples:
*	 0 : -21dB left		50 : balance deactivated 	100 : -21 dB right
*/
#define SNDCTL_MC13783_WRITE_OUT_BALANCE	_SIOWR('Z', 9, int)

/*!
* To get the codec filter configuration, use the audio control :
* SNDCTL_MC13783_WRITE_IN_BIAS and SNDCTL_MC13783_READ_IN_BIAS.
* Range is 0 (-21 dB left) to 100 (-21 dB right), linear, 3dB step ; 50 is no balance.
* Possible returned values are :
*	 0 : bias disabled
*	 1 : bias enabled
*/
#define SNDCTL_MC13783_READ_IN_BIAS		_SIOR('Z', 10, int)

/*!
* To set the codec filter configuration, use the audio control :
* SNDCTL_MC13783_WRITE_IN_BIAS and SNDCTL_MC13783_READ_IN_BIAS.
* Range is 0 (-21 dB left) to 100 (-21 dB right), linear, 3dB step ; 50 is no balance.
* Possible arguments are :
*	 0 : to disable the bias
*	 1 : to enable the bias
*/
#define SNDCTL_MC13783_WRITE_IN_BIAS		_SIOWR('Z', 11, int)

/*!
* To set the codec filter configuration, use the audio control :
* SNDCTL_MC13783_WRITE_CODEC_FILTER and SNDCTL_MC13783_READ_CODEC_FILTER.
* The new configuration replaces the old one.
* Possible arguments are :
*	 MC13783_CODEC_FILTER_HIGH_PASS_IN
*	 MC13783_CODEC_FILTER_HIGH_PASS_OUT
*	 MC13783_CODEC_FILTER_DITHERING
*/
#define SNDCTL_MC13783_WRITE_CODEC_FILTER 	_SIOWR('Z', 20, int)

/*!
* To set the codec filter configuration, use the audio control :
* SNDCTL_MC13783_WRITE_CODEC_FILTER and SNDCTL_MC13783_READ_CODEC_FILTER.
* The new configuration replaces the old one.
* Possible returned values are :
*	 MC13783_CODEC_FILTER_HIGH_PASS_IN
*	 MC13783_CODEC_FILTER_HIGH_PASS_OUT
*	 MC13783_CODEC_FILTER_DITHERING
*/
#define SNDCTL_MC13783_READ_CODEC_FILTER		_SIOR('Z', 21, int)

/*! Definition for SNDCTL_MC13783_WRITE_CODEC_FILTER and SNDCTL_MC13783_READ_CODEC_FILTER */
#define MC13783_CODEC_FILTER_DISABLE		0
/*! Definition for SNDCTL_MC13783_WRITE_CODEC_FILTER and SNDCTL_MC13783_READ_CODEC_FILTER */
#define MC13783_CODEC_FILTER_HIGH_PASS_IN		1
/*! Definition for SNDCTL_MC13783_WRITE_CODEC_FILTER and SNDCTL_MC13783_READ_CODEC_FILTER */
#define MC13783_CODEC_FILTER_HIGH_PASS_OUT	2
/*! Definition for SNDCTL_MC13783_WRITE_CODEC_FILTER and SNDCTL_MC13783_READ_CODEC_FILTER */
#define MC13783_CODEC_FILTER_DITHERING		4

/*! 
* These controls are only debug purposes. 
*/
#define SNDCTL_DBMX_HW_WRITE_REG		_SIOW('Z', 100, dbmx_cfg)	/*!< debug only*/
#define SNDCTL_DBMX_HW_READ_REG			_SIOR('Z', 101, dbmx_cfg)	/*!< debug only*/
#define SNDCTL_DBMX_HW_WRITE_SSI_STX0		_SIOW('Z', 102, int)		/*!< debug only*/
#define SNDCTL_DBMX_HW_SSI_LOOPBACK		_SIOW('Z', 103, int)		/*!< debug only*/
#define SNDCTL_DBMX_HW_SSI_FIFO_FULL            _SIO('Z', 104)  		/*!< debug only*/
#endif /* ifdef DBMX_DBG_CFG */

#endif
