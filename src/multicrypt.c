/* src/multicrypt.c - Divers outils de cryptage de hosts
 * Copyright (C) 2004 ircdreams.org
 *
 * contact: bugs@ircdreams.org
 * site web: http://ircdreams.org
 *
 * Services pour serveur IRC. Supporté sur IrcDreams V.2
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
 * $Id: multicrypt.c,v 1.5 2006/03/29 17:34:21 bugs Exp $
 */

#include <ctype.h>

#include <fcntl.h>
#include <unistd.h> 

#include "main.h"
#include "debug.h"
#include "cs_cmds.h"
#include "admin_manage.h"
#include "hash.h"
#include "outils.h"
#include "checksum.h"
#include "config.h"

int check_if_ipmask(const char *mask)
{
  int has_digit = 0;
  int digitcount = 0;
  const char *p;

  for (p = mask; *p; ++p)
    if (*p != '*' && *p != '?' && *p != '.' && *p != '/')
    {
      if (!isdigit(*p) || digitcount > 3)
        return 0;
          digitcount++;
      has_digit = -1;
    }
        else {
         digitcount = 0;
        }
  return has_digit;
}

/* PirO hp
* basé sur une alghorthime en SHA
* (1028 bits * 128 bits) source code issu
* du daemon : "shacrypt"
* (dispo sur sourceforge.net)
* principe :
* HostNonCrypté -> hachage -> conversion en pile mémoire
* -> cryptage -> conversion inverse -> HostCrypté
* la clé de hashage est éffectuée en fontion de l'host de maniére
* a obtenir toujour la meme clé pour la meme host , donc le meme cryptage.
*/

#define SIZEOF_INT 4

/* The number of bytes in a long.  */
#define SIZEOF_LONG 4

/* The number of bytes in a short.  */
#define SIZEOF_SHORT 2

/* The number of bytes in a size_t.  */
#define SIZEOF_SIZE_T 4

#if SIZEOF_INT == 4
  typedef unsigned int uint32t;
#else
# if SIZEOF_LONG == 4
   typedef unsigned long uint32t;
# else
#  if SIZEOF_SHORT == 4
    typedef unsigned short uint32t;
#  else
#   error "unable to find 32-bit data type"
#  endif /* SHORT != 4 */
# endif /* LONG != 4 */
#endif /* INT != 4 */


#define SHA_DATASIZE    64
#define SHA_DATALEN     16
#define SHA_DIGESTSIZE  20
#define SHA_DIGESTLEN    10

#define f1(x,y,z)   ( z ^ ( x & ( y ^ z ) ) )           /* Rounds  0-19 */
#define f2(x,y,z)   ( x ^ y ^ z )                       /* Rounds 20-39 */
#define f3(x,y,z)   ( ( x & y ) | ( z & ( x | y ) ) )   /* Rounds 40-59 */
#define f4(x,y,z)   ( x ^ y ^ z )                       /* Rounds 60-79 */

#define K1  0x5A827999L                                 /* Rounds  0-19 */
#define K2  0x6ED9EBA1L                                 /* Rounds 20-39 */
#define K3  0x8F1BBCDCL                                 /* Rounds 40-59 */
#define K4  0xCA62C1D6L                                 /* Rounds 60-79 */

#define h0init  0x67452301L
#define h1init  0xEFCDAB89L
#define h2init  0x98BADCFEL
#define h3init  0x10325476L
#define h4init  0xC3D2E1F0L

#define ROTL(n,X)  ( ( (X) << (n) ) | ( (X) >> ( 32 - (n) ) ) )

#define expand(W,i) ( W[ i & 15 ] = \
                      ROTL( 1, ( W[ i & 15 ] ^ W[ (i - 14) & 15 ] ^ \
                                 W[ (i - 8) & 15 ] ^ W[ (i - 3) & 15 ] ) ) )

#define subRound(a, b, c, d, e, f, k, data) \
    ( e += ROTL( 5, a ) + f( b, c, d ) + k + data, b = ROTL( 30, b ) )

