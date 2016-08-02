/***********************  I n c l u d e  -  F i l e  ************************/
/*!  
 *        \file  z47.h
 *
 *      \author  dieter.pfeuffer@men.de
 *        $Date: $
 *    $Revision: $
 * 
 *  	 \brief  Header file for 16Z047 IP core register defines
 *                      
 *     Switches: -
 */
/*-------------------------------[ History ]---------------------------------
 *
 * $Log: z47.h,v $
 *---------------------------------------------------------------------------
 * (c) Copyright 2016 by MEN Mikro Elektronik GmbH, Nuremberg, Germany 
 ****************************************************************************/

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
#define Z47_CFG		0x04	/**< Configuration */
#define Z47_MINT	0x08	/**< Min Timeout */
#define Z47_MAXT	0x0c	/**< Max Timeout */
#define Z47_IRQT	0x10	/**< IRQ Timeout */
#define Z47_CLR		0x14	/**< Counter Clear */


/* Z47_STAT - Status register bits */
#define Z47_STAT_WDOG_CNT_STATUS	0x0001	/**< Counter Status: 1=running */
#define Z47_STAT_WDOG_IRQ_STATUS	0x0002	/**< IRQ Status: 1=IRQ asserted, write 1 to clear */

#define Z47_STAT_REASON_TOUT_MASK	0x000c	/**< Last Timeout Event Reason */
#define Z47_STAT_REASON_TOUT_SHIFT	2		/**< Bit shift for Z47_STAT_REASON_TOUT_MASK */
#define Z47_STAT_REASON_IRQ_MASK	0x0030	/**< Last IRQ Event Reason */
#define Z47_STAT_REASON_IRQ_SHIFT	4		/**< Bit shift for Z47_STAT_REASON_IRQ_MASK */
#define Z47_REASON_MIN_TOUT			0x0001	/**< Min Timeout */
#define Z47_REASON_MAX_TOUT			0x0002	/**< Max Timeout */
#define Z47_REASON_MAN_TOUT			0x0003	/**< Manual set */

#define Z47_STAT_SW_ERR				0x0040	/**< SW Error Indication: 1=WDOG_ERR set */
#define Z47_STAT_SET_WDOG_IRQ		0x0100	/**< WDOG_IRQ status: 1=WDOG_IRQ set */
#define Z47_STAT_SET_WDOG_TOUT		0x0200	/**< WDOG_TIMEOUT status: 1=WDOG_TIMEOUT set */


/* Z47_CFG - Configuration register bits */
#define Z47_CFG_WDOG_EN				0x0001	/**< Counter enable: 1=enabled */
#define Z47_CFG_WDOG_RESTART		0x0002	/**< Watchdog Restart: write 1 to restart */

#ifdef __cplusplus
	}
#endif

#endif	/* _Z47_H */
