/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  z47_drv.c
 *
 *      \author  dieter.pfeuffer@men.de 
 *        $Date: $
 *    $Revision: $
 *
 *       \brief  Low-level driver for the 16Z047 watchdog IP core
 *
 *     Required: OSS, DESC, DBG libraries
 *
 *    \switches  _ONE_NAMESPACE_PER_DRIVER_
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

#define _NO_LL_HANDLE        /* ll_defs.h: don't define LL_HANDLE struct */

/*-----------------------------------------+
|  INCLUDES                                |
+-----------------------------------------*/
#include <MEN/men_typs.h>    /* system dependent definitions   */
#include <MEN/maccess.h>     /* hw access macros and types     */
#include <MEN/dbg.h>         /* debug functions                */
#include <MEN/oss.h>         /* oss functions                  */
#include <MEN/desc.h>        /* descriptor functions           */
#include <MEN/mdis_api.h>    /* MDIS global defs               */
#include <MEN/mdis_com.h>    /* MDIS common defs               */
#include <MEN/mdis_err.h>    /* MDIS error codes               */
#include <MEN/ll_defs.h>     /* low-level driver definitions   */
#include <MEN/wdog.h>        /* watchdog status codes          */
#include <MEN/z47.h>         /* 16Z047 IP core reg definitions */
#include <MEN/chameleon.h>   /* chameleon header               */

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/

/* general defines */
#define CH_NUMBER          1          /**< number of device channels      */
#define USE_IRQ			   TRUE       /**< interrupt required             */
#define ADDRSPACE_COUNT    1          /**< nbr of required address spaces */
#define ADDRSPACE_SIZE     0x18       /**< size of address space          */

/* debug defines */
#define DBG_MYLEVEL        llHdl->dbgLevel    /**< debug level  */
#define DBH                llHdl->dbgHdl      /**< debug handle */
#define OSH                llHdl->osHdl       /**< OS handle    */

/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/** low-level handle */
typedef struct {
	/* general */
	int32                   memAlloc;       /**< size allocated for the handle */
	OSS_HANDLE              *osHdl;         /**< oss handle */
	OSS_IRQ_HANDLE          *irqHdl;        /**< irq handle */
	DESC_HANDLE             *descHdl;       /**< desc handle */
	MACCESS                 ma;             /**< hw access handle */
	MDIS_IDENT_FUNCT_TBL    idFuncTbl;      /**< id function table */
	/* debug */
	u_int32                 dbgLevel;       /**< debug level  */
	DBG_HANDLE              *dbgHdl;        /**< debug handle */
	/* misc */
	OSS_SIG_HANDLE          *sigHdl;        /**< signal for port change */
	u_int32                 irqCount;       /**< interrupt counter */
	u_int8					wdStart;		/**< 1=wdog started */
} LL_HANDLE;

/* include files which need LL_HANDLE */
#include <MEN/ll_entry.h>       /* low-level driver jump table */
#include <MEN/z47_drv.h>        /* Z47 driver header file      */

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
static int32 Z47_Init(DESC_SPEC *descSpec, OSS_HANDLE *osHdl,
						MACCESS *ma, OSS_SEM_HANDLE *devSemHdl,
						OSS_IRQ_HANDLE *irqHdl, LL_HANDLE **llHdlP);
static int32 Z47_Exit(LL_HANDLE **llHdlP);
static int32 Z47_Read(LL_HANDLE *llHdl, int32 ch, int32 *value);
static int32 Z47_Write(LL_HANDLE *llHdl, int32 ch, int32 value);
static int32 Z47_SetStat(LL_HANDLE *llHdl,int32 ch, int32 code,
							INT32_OR_64 value32_or_64);
static int32 Z47_GetStat(LL_HANDLE *llHdl, int32 ch, int32 code,
							INT32_OR_64 *value32_or64P);
static int32 Z47_BlockRead(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
							int32 *nbrRdBytesP);
static int32 Z47_BlockWrite(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
								int32 *nbrWrBytesP);
	static int32 Z47_Irq(LL_HANDLE *llHdl);
