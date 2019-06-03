/***********************  I n c l u d e  -  F i l e  ************************/
/*!  
 *        \file  z47.h
 *
 *      \author  dieter.pfeuffer@men.de
 * 
 *  	 \brief  Header file for 16Z047 IP core register defines
 *                      
 *     Switches: -
 */
/*
 *---------------------------------------------------------------------------
 * Copyright (c) 2016-2019, MEN Mikro Elektronik GmbH
 ****************************************************************************/
/*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _Z47_H
#define _Z47_H

#ifdef __cplusplus
	extern "C" {
#endif

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
/* register offsets */
#define Z47_STAT	0x00	/**< Status */
#define Z47_CTRL	0x04	/**< Control */
#define Z47_CFG		0x08	/**< Configuration */
#define Z47_MINT	0x0C	/**< Min Timeout */
#define Z47_MAXT	0x10	/**< Max Timeout */
#define Z47_IRQT	0x14	/**< IRQ Timeout */
#define Z47_CLR		0x18	/**< Counter Clear */


/* Z47_STAT - Status register bits */
#define Z47_STAT_WDOG_CNT_STATUS	0x01	/**< Counter Status: 1=running */
#define Z47_STAT_WDOG_IRQ_STATUS	0x02	/**< IRQ Status: 1=IRQ asserted, write 1 to clear */

#define Z47_STAT_REASON_TOUT_MASK	0x0c	/**< Last Timeout Event Reason */
#define Z47_STAT_REASON_TOUT_SHIFT	2		/**< Bit shift for Z47_STAT_REASON_TOUT_MASK */
#define Z47_STAT_REASON_IRQ_MASK	0x30	/**< Last IRQ Event Reason */
#define Z47_STAT_REASON_IRQ_SHIFT	4		/**< Bit shift for Z47_STAT_REASON_IRQ_MASK */
#define Z47_REASON_MIN_TOUT			0x01	/**< Min Timeout */
#define Z47_REASON_MAX_TOUT			0x02	/**< Max Timeout */
#define Z47_REASON_MAN_TOUT			0x03	/**< Manual set */

/* Z47_CTRL - Control register bits */
#define Z47_CTRL_SW_ERR				0x01	/**< SW Error Indication: 1=WDOG_ERR set */
#define Z47_CTRL_SET_WDOG_IRQ		0x02	/**< WDOG_IRQ status: 1=WDOG_IRQ set */
#define Z47_CTRL_SET_WDOG_TOUT		0x04	/**< WDOG_TIMEOUT status: 1=WDOG_TIMEOUT set */

/* Z47_CFG - Configuration register bits */
#define Z47_CFG_WDOG_EN				0x01	/**< Counter enable: 1=enabled */
#define Z47_CFG_WDOG_RESTART		0x02	/**< Watchdog Restart: write 1 to restart */

#ifdef __cplusplus
	}
#endif

#endif	/* _Z47_H */