struct sha_ctx {
  uint32t digest[SHA_DIGESTLEN];  /* Message digest */
  uint32t count_l, count_h;       /* 64-bit block count */
  int index;                             /* index into buffer */
  char block[SHA_DATASIZE];     /* SHA data buffer */
};

static void sha_transform(struct sha_ctx *ctx, uint32t *data)
{
  uint32t A, B, C, D, E;     /* Local vars */

  /* Set up first buffer and local data buffer */
  A = ctx->digest[0];
  B = ctx->digest[1];
  C = ctx->digest[2];
  D = ctx->digest[3];
  E = ctx->digest[4];

  /* Heavy mangling, in 4 sub-rounds of 20 interations each. */
  subRound( A, B, C, D, E, f1, K1, data[ 0] );
  subRound( E, A, B, C, D, f1, K1, data[ 1] );
  subRound( D, E, A, B, C, f1, K1, data[ 2] );
  subRound( C, D, E, A, B, f1, K1, data[ 3] );
  subRound( B, C, D, E, A, f1, K1, data[ 4] );
  subRound( A, B, C, D, E, f1, K1, data[ 5] );
  subRound( E, A, B, C, D, f1, K1, data[ 6] );
  subRound( D, E, A, B, C, f1, K1, data[ 7] );
  subRound( C, D, E, A, B, f1, K1, data[ 8] );
  subRound( B, C, D, E, A, f1, K1, data[ 9] );
  subRound( A, B, C, D, E, f1, K1, data[10] );
  subRound( E, A, B, C, D, f1, K1, data[11] );
  subRound( D, E, A, B, C, f1, K1, data[12] );
  subRound( C, D, E, A, B, f1, K1, data[13] );
  subRound( B, C, D, E, A, f1, K1, data[14] );
  subRound( A, B, C, D, E, f1, K1, data[15] );
  subRound( E, A, B, C, D, f1, K1, expand( data, 16 ) );
  subRound( D, E, A, B, C, f1, K1, expand( data, 17 ) );
  subRound( C, D, E, A, B, f1, K1, expand( data, 18 ) );
  subRound( B, C, D, E, A, f1, K1, expand( data, 19 ) );

  subRound( A, B, C, D, E, f2, K2, expand( data, 20 ) );
  subRound( E, A, B, C, D, f2, K2, expand( data, 21 ) );
  subRound( D, E, A, B, C, f2, K2, expand( data, 22 ) );
  subRound( C, D, E, A, B, f2, K2, expand( data, 23 ) );
  subRound( B, C, D, E, A, f2, K2, expand( data, 24 ) );
  subRound( A, B, C, D, E, f2, K2, expand( data, 25 ) );
  subRound( E, A, B, C, D, f2, K2, expand( data, 26 ) );
  subRound( D, E, A, B, C, f2, K2, expand( data, 27 ) );
  subRound( C, D, E, A, B, f2, K2, expand( data, 28 ) );
  subRound( B, C, D, E, A, f2, K2, expand( data, 29 ) );
  subRound( A, B, C, D, E, f2, K2, expand( data, 30 ) );
  subRound( E, A, B, C, D, f2, K2, expand( data, 31 ) );
  subRound( D, E, A, B, C, f2, K2, expand( data, 32 ) );
  subRound( C, D, E, A, B, f2, K2, expand( data, 33 ) );
  subRound( B, C, D, E, A, f2, K2, expand( data, 34 ) );
  subRound( A, B, C, D, E, f2, K2, expand( data, 35 ) );
  subRound( E, A, B, C, D, f2, K2, expand( data, 36 ) );
  subRound( D, E, A, B, C, f2, K2, expand( data, 37 ) );
  subRound( C, D, E, A, B, f2, K2, expand( data, 38 ) );
  subRound( B, C, D, E, A, f2, K2, expand( data, 39 ) );

  subRound( A, B, C, D, E, f3, K3, expand( data, 40 ) );
  subRound( E, A, B, C, D, f3, K3, expand( data, 41 ) );
  subRound( D, E, A, B, C, f3, K3, expand( data, 42 ) );
  subRound( C, D, E, A, B, f3, K3, expand( data, 43 ) );
  subRound( B, C, D, E, A, f3, K3, expand( data, 44 ) );
  subRound( A, B, C, D, E, f3, K3, expand( data, 45 ) );
  subRound( E, A, B, C, D, f3, K3, expand( data, 46 ) );
  subRound( D, E, A, B, C, f3, K3, expand( data, 47 ) );
  subRound( C, D, E, A, B, f3, K3, expand( data, 48 ) );
  subRound( B, C, D, E, A, f3, K3, expand( data, 49 ) );
  subRound( A, B, C, D, E, f3, K3, expand( data, 50 ) );
  subRound( E, A, B, C, D, f3, K3, expand( data, 51 ) );
  subRound( D, E, A, B, C, f3, K3, expand( data, 52 ) );
  subRound( C, D, E, A, B, f3, K3, expand( data, 53 ) );
  subRound( B, C, D, E, A, f3, K3, expand( data, 54 ) );
  subRound( A, B, C, D, E, f3, K3, expand( data, 55 ) );
  subRound( E, A, B, C, D, f3, K3, expand( data, 56 ) );
  subRound( D, E, A, B, C, f3, K3, expand( data, 57 ) );
  subRound( C, D, E, A, B, f3, K3, expand( data, 58 ) );
  subRound( B, C, D, E, A, f3, K3, expand( data, 59 ) );

  subRound( A, B, C, D, E, f4, K4, expand( data, 60 ) );
  subRound( E, A, B, C, D, f4, K4, expand( data, 61 ) );
  subRound( D, E, A, B, C, f4, K4, expand( data, 62 ) );
  subRound( C, D, E, A, B, f4, K4, expand( data, 63 ) );
  subRound( B, C, D, E, A, f4, K4, expand( data, 64 ) );
  subRound( A, B, C, D, E, f4, K4, expand( data, 65 ) );
  subRound( E, A, B, C, D, f4, K4, expand( data, 66 ) );
  subRound( D, E, A, B, C, f4, K4, expand( data, 67 ) );
  subRound( C, D, E, A, B, f4, K4, expand( data, 68 ) );
  subRound( B, C, D, E, A, f4, K4, expand( data, 69 ) );
  subRound( A, B, C, D, E, f4, K4, expand( data, 70 ) );
  subRound( E, A, B, C, D, f4, K4, expand( data, 71 ) );
  subRound( D, E, A, B, C, f4, K4, expand( data, 72 ) );
  subRound( C, D, E, A, B, f4, K4, expand( data, 73 ) );
  subRound( B, C, D, E, A, f4, K4, expand( data, 74 ) );
  subRound( A, B, C, D, E, f4, K4, expand( data, 75 ) );
  subRound( E, A, B, C, D, f4, K4, expand( data, 76 ) );
  subRound( D, E, A, B, C, f4, K4, expand( data, 77 ) );
  subRound( C, D, E, A, B, f4, K4, expand( data, 78 ) );
  subRound( B, C, D, E, A, f4, K4, expand( data, 79 ) );

  /* Build message digest */
  ctx->digest[0] += A;
  ctx->digest[1] += B;
  ctx->digest[2] += C;
  ctx->digest[3] += D;
  ctx->digest[4] += E;
}