static int32 Z47_Info(int32 infoType, ...);
static char* Ident(void);
static int32 Cleanup(LL_HANDLE *llHdl, int32 retCode);
static int32 WdogTrig(LL_HANDLE *llHdl, int32 pat);

/****************************** Z47_GetEntry ********************************/
/** Initialize driver's jump table
 *
 *  \param drvP     \OUT pointer to the initialized jump table structure
 */
#ifdef _ONE_NAMESPACE_PER_DRIVER_
	extern void LL_GetEntry(
		LL_ENTRY* drvP
	)
#else
	extern void __Z47_GetEntry(
		LL_ENTRY* drvP
	)
#endif
{
	drvP->init        = Z47_Init;
	drvP->exit        = Z47_Exit;
	drvP->read        = Z47_Read;
	drvP->write       = Z47_Write;
	drvP->blockRead   = Z47_BlockRead;
	drvP->blockWrite  = Z47_BlockWrite;
	drvP->setStat     = Z47_SetStat;
	drvP->getStat     = Z47_GetStat;
	drvP->irq         = Z47_Irq;
	drvP->info        = Z47_Info;
}

/******************************** Z47_Init **********************************/
/** Allocate and return low-level handle, initialize hardware
 *
 * The function resets the watchdog counter, WDOG_TIMEOUT, WDOG_IRQ signals
 * and restarts the watchdog functionality. Watchdog start/stop is not affected.
 *
 * The following descriptor keys are used:
 *
 * \code
 * Descriptor key        Default          Range
 * --------------------  ---------------  -------------
 * DEBUG_LEVEL_DESC      OSS_DBG_DEFAULT  see dbg.h
 * DEBUG_LEVEL           OSS_DBG_DEFAULT  see dbg.h
 * \endcode
 *
 *  \param descP      \IN  pointer to descriptor data
 *  \param osHdl      \IN  oss handle
 *  \param ma         \IN  hw access handle
 *  \param devSemHdl  \IN  device semaphore handle
 *  \param irqHdl     \IN  irq handle
 *  \param llHdlP     \OUT pointer to low-level driver handle
 *
 *  \return           \c 0 on success or error code
 */
static int32 Z47_Init(
	DESC_SPEC       *descP,
	OSS_HANDLE      *osHdl,
	MACCESS         *ma,
	OSS_SEM_HANDLE  *devSemHdl,
	OSS_IRQ_HANDLE  *irqHdl,
	LL_HANDLE       **llHdlP
)
{
	LL_HANDLE *llHdl = NULL;
	u_int32 gotsize;
	int32 error;
	u_int32 value;

	/*------------------------------+
	|  prepare the handle           |
	+------------------------------*/
	*llHdlP = NULL;		/* set low-level driver handle to NULL */

	/* alloc */
	if ((llHdl = (LL_HANDLE*)OSS_MemGet(
					osHdl, sizeof(LL_HANDLE), &gotsize)) == NULL)
		return (ERR_OSS_MEM_ALLOC);

	/* clear */
	OSS_MemFill(osHdl, gotsize, (char*)llHdl, 0x00);

	/* init */
	llHdl->memAlloc    = gotsize;
	llHdl->osHdl       = osHdl;
	llHdl->irqHdl      = irqHdl;
	llHdl->ma          = *ma;

	/*------------------------------+
	|  init id function table       |
	+------------------------------*/
	/* driver's ident function */
	llHdl->idFuncTbl.idCall[0].identCall = Ident;
	/* library's ident functions */
	llHdl->idFuncTbl.idCall[1].identCall = DESC_Ident;
	llHdl->idFuncTbl.idCall[2].identCall = OSS_Ident;
	/* terminator */
	llHdl->idFuncTbl.idCall[3].identCall = NULL;

	/*------------------------------+
	|  prepare debugging            |
	+------------------------------*/
	DBG_MYLEVEL = OSS_DBG_DEFAULT;		/* set OS specific debug level */
	DBGINIT((NULL,&DBH));

	/*------------------------------+
	|  scan descriptor              |
	+------------------------------*/
	if ((error = DESC_Init(descP, osHdl, &llHdl->descHdl)))
		return (Cleanup(llHdl, error));

	/* DEBUG_LEVEL_DESC */
	if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT,
								&value, "DEBUG_LEVEL_DESC")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return (Cleanup(llHdl, error));

	DESC_DbgLevelSet(llHdl->descHdl, value);

	/* DEBUG_LEVEL */
	if ((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT,
								&llHdl->dbgLevel, "DEBUG_LEVEL")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return (Cleanup(llHdl, error));

	/*------------------------------+
	|  init hardware                |
	+------------------------------*/
	/* do nothing */

	*llHdlP = llHdl;		/* set low-level driver handle */

	return (ERR_SUCCESS);
}

