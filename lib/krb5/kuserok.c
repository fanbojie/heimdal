/*
 * Copyright (c) 1997 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden). 
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
 *      This product includes software developed by Kungliga Tekniska 
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

#include "krb5_locl.h"

RCSID("$Id$");

krb5_boolean
krb5_kuserok (krb5_context context,
	      krb5_principal principal,
	      const char *luser)
{
    char buf[BUFSIZ];
    struct passwd *pwd;
    FILE *f;
    char *realm;
    krb5_principal local_principal;
    krb5_error_code ret;
    krb5_boolean b;

    ret = krb5_get_default_realm (context, &realm);
    if (ret) {
	free (realm);
	return FALSE;
    }

    ret = krb5_build_principal (context,
				&local_principal,
				strlen(realm),
				realm,
				luser,
				NULL);
    free (realm);
    if (ret)
	return FALSE;

    b = krb5_principal_compare (context, principal, local_principal);
    krb5_free_principal (context, local_principal);
    if (b)
	return TRUE;

    pwd = getpwnam (luser);	/* XXX - Should use k_getpwnam? */
    if (pwd == NULL)
	return FALSE;
    snprintf (buf, sizeof(buf), "%s/.k5login", pwd->pw_dir);
    f = fopen (buf, "r");
    if (f == NULL)
	return FALSE;
    while (fgets (buf, sizeof(buf), f) != NULL) {
	krb5_principal tmp;

	if(buf[strlen(buf) - 1] == '\n')
	    buf[strlen(buf) - 1] = '\0';

	ret = krb5_parse_name (context, buf, &tmp);
	if (ret) {
	    fclose (f);
	    return FALSE;
	}
	b = krb5_principal_compare (context, principal, tmp);
	krb5_free_principal (context, tmp);
	if (b) {
	    fclose (f);
	    return TRUE;
	}
    }
    fclose (f);
    return FALSE;
}