#define EXTRACT_UCHAR(p)  (*(unsigned char *)(p))
#define STRING2INT(s) ((((((EXTRACT_UCHAR(s) << 8)    \
                         | EXTRACT_UCHAR(s+1)) << 8)  \
                         | EXTRACT_UCHAR(s+2)) << 8)  \
                         | EXTRACT_UCHAR(s+3))

static void sha_block(struct sha_ctx *ctx, char *block)
{
  uint32t data[SHA_DATALEN];
  int i;

  /* Update block count */
  if (!++ctx->count_l)
    ++ctx->count_h;

  /* Endian independent conversion */
  for (i = 0; i<SHA_DATALEN; i++, block += 4)
    data[i] = STRING2INT(block);

  sha_transform(ctx, data);
}

static void make_sha(char *buffer, uint32t len, uint32t *s)
{
  struct sha_ctx ctxbuf = {
    {h0init, h1init, h2init, h3init, h4init},
    0, 0, 0,
  };
  int i, words;
  uint32t data[SHA_DATALEN];

  while (len >= SHA_DATASIZE)
    {
      sha_block(&ctxbuf, buffer);
      buffer += SHA_DATASIZE;
      len -= SHA_DATASIZE;
    }
  if ((ctxbuf.index = len))     /* This assignment is intended */
    /* Buffer leftovers */
    memmove(ctxbuf.block, buffer, len);

  /*** sha_final ***/

  i = ctxbuf.index;
  /* Set the first char of padding to 0x80.  This is safe since there is
     always at least one byte free */
  ctxbuf.block[i++] = 0x80;

  /* Fill rest of word */
 for( ; i & 3; i++)
    ctxbuf.block[i] = 0;

  /* i is now a multiple of the word size 4 */
  words = i >> 2;
  for (i = 0; i < words; i++)
    data[i] = STRING2INT(ctxbuf.block + 4*i);

  if (words > (SHA_DATALEN-2))
    { /* No room for length in this block. Process it and
       * pad with another one */
      for (i = words ; i < SHA_DATALEN; i++)
        data[i] = 0;
      sha_transform(&ctxbuf, data);
      for (i = 0; i < (SHA_DATALEN-2); i++)
        data[i] = 0;
    }
  else
    for (i = words ; i < SHA_DATALEN - 2; i++)
      data[i] = 0;
  /* Theres 512 = 2^9 bits in one block */
  data[SHA_DATALEN-2] = (ctxbuf.count_h << 9) | (ctxbuf.count_l >> 23);
  data[SHA_DATALEN-1] = (ctxbuf.count_l << 9) | (ctxbuf.index << 3);
  sha_transform(&ctxbuf, data);

  /*
   * we don't want to use the digest as 20x8-bit character array, but
   * as 5x32-bit integer array, so we use this to make it endian-safe
   * for *our* purposes
   */
  memcpy(s, ctxbuf.digest, SHA_DIGESTSIZE);
}