/****************************** Z47_Exit ************************************/
/** De-initialize hardware and clean up memory
 *
 *  The function disables the interrupt.
 *
 *  \param llHdlP     \IN  pointer to low-level driver handle
 *
 *  \return           \c 0 on success or error code
 */
static int32 Z47_Exit(
	LL_HANDLE **llHdlP
)
{
	LL_HANDLE *llHdl = *llHdlP;
	int32 error = 0;

	DBGWRT_1((DBH, "LL - Z47_Exit\n"));

	/*------------------------------+
	|  de-init hardware             |
	+------------------------------*/
	/* disable interrupt */
	MWRITE_D32(llHdl->ma, Z47_IRQT, 0);

	/*------------------------------+
	|  clean up memory              |
	+------------------------------*/
	*llHdlP = NULL;		/* set low-level driver handle to NULL */
	error = Cleanup(llHdl, error);

	return (error);
}

/****************************** Z47_Read ************************************/
/** Read a value from the device
 *
 *  The function is not supported and always returns an ERR_LL_ILL_FUNC error.
 *
 *  \param llHdl      \IN  low-level handle
 *  \param ch         \IN  current channel
 *  \param valueP     \OUT read value
 *
 *  \return           \c 0 on success or error code
 */
static int32 Z47_Read(
	LL_HANDLE  *llHdl,
	int32      ch,
	int32      *valueP
)
{
	DBGWRT_1((DBH, "LL - Z47_Read: ch=%d\n", ch));

	return(ERR_LL_ILL_FUNC);
}

/****************************** Z47_Write ***********************************/
/** Description:  Write a value to the device
 *
 *  The function is not supported and always returns an ERR_LL_ILL_FUNC error.
 *
 *  \param llHdl      \IN  low-level handle
 *  \param ch         \IN  current channel
 *  \param value      \IN  value to write
 *
 *  \return           \c 0 on success or error code
 */
static int32 Z47_Write(
	LL_HANDLE  *llHdl,
	int32      ch,
	int32      value
)
{
	DBGWRT_1((DBH, "LL - Z47_Write: ch=%d  val=0x%x\n", ch, value));

	return(ERR_LL_ILL_FUNC);
}

/****************************** Z47_SetStat *********************************/
/** Set the driver status
 *
 *  The driver supports \ref getstat_setstat_codes "these status codes"
 *  in addition to the standard codes (see mdis_api.h).
 *
 *  \param llHdl         \IN  low-level handle
 *  \param code          \IN  \ref getstat_setstat_codes "status code"
 *  \param ch            \IN  current channel
 *  \param value32_or_64 \IN  data or pointer to block data structure
 *                            (M_SG_BLOCK) for block status codes
 *  \return              \c 0 on success or error code
 */
