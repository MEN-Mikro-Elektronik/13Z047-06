/***********************  I n c l u d e  -  F i l e  ************************/
/*!
 *        \file  z47_drv.h
 *
 *      \author  dieter.pfeuffer@men.de
 *        $Date: $
 *    $Revision: 2.6 $
 *
 *       \brief  Header file for Z47 driver containing
 *               Z47 function prototypes
 *               Note: driver uses status codes from wdog.h
 *
 *    \switches  _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: z47_drv.h,v $
 *---------------------------------------------------------------------------
 * (c) Copyright 2016 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _Z47_DRV_H
#define _Z47_DRV_H

#ifdef __cplusplus
	extern "C" {
#endif

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
#ifndef  Z47_VARIANT
  #define Z47_VARIANT    Z47
#endif

#define _Z47_GLOBNAME(var,name) var##_##name

#ifndef _ONE_NAMESPACE_PER_DRIVER_
  #define Z47_GLOBNAME(var,name)    _Z47_GLOBNAME(var,name)
#else
  #define Z47_GLOBNAME(var,name)    _Z47_GLOBNAME(Z47,name)
#endif

#define __Z47_GetEntry    Z47_GLOBNAME(Z47_VARIANT, GetEntry)

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
#ifdef _LL_DRV_
#ifndef _ONE_NAMESPACE_PER_DRIVER_
	extern void __Z47_GetEntry(LL_ENTRY* drvP);
#endif
#endif /* _LL_DRV_ */

/*-----------------------------------------+
|  BACKWARD COMPATIBILITY TO MDIS4         |
+-----------------------------------------*/
#ifndef U_INT32_OR_64
  /* we have an MDIS4 men_types.h and mdis_api.h included */
  /* only 32bit compatibility needed!                     */
  #define INT32_OR_64    int32
  #define U_INT32_OR_64  u_int32
  typedef INT32_OR_64    MDIS_PATH;
#endif /* U_INT32_OR_64 */

#ifdef __cplusplus
	}
#endif

#endif /* _Z47_DRV_H */