#define KEYSIZE 128
unsigned char hostprotkey[KEYSIZE];
/* generates random data and fills the key buffer */
void make_hostprotkey()
{
  int fd;
  int i;
  fd = open("/dev/urandom", O_RDONLY);
  if (fd >= 0)
  {  /* we have a good /dev/urandom, read from it */
    if (read(fd, hostprotkey, KEYSIZE) == KEYSIZE)
    {
      close(fd);
      return;
    }
    /* there was an error reading, fall back to random() */
    close(fd);
  }
  /* i hope the following satisfies different implementations of random() */
  for (i = 0; i < KEYSIZE; i++)
    hostprotkey[i] = (unsigned char) ((random() & 0xff0000) >> 16);
}

static void make_digest_len(const char *src, uint32t *digest, int len)
{
  char buf[KEYSIZE + HOSTLEN];
  int i, k;

  i = KEYSIZE / 2;
  k = KEYSIZE - i;
  memcpy(buf, hostprotkey, i);
  memcpy(buf + i, src, len);
  memcpy(buf + i + len, hostprotkey + i, k);

  make_sha(buf, i + len + k, digest);
}

static void make_digest(const char *src, uint32t *digest)
{
  make_digest_len(src, digest, strlen(src));
}

static uint32t make_sum_from_digest(uint32t *digest)
{
  uint32t sum;

  sum = digest[0] ^ digest[1] ^ digest[2] ^ digest[3] ^ digest[4];
  /* instead of a simple (sum %= 0x7ffff), we use.. */
  sum = (sum & 0xFFFFFFFF) ^ ((sum & 0x4FFFFFFF) >> 13);
 /* sum = (sum & 0xFFFFFFFFFFF) ^ ((sum & 0x4FFFFFFF) >> 13); */
  /* this will give a number in the range 0 - 524287 */
  return sum;
}