static int32 Z47_SetStat(
	LL_HANDLE   *llHdl,
	int32       code,
	int32       ch,
	INT32_OR_64 value32_or_64
)
{
	char *func = "LL - Z47_SetStat";
	int32 value = (int32)value32_or_64;		/* 32bit value */
	MACCESS ma = llHdl->ma;
	int32 error = ERR_SUCCESS;

	DBGWRT_1((DBH, "%s: ch=%d code=0x%04x value=0x%x\n",
				func, ch, code, value));

	switch (code) {
		/*--------------------------+
		|  debug level              |
		+--------------------------*/
		case M_LL_DEBUG_LEVEL:
			llHdl->dbgLevel = value;
			break;
		/*--------------------------+
		|  enable interrupts        |
		+--------------------------*/
		case M_MK_IRQ_ENABLE:
			break;
		/*--------------------------+
		|  set irq counter          |
		+--------------------------*/
		case M_LL_IRQ_COUNT:
			llHdl->irqCount = value;
			break;
		/*--------------------------+
		|  channel direction        |
		+--------------------------*/
		case M_LL_CH_DIR:
			if (value != M_CH_INOUT) {
				error = ERR_LL_ILL_DIR;
			}
			break;
		/*--------------------------+
		|  old WDOG codes           |
		+--------------------------*/
		case WDOG_START:
			MSETMASK_D32(ma, Z47_CFG, Z47_CFG_WDOG_EN);
			llHdl->wdStart = 1;
			break;

		case WDOG_STOP:
			MCLRMASK_D32(ma, Z47_CFG, Z47_CFG_WDOG_EN);
			llHdl->wdStart = 0;
			break;

		case WDOG_TRIG:
			error = WdogTrig(llHdl, 0);
			break;

		case WDOG_TIME:
			/* given time [ms], IP core time [us] */
			MWRITE_D32(ma, Z47_MAXT, value * 1000);
			break;

		/*--------------------------+
		|  new WDOG codes           |
		+--------------------------*/
		case WDOG_RESET_CTRL:
			MSETMASK_D32(ma, Z47_CFG, Z47_CFG_WDOG_RESTART);
			break;

		case WDOG_TRIG_PAT:
			error = WdogTrig(llHdl, value);
			break;

		case WDOG_TIME_MIN:
			/* given time [us], IP core time [us] */
			MWRITE_D32(ma, Z47_MINT, value);
			break;

		case WDOG_TIME_MAX:
			/* given time [us], IP core time [us] */
			MWRITE_D32(ma, Z47_MAXT, value);
			break;

		case WDOG_TIME_IRQ:
			/*
			 * given time [us], IP core time [us]
			 * Note: Value>0 enables the interrupt!
			 */
			MWRITE_D32(ma, Z47_IRQT, value);
			break;

		case WDOG_OUT_PIN:
			switch (value){
			case 0:
				MCLRMASK_D32(ma, Z47_CTRL, Z47_CTRL_SET_WDOG_TOUT);
			break;
			case 1:
				MSETMASK_D32(ma, Z47_CTRL, Z47_CTRL_SET_WDOG_TOUT);
				break;
			default:
				DBGWRT_ERR((DBH, "*** %s(WDOG_OUT_PIN): illegal pattern 0x%x\n",
					func, value));
				return ERR_LL_ILL_PARAM;
			}
			break;

		case WDOG_OUT_REASON:
			MCLRMASK_D32(ma, Z47_STAT, Z47_STAT_REASON_TOUT_MASK);
			break;

		case WDOG_IRQ_PIN:
			switch (value) {
			case 0:
				MCLRMASK_D32(ma, Z47_CTRL, Z47_CTRL_SET_WDOG_IRQ);
				break;
			case 1:
				MSETMASK_D32(ma, Z47_CTRL, Z47_CTRL_SET_WDOG_IRQ);
				break;
			default:
				DBGWRT_ERR((DBH, "*** %s(WDOG_IRQ_PIN): illegal pattern 0x%x\n",
					func, value));
				return ERR_LL_ILL_PARAM;
			}
			break;

		case WDOG_IRQ_SIGSET:
			/* signal already installed ? */
			if (llHdl->sigHdl) {
				error = ERR_OSS_SIG_SET;
				break;
			}
			error = OSS_SigCreate(OSH, value, &llHdl->sigHdl);
			break;

		case WDOG_IRQ_SIGCLR:
			/* signal not installed ? */
			if (llHdl->sigHdl == NULL) {
				error = ERR_OSS_SIG_CLR;
				break;
			}
			error = OSS_SigRemove(OSH, &llHdl->sigHdl);
			break;

		case WDOG_IRQ_REASON:
			MCLRMASK_D32(ma, Z47_STAT, Z47_STAT_REASON_IRQ_MASK);
			break;

		case WDOG_ERR_PIN:
			switch (value) {
			case 0:
				MCLRMASK_D32(ma, Z47_CTRL, Z47_CTRL_SW_ERR);
				break;
			case 1:
				MSETMASK_D32(ma, Z47_CTRL, Z47_CTRL_SW_ERR);
				break;
			default:
				DBGWRT_ERR((DBH, "*** %s(WDOG_ERR_PIN): illegal pattern 0x%x\n",
					func, value));
				return ERR_LL_ILL_PARAM;
			}
			break;
					
		/*--------------------------+
		|  (unknown)                |
		+--------------------------*/
		default:
			error = ERR_LL_UNK_CODE;
	}

	return (error);
}

