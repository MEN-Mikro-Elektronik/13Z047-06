/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
*        \file  z47_doc.c
*
*      \author  dieter.pfeuffer@men.de
*
*      \brief   User documentation for Z47 device driver
*
*     Required: -
*
*     \switches -
*
*---------------------------------------------------------------------------
* Copyright 2016-2019, MEN Mikro Elektronik GmbH
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

/*! \mainpage
    This is the documentation of the MDIS5 low-level driver for the Z47 Watchdog
    IP  core.

	\n \section FuncDesc Functional Description

	\n \subsection IpCore IP Core

    The Z47 is a general purpose software watchdog device that can be used to
    supervise the state of a running software module. To do this, the software module
	has to trigger the watchdog within a defined time intervall. In an error case,
	the watchdog can trigger an output pin (e.g. to reset the CPU or switch into a
	save mode) or generate an IRQ (e.g. to inform an application about the error
	case).\n 
	\n
    Max Timeout:\n
    A free running counter with base 1us is incrementing until a configurable max
	timeout compare value is reached. If the software triggers the counter to restart
	counting from 0 before the compare value has reached, no error will be indicated.
	If the software is not able to trigger the counter and the compare value is reached,
	an error will be signalled (the output pin will be triggered).\n
	\n
    Min Timeout (Windowed Watchdog):\n
    Additionally, to the max timeout compare value, a min timeout compare value can be
	configured. If the trigger is within the range of minimum and maximum compare value,
	the counter will be cleared and restarts at 0. If the trigger comes before the min
	compare value has been reached, the software can be considered as faulty and the
	watchdog signals the error in the same way as when the max compare value is reached.\n
	\n
	IRQ Timeout:\n
	An IRQ compare value can be configured to trigger an interrupt if the counter reaches
	this timeout value.\n
	\n
	Additional Features:\n
	- To trigger the counter, an alternating byte value is used (to detect additionally
	  SW faults).
	- Output pin and IRQ pin can be set manually from the SW.
	- SW configurable error pin that can be used to	switch e.g. an error LED.
	- Status information (watchdog enabled/disable) can be queried.
	- Information about last output/IRQ pin trigger can be queried.


	\n \subsection Driver Driver

    The driver supports controlling of the Z47 Watchdog via M_setstat() and M_getstat(),
	see \ref getstat_setstat_codes.
    An interrupt can be used to inform applications via an MDIS signal about an error case.

    When the first path is opened to an Z47 device, the HW and the driver are being
    initialized.
    \n

    \n \section api_functions Supported API Functions

    <table border="0">
    <tr><td><b>API Function</b></td> <td><b>Functionality</b></td>   <td><b>Corresponding Low - Level Function</b></td></tr>
    <tr><td>M_open()</td>            <td>Open device</td>            <td>Z47_Init()</td></tr>
    <tr><td>M_close()</td>           <td>Close device</td>           <td>Z47_Exit()</td></tr>
    <tr><td>M_setstat()</td>         <td>Set device parameter</td>   <td>Z47_SetStat()</td></tr>
    <tr><td>M_getstat()</td>         <td>Get device parameter</td>   <td>Z47_GetStat()</td></tr>
    <tr><td>M_errstringTs()</td>     <td>Generate error message</td> <td>-</td></tr>
	<tr><td>-</td>                   <td>Interrupt Handling</td>     <td>Z47_Irq()</td></tr>
	</table>


    \n \section getstat_setstat_codes Getstat / Setstat codes

    The driver supports the following Getstat / Setstat codes (defined in wdog.h):
    - #WDOG_START
    - #WDOG_STOP
    - #WDOG_TRIG
    - #WDOG_TIME
    - #WDOG_STATUS
    - #WDOG_SHOT\n
    - #WDOG_RESET_CTRL\n
    - #WDOG_TRIG_PAT\n
    - #WDOG_TIME_MIN\n
    - #WDOG_TIME_MAX\n
    - #WDOG_TIME_IRQ\n
    - #WDOG_OUT_PIN\n
    - #WDOG_OUT_REASON\n
    - #WDOG_IRQ_PIN\n
    - #WDOG_IRQ_SIGSET\n
    - #WDOG_IRQ_SIGCLR\n
    - #WDOG_IRQ_REASON\n
    - #WDOG_ERR_PIN\n

    \n \section programs Overview of provided programs

    \subsection wdog_ctrl Watchdog control tool
    wdog_ctrl.c(see example section)

    \subsection wdog_simp Simple watchdog example program
    wdog_simp.c(see example section)

    \subsection wdog_test Watchdog test tool
    wdog_test.c(see example section)
*/

/** \example wdog_ctrl.c */
/** \example wdog_simp.c */
/** \example wdog_test.c */

/*! \page z47dummy MEN logo
\menimages
*/
