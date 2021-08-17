/* include/checksum.h - md5 & md2
 * Copyright (C) 2002-2004 Inter System
 *
 * contact: Progs@Inter-System.Net
 *          Cesar@Inter-System.Net
 *          kouak@kouak.org
 * site web: http://coderz.inter-system.net
 *
 * Services pour serveur IRC. Supporté sur IrcProgs et IrCoderZ
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * $Id: checksum.h,v 1.3 2004/09/17 23:23:25 romexzf Exp $
 */

#ifndef HAVEINC_checksum
#define HAVEINC_checksum

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT4 defines a four byte word */
typedef unsigned int UINT4;

typedef struct MD5Context {
  UINT4 state[4];   /* state (ABCD) */
  UINT4 count[2];   /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];     /* input buffer */
} MD5_CTX;

extern void MD5Init(MD5_CTX *);
extern void MD5Update(MD5_CTX *, unsigned char *, unsigned int);
extern void MD5Final(UINT4 *, MD5_CTX *);

#	ifdef HAVE_CRYPTHOST
/* Code pour cryptage md2 */
typedef struct {
  UINT4 state[16];                                         /* state */
  unsigned char checksum[16];                                   /* checksum */
  UINT4 count;                         /* number of bytes, modulo 16 */
  unsigned char buffer[16];                                 /* input buffer */
} MD2_CTX;

extern void MD2Init(MD2_CTX *);
extern void MD2Update(MD2_CTX *, unsigned char *, unsigned int);
extern void MD2Final(UINT4 *, MD2_CTX *);
#	endif /* HAVE_CRYPTHOST */

#endif /* HAVEINC_checksum */