/****************************** Z47_GetStat *********************************/
/** Get the driver status
 *
 *  The driver supports \ref getstat_setstat_codes "these status codes"
 *  in addition to the standard codes (see mdis_api.h).
 *
 *  \param llHdl             \IN  low-level handle
 *  \param code              \IN  \ref getstat_setstat_codes "status code"
 *  \param ch                \IN  current channel
 *  \param value32_or_64P    \IN  pointer to block data structure (M_SG_BLOCK) for
 *                                block status codes
 *  \param value32_or_64P    \OUT data pointer or pointer to block data structure
 *                                (M_SG_BLOCK) for block status codes
 *
 *  \return                  \c 0 on success or error code
 */
static int32 Z47_GetStat(
	LL_HANDLE   *llHdl,
	int32       code,
	int32       ch,
	INT32_OR_64 *value32_or_64P
)
{
	int32 *valueP = (int32*)value32_or_64P;		/* pointer to 32bit value */
	INT32_OR_64 *value64P = value32_or_64P;		/* stores 32/64bit pointer */
	MACCESS ma = llHdl->ma;
	int32 error = ERR_SUCCESS;
	int32 read;

	DBGWRT_1((DBH, "LL - Z47_GetStat: ch=%d code=0x%04x\n", ch, code));

	switch (code) {
		/*--------------------------+
		|  debug level              |
		+--------------------------*/
		case M_LL_DEBUG_LEVEL:
			*valueP = llHdl->dbgLevel;
			break;
		/*--------------------------+
		|  number of channels       |
		+--------------------------*/
		case M_LL_CH_NUMBER:
			*valueP = CH_NUMBER;
			break;
		/*--------------------------+
		|  channel direction        |
		+--------------------------*/
		case M_LL_CH_DIR:
			*valueP = M_CH_INOUT;
			break;
		/*--------------------------+
		|  channel length [bits]    |
		+--------------------------*/
		case M_LL_CH_LEN:
			*valueP = 1;
			break;
		/*--------------------------+
		|  channel type info        |
		+--------------------------*/
		case M_LL_CH_TYP:
			*valueP = M_CH_BINARY;
			break;
		/*--------------------------+
		|  irq counter              |
		+--------------------------*/
		case M_LL_IRQ_COUNT:
			*valueP = llHdl->irqCount;
			break;
		/*--------------------------+
		|  ID PROM check enabled    |
		+--------------------------*/
		case M_LL_ID_CHECK:
			*valueP = 0;
			break;
		/*--------------------------+
		|  ident table pointer      |
		|  (treat as non-block!)    |
		+--------------------------*/
		case M_MK_BLK_REV_ID:
			*value64P = (INT32_OR_64)&llHdl->idFuncTbl;
			break;

		/*--------------------------+
		|  old WDOG codes           |
		+--------------------------*/
		case WDOG_TIME:
			/* IP core time [us], requested time [ms] */
			*valueP = (MREAD_D32(ma, Z47_MAXT)) / 1000;
			break;

		case WDOG_STATUS:
			read = MREAD_D32(ma, Z47_STAT);
			*valueP = (read & Z47_STAT_WDOG_CNT_STATUS) ? 1 : 0;
			break;

		case WDOG_SHOT:
			read = MREAD_D32(ma, Z47_STAT);
			read = (read & Z47_STAT_REASON_TOUT_MASK) >> Z47_STAT_REASON_TOUT_SHIFT;
			*valueP = ((read == Z47_REASON_MIN_TOUT) ||
					   (read == Z47_REASON_MAX_TOUT) ) ? 1 : 0;
			break;

		/*--------------------------+
		|  new WDOG codes           |
		+--------------------------*/
		case WDOG_TRIG_PAT:
			*valueP = MREAD_D32(llHdl->ma, Z47_CLR);
			break;

		case WDOG_TIME_MIN:
			/* IP core time [us], requested time [us] */
			*valueP = MREAD_D32(ma, Z47_MINT);
			break;

		case WDOG_TIME_MAX:
			/* IP core time [us], requested time [us] */
			*valueP = MREAD_D32(ma, Z47_MAXT);
			break;

		case WDOG_TIME_IRQ:
			/* IP core time [us], requested time [us] */
			*valueP = MREAD_D32(ma, Z47_IRQT);
			break;

		case WDOG_OUT_PIN:
			read = MREAD_D32(ma, Z47_CTRL);
			*valueP = (read & Z47_CTRL_SET_WDOG_TOUT) ? 1 : 0;
			break;

		case WDOG_OUT_REASON:
			read = MREAD_D32(ma, Z47_STAT);
			*valueP = (read & Z47_STAT_REASON_TOUT_MASK) >> Z47_STAT_REASON_TOUT_SHIFT;
			break;

		case WDOG_IRQ_PIN:
			read = MREAD_D32(ma, Z47_CTRL);
			*valueP = (read & Z47_CTRL_SET_WDOG_IRQ) ? 1 : 0;
			break;

		case WDOG_IRQ_REASON:
			read = MREAD_D32(ma, Z47_STAT);
			*valueP = (read & Z47_STAT_REASON_IRQ_MASK) >> Z47_STAT_REASON_IRQ_SHIFT;
			break;

		case WDOG_ERR_PIN:
			read = MREAD_D32(ma, Z47_CTRL);
			*valueP = (read & Z47_CTRL_SW_ERR) ? 1 : 0;
			break;
			
		/*--------------------------+
		|  (unknown)                |
		+--------------------------*/
		default:
			error = ERR_LL_UNK_CODE;
	}

	return (error);
}

