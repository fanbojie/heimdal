/*
 * Copyright (c) 1995, 1996 Kungliga Tekniska H�gskolan (Royal Institute
 * of Technology, Stockholm, Sweden).
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Kungliga Tekniska
 *      H�gskolan and its contributors.
 * 
 * 4. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* $Id$ */

#include <stdlib.h>

enum {OTPKEYSIZE = 8};

typedef unsigned char OtpKey[OTPKEYSIZE];

#define OTP_MIN_PASSPHRASE 10
#define OTP_MAX_PASSPHRASE 63

#define OTP_USER_TIMEOUT   60
#define OTP_DB_TIMEOUT     60

#define OTP_HEXPREFIX "hex:"
#define OTP_WORDPREFIX "word:"

typedef enum { ALG_MD4, ALG_MD5, ALG_SHA } OtpAlgID;

typedef struct {
  OtpAlgID id;
  char *name;
  int hashsize;
  int (*hash)(char *s, size_t len, unsigned char *res);
  int (*init)(OtpKey key, char *pwd, char *seed);
  int (*next)(OtpKey key);
} OtpAlgorithm;

typedef struct {
  char *user;
  OtpAlgorithm *alg;
  unsigned n;
  char seed[17];
  OtpKey key;
} OtpContext;

OtpAlgorithm *otp_find_alg (char *name);
void otp_print_stddict (OtpKey key, char *str);
void otp_print_hex (OtpKey key, char *str);
unsigned otp_checksum (OtpKey key);
int otp_parse_hex (OtpKey key, char *);
int otp_parse_stddict (OtpKey key, char *);
int otp_parse_altdict (OtpKey key, char *, OtpAlgorithm *);
int otp_parse (OtpKey key, char *, OtpAlgorithm *);
int otp_challenge (OtpContext *ctx, char *user, char *str, size_t len);
int otp_verify_user (OtpContext *ctx, char *passwd);
int otp_verify_user_1 (OtpContext *ctx, char *passwd);

void *otp_db_open (void);
void otp_db_close (void *);
int otp_put (void *, OtpContext *ctx);
int otp_get (void *, OtpContext *ctx);
