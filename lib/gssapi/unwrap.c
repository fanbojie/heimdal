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

#include "gssapi_locl.h"

RCSID("$Id$");

OM_uint32 gss_unwrap
           (OM_uint32 * minor_status,
            const gss_ctx_id_t context_handle,
            const gss_buffer_t input_message_buffer,
            gss_buffer_t output_message_buffer,
            int * conf_state,
            gss_qop_t * qop_state
           )
{
  u_char *p, *pad;
  size_t len;
  struct md5 md5;
  u_char hash[16], seq_data[8];
  des_key_schedule schedule;
  des_cblock key;
  des_cblock zero;
  int i;
  int32_t seq_number;
  size_t padlength;
  OM_uint32 ret;

  p = input_message_buffer->value;
  ret = gssapi_krb5_verify_header (&p,
				   input_message_buffer->length,
				   "\x02\x01");
  if (ret)
      return ret;

  if (memcmp (p, "\x00\x00", 2) != 0)
    return GSS_S_BAD_SIG;
  p += 2;
  if (memcmp (p, "\x00\x00", 2) != 0)
    return GSS_S_BAD_MIC;
  p += 2;
  if (memcmp (p, "\xff\xff", 2) != 0)
    return GSS_S_DEFECTIVE_TOKEN;
  p += 2;
  p += 16;

  len = p - (u_char *)input_message_buffer->value;

  /* decrypt data */

  memset (&zero, 0, sizeof(zero));
#if 0
  memcpy (&key, context_handle->auth_context->key.keyvalue.data,
	  sizeof(key));
#endif
  memcpy (&key, context_handle->auth_context->remote_subkey.keyvalue.data,
	  sizeof(key));
  for (i = 0; i < sizeof(key); ++i)
    key[i] ^= 0xf0;
  des_set_key (&key, schedule);
  des_cbc_encrypt ((des_cblock *)p,
		   (des_cblock *)p,
		   input_message_buffer->length - len,
		   schedule,
		   &zero,
		   DES_DECRYPT);

  memset (key, 0, sizeof(key));
  memset (schedule, 0, sizeof(schedule));

  /* check pad */

  pad = (char *)input_message_buffer->value + input_message_buffer->length - 1;
  padlength = *pad;

  for (i = padlength; i >= 0 && *pad == padlength; i--, pad--)
    ;
  if (i != 0)
    return GSS_S_BAD_MIC;

  md5_init (&md5);
  md5_update (&md5, p - 24, 8);
  md5_update (&md5, p, input_message_buffer->length - len);
  md5_finito (&md5, hash);

  memset (&zero, 0, sizeof(zero));
#if 0
  memcpy (&key, context_handle->auth_context->key.keyvalue.data,
	  sizeof(key));
#endif
  memcpy (&key, context_handle->auth_context->remote_subkey.keyvalue.data,
	  sizeof(key));
  des_set_key (&key, schedule);
  des_cbc_cksum ((des_cblock *)hash,
		 (des_cblock *)hash, sizeof(hash), schedule, &zero);
  if (memcmp (p - 8, hash, 8) != 0)
    return GSS_S_BAD_MIC;

  /* verify sequence number */
  
  krb5_auth_getremoteseqnumber (gssapi_krb5_context,
				context_handle->auth_context,
				&seq_number);
  seq_data[0] = (seq_number >> 0)  & 0xFF;
  seq_data[1] = (seq_number >> 8)  & 0xFF;
  seq_data[2] = (seq_number >> 16) & 0xFF;
  seq_data[3] = (seq_number >> 24) & 0xFF;
  memset (seq_data + 4,
	  (context_handle->more_flags & LOCAL) ? 0xFF : 0,
	  4);

  p -= 16;
  des_set_key (&key, schedule);
  des_cbc_encrypt ((des_cblock *)p, (des_cblock *)p, 8,
		   schedule, (des_cblock *)hash, DES_DECRYPT);

  memset (key, 0, sizeof(key));
  memset (schedule, 0, sizeof(schedule));

  if (memcmp (p, seq_data, 8) != 0) {
    return GSS_S_BAD_MIC;
  }

  krb5_auth_setremoteseqnumber (gssapi_krb5_context,
				context_handle->auth_context,
				++seq_number);

  /* copy out data */

  output_message_buffer->length = input_message_buffer->length
    - len - 8 - padlength;
  output_message_buffer->value  = malloc(output_message_buffer->length);
  if(output_message_buffer->value == NULL)
    return GSS_S_FAILURE;
  memcpy (output_message_buffer->value,
	  p + 24,
	  output_message_buffer->length);
  return GSS_S_COMPLETE;
}