/******************************* Z47_BlockRead ******************************/
/** Read a data block from the device
*
*  The function is not supported and always returns an ERR_LL_ILL_FUNC error.
*
*  \param llHdl       \IN  low-level handle
 *  \param ch          \IN  current channel
 *  \param buf         \IN  data buffer
 *  \param size        \IN  data buffer size
 *  \param nbrRdBytesP \OUT number of read bytes
 *
 *  \return            \c 0 on success or error code
 */
static int32 Z47_BlockRead(
	LL_HANDLE *llHdl,
	int32     ch,
	void      *buf,
	int32     size,
	int32     *nbrRdBytesP
)
{
	DBGWRT_1((DBH, "LL - Z47_BlockRead: ch=%d, size=%d\n", ch, size));

	/* return number of read bytes */
	*nbrRdBytesP = 0;

	return (ERR_LL_ILL_FUNC);
}

/****************************** Z47_BlockWrite *****************************/
/** Write a data block from the device
*
*  The function is not supported and always returns an ERR_LL_ILL_FUNC error.
*
*  \param llHdl       \IN  low-level handle
 *  \param ch          \IN  current channel
 *  \param buf         \IN  data buffer
 *  \param size        \IN  data buffer size
 *  \param nbrWrBytesP \OUT number of written bytes
 *
 *  \return            \c 0 on success or error code
 */
static int32 Z47_BlockWrite(
	LL_HANDLE *llHdl,
	int32     ch,
	void      *buf,
	int32     size,
	int32     *nbrWrBytesP
)
{
	DBGWRT_1((DBH, "LL - Z47_BlockWrite: ch=%d, size=%d\n", ch, size));

	/* return number of written bytes */
	*nbrWrBytesP = 0;

	return (ERR_LL_ILL_FUNC);
}