char *makeHash(const char *mask)
{
static char final[HOSTLEN+1];

  uint32t digest[SHA_DIGESTLEN];
  uint32t sum;

  make_digest(mask, digest);
  sum = make_sum_from_digest(digest);

  sprintf(final, "%lu", (unsigned long) sum);
  return final;
}

char *hostprotcrypt(const char *mask)
{
  char host[HOSTLEN+1], * p, *crypted, domaine[HOSTLEN+1];
  static char final[HOSTLEN+1];
  int i, j, nbp=0;

  strcpy(host, mask);
        /* stage 1 : mask IP -> int */
  if(check_if_ipmask(host)) {
                char *prec = host;
                int num = 0;
                while((p = strchr(prec,'.')) != NULL) {
                        *p = '\0';
                        num <<= 8;
                        num |= (atoi(prec) & 0xFF);
                        prec = p + 1;
                }
                num <<= 8;
                num |= (atoi(prec) & 0xFF);


                /* stage 2 : IP int -> string */
                {
                        char toCrypt[9];
                        int i;
                        for(i = 0; i < 8; i++) {
                                toCrypt[i] = (num & 0xF) + 32;
                                num >>= 4;
                        }
                        toCrypt[8] = '\0';


                /* stage 3 string -> crypt */
                        {
                        char *crypted = makeHash(toCrypt);

                /* stage 4 crypt -> look like an adresse*/
                        {
				strcpy(final, crypted);
                                strcat(final,".ip");
                        }
                }
        }
  } else {
	crypted = (char *)makeHash(host);
        strcpy(final, crypted);
        strcat(final,".");

	if(GetConf(CF_SHA2))
	{
		/* merci Progs pour ton aide ! */
  	        i=strlen(host)-1;
		for(;i>=0 && (host[i] != '.' || ++nbp<2); --i);
  	        j=strlen(host)-i-1;
  	        i=strlen(host)-1;
  	        domaine[j--]='\0';

  	        for(; j>=0; i--, j--) domaine[j] = host[i];
  	 
  	        strcat(final,domaine);
	} else {
        	p=strrchr(host,'.');
		if(p) {
        		*p = '\0';
                	strcat(final,p + 1);
		}
		else strcat(final, "Crypted");
	}
  }
        return final;
}
/* fin du cryptage SHA */

void hostprot(const char *host, char *buf)
{
	if(GetConf(CF_CRYPT_MD2_MD5))
	{
		UINT4 sum, digest[4];
		int ip = is_ip(host);
		char key1[HOSTLEN + 1], *key2 = strchr(host, '.'), *key = ip ? key2 : key1;

		if(!key2)
		{
			Strncpy(buf, host, HOSTLEN);
			return;
		}

		Strncpy(key1, host, key2 - host);

		if(!host[0] % 2)
		{
			MD2_CTX context;
			MD2Init(&context);
			MD2Update(&context, (unsigned char *) key, strlen(key));
			MD2Update(&context, (unsigned char *) host, strlen(host));
			MD2Final(digest, &context);
		}
		else
		{
			MD5_CTX context;
			MD5Init(&context);
			MD5Update(&context, (unsigned char *) key, strlen(key));
			MD5Update(&context, (unsigned char *) host, strlen(host));
			MD5Final(digest, &context);
		}

		sum = digest[0] + digest[1] + digest[2] + digest[3];

		if(!ip) snprintf(buf, HOSTLEN+1, "%X%s", sum, key2);
		else snprintf(buf, HOSTLEN+1, "%s.%X", key1, sum);
	}
	else strcpy(buf,hostprotcrypt(host)); /* sinon c'est le SHA */
}
