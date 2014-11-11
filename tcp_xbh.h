/*----------------------------------------------------------------------------
 Copyright:      Christian Wenzel-Benner  mailto: c-w-b@web.de
 Author:         Christian Wenzel-Benner
 Remarks:
 known Problems: none
 Version:        25.04.2009
 Description:    1st try: echo rec'd UDP data back to sender IP, port 22594 (XB)

 Dieses Programm ist freie Software. Sie k�nnen es unter den Bedingungen der
 GNU General Public License, wie von der Free Software Foundation ver�ffentlicht,
 weitergeben und/oder modifizieren, entweder gem�� Version 2 der Lizenz oder
 (nach Ihrer Option) jeder sp�teren Version.

 Die Ver�ffentlichung dieses Programms erfolgt in der Hoffnung,
 da� es Ihnen von Nutzen sein wird, aber OHNE IRGENDEINE GARANTIE,
 sogar ohne die implizite Garantie der MARKTREIFE oder der VERWENDBARKEIT
 F�R EINEN BESTIMMTEN ZWECK. Details finden Sie in der GNU General Public License.

 Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
 Programm erhalten haben.
 Falls nicht, schreiben Sie an die Free Software Foundation,
 Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
------------------------------------------------------------------------------*/
#ifndef _TCPXBH_H
#define _TCPXBH_H

#include "avrlibtypes.h"


#define TCP_XBH_PORT		22595

void tcp_xbh_init(void);
void tcp_xbh_get(unsigned char);


#endif