/****************************** Z47_Irq ************************************/
/** Interrupt service routine
 *
 *  The interrupt is triggered, when a configured IRQ timeout (see #WDOG_TIME_IRQ)
 *  is reached. The application will be informed about the interrupt, if
 *  a signal was installed (see #WDOG_IRQ_SIGSET).
 *
 *  If the driver can detect the interrupt's cause it returns
 *  LL_IRQ_DEVICE or LL_IRQ_DEV_NOT, otherwise LL_IRQ_UNKNOWN.
 *  
 *  For MSI(x) it is necessary to disable all IRQs and enable them again
 *  at the end of the ISR.
 *
 *  \param llHdl       \IN  low-level handle
 *  \return LL_IRQ_DEVICE   irq caused by device
 *          LL_IRQ_DEV_NOT  irq not caused by device
 *          LL_IRQ_UNKNOWN  unknown
 */
static int32 Z47_Irq(
	LL_HANDLE *llHdl
)
{
	if (MREAD_D32(llHdl->ma, Z47_STAT) & Z47_STAT_WDOG_IRQ_STATUS){

		IDBGWRT_1((DBH, ">>> LL - Z47_Irq\n"));

		/* clear interrupt */
		MSETMASK_D32(llHdl->ma, Z47_STAT, Z47_STAT_WDOG_IRQ_STATUS);
		
		/* if requested send signal to application */
		if (llHdl->sigHdl)
			OSS_SigSend(OSH, llHdl->sigHdl);
		
		llHdl->irqCount++;

		return (LL_IRQ_DEVICE);
	}

	return (LL_IRQ_DEV_NOT);
}

/****************************** Z47_Info ***********************************/
/** Get information about hardware and driver requirements
 *
 *  The following info codes are supported:
 *
 * \code
 *  Code                      Description
 *  ------------------------  -----------------------------
 *  LL_INFO_HW_CHARACTER      hardware characteristics
 *  LL_INFO_ADDRSPACE_COUNT   nr of required address spaces
 *  LL_INFO_ADDRSPACE         address space information
 *  LL_INFO_IRQ               interrupt required
 *  LL_INFO_LOCKMODE          process lock mode required
 * \endcode
 *
 *  The LL_INFO_HW_CHARACTER code returns all address and
 *  data modes (ORed) which are supported by the hardware
 *  (MDIS_MAxx, MDIS_MDxx).
 *
 *  The LL_INFO_ADDRSPACE_COUNT code returns the number
 *  of address spaces used by the driver.
 *
 *  The LL_INFO_ADDRSPACE code returns information about one
 *  specific address space (MDIS_MAxx, MDIS_MDxx). The returned
 *  data mode represents the widest hardware access used by
 *  the driver.
 *
 *  The LL_INFO_IRQ code returns whether the driver supports an
 *  interrupt routine (TRUE or FALSE).
 *
 *  The LL_INFO_LOCKMODE code returns which process locking
 *  mode the driver needs (LL_LOCK_xxx).
 *
 *  \param infoType    \IN  info code
 *  \param ...         \IN  argument(s)
 *
 *  \return            \c 0 on success or error code
 */
static int32 Z47_Info(
	int32 infoType,
	...
)
{
	int32   error = ERR_SUCCESS;
	va_list argptr;

	va_start(argptr, infoType);

	switch (infoType) {
		/*-------------------------------+
		|  hardware characteristics      |
		|  (all addr/data modes ORed)    |
		+-------------------------------*/
		case LL_INFO_HW_CHARACTER:
		{
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);
		
			*addrModeP = MDIS_MA08;
			*dataModeP = MDIS_MD08 | MDIS_MD16;
			break;
		}
		/*-------------------------------+
		|  nr of required address spaces |
		|  (total spaces used)           |
		+-------------------------------*/
		case LL_INFO_ADDRSPACE_COUNT:
		{
			u_int32 *nbrOfAddrSpaceP = va_arg(argptr, u_int32*);
		
			*nbrOfAddrSpaceP = ADDRSPACE_COUNT;
			break;
		}
		/*-------------------------------+
		|  address space type            |
		|  (widest used data mode)       |
		+-------------------------------*/
		case LL_INFO_ADDRSPACE:
		{
			u_int32 addrSpaceIndex = va_arg(argptr, u_int32);
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);
			u_int32 *addrSizeP = va_arg(argptr, u_int32*);
		
			if (addrSpaceIndex >= ADDRSPACE_COUNT)
				error = ERR_LL_ILL_PARAM;
			else {
				*addrModeP = MDIS_MA08;
				*dataModeP = MDIS_MD16;
				*addrSizeP = ADDRSPACE_SIZE;
			}

			break;
		}
		/*-------------------------------+
		|  interrupt required            |
		+-------------------------------*/
		case LL_INFO_IRQ:
		{
			u_int32 *useIrqP = va_arg(argptr, u_int32*);

			*useIrqP = USE_IRQ;
			break;
		}
		/*-------------------------------+
		|  process lock mode             |
		+-------------------------------*/
		case LL_INFO_LOCKMODE:
		{
			u_int32 *lockModeP = va_arg(argptr, u_int32*);

			*lockModeP = LL_LOCK_CALL;
			break;
		}
		/*-------------------------------+
		|  (unknown)                     |
		+-------------------------------*/
		default:
			error = ERR_LL_ILL_PARAM;
	}

	va_end(argptr);

	return (error);
}

/******************************** Ident ************************************/
/** Return ident string
 *
 *  \return            pointer to ident string
 */
static char* Ident(void)
{
	return ("Z47 - Z47 low level driver: $Id: z47_drv.c,v $");
}

/********************************* Cleanup *********************************/
/** Close all handles, free memory and return error code
 *
 *  \warning The low-level handle is invalid after this function is called.
 *
 *  \param llHdl      \IN  low-level handle
 *  \param retCode    \IN  return value
 *
 *  \return           \IN  retCode
 */
static int32 Cleanup(
	LL_HANDLE *llHdl,
	int32     retCode
)
{
	/*------------------------------+
	|  close handles                |
	+------------------------------*/
	/* clean up desc */
	if (llHdl->descHdl)
		DESC_Exit(&llHdl->descHdl);

	/* clean up debug */
	DBGEXIT((&DBH));

	/*------------------------------+
	|  free memory                  |
	+------------------------------*/
	/* free my handle */
	OSS_MemFree(llHdl->osHdl, (int8*)llHdl, llHdl->memAlloc);

	/*return error code */
	return (retCode);
}

/********************************* WdogTrig *********************************/
/** Trigger watchdog
*
*   Z47 IP core requires an alternating 0x5A/0xA5 pattern.
*
*  \param llHdl      \IN  low-level handle
*  \param pat        \IN  trigger pattern
*
*  \return           \IN  retCode
*/
static int32 WdogTrig(
	LL_HANDLE *llHdl,
	int32     pat
)
{
	int32 error=ERR_SUCCESS;
	int32 read, val;

	/* watchdog not enabled ? */
	if (llHdl->wdStart == 0) {
		return ERR_LL_DEV_NOTRDY;
	}

	/* read last written pattern */
	read = MREAD_D32(llHdl->ma, Z47_CLR);

	/* pattern from user */
	if (pat) {
		/* illegal pattern? */
		if ((pat != WDOG_TRIGPAT(0)) &&
			(pat != WDOG_TRIGPAT(1))) {
			DBGWRT_ERR((DBH, "*** LL - Z17_SetStat(WDOG_TRIG_PAT): illegal pattern 0x%x\n", pat));
			return ERR_LL_ILL_PARAM;
		}

		/* not the first trigger? */
		if (read != 0) {
			/* repeated pattern? */
			if (pat == read) {
				DBGWRT_ERR((DBH, "*** LL - Z17_SetStat(WDOG_TRIG_PAT): repeated pattern 0x%x\n", pat));
				return ERR_LL_ILL_PARAM;
			}
		}
		val = pat;
	}
	/* no pattern given */
	else {
		/* first trigger? */
		if (read == 0)
			read = WDOG_TRIGPAT(0);

		/* compute new pattern from read pattern */
		val =  (read & 0x0f) << 4;
		val |= (read & 0xf0) >> 4;
	}

	/* write trigger pattern */
	MWRITE_D32(llHdl->ma, Z47_CLR, val);

	return (error);
}
