/* <!--Exclude from bashfeed-->
 *
 * PBcrypto - A wrapper around libcrypto from openssl.
 * PB stands for any of the following:
 * PixelBeat, Pointer to Buffer, Probably Better
 *
 * PBcrypto is provided under the terms of the GNU Library
 * General Public License (LGPL) Version 2 with the following exception:
 *   Static linking of applications to the PBcrypto library does not
 *   constitute a derivative work and does not require the author to
 *   provide source code for the application.
 *
 * Author:
 *    http://www.pixelbeat.org/
 * Notes:
 *    To link to this you will also need to link libcrypto and libdl.
 *    I.E. add "-lcrypto -ldl" to the gcc command line.
 * Changes:
 *    V0.1, 03 Sep 2007, Initial release
 *    V0.2, 16 Oct 2009, Documented that aes_256 needs 32 byte keys
 */

#ifndef PBCRYPTO_H
#define PBCRYPTO_H

#include <inttypes.h>  //uint8_t
#include <sys/types.h> //size_t

/* return sha1/md5 digest of data (in ascii hex if hex==1).
 * input length is passed through len parameter.
 * output length is also passed through len parameter.
 * free returned buffer. NULL on error */
uint8_t* sha1(const uint8_t* input, size_t* len, int hex);
uint8_t*  md5(const uint8_t* input, size_t* len, int hex);

/* return Base64 encoded/decoded data (with line breaks if linebreaks==1)
 * free returned string. NULL returned on error */
char*    b64_enc(const uint8_t* input, size_t in_len, int linebreaks);
uint8_t* b64_dec(const char *input, size_t* out_len, int linebreaks);

/* Symmetric ciphers.
 * free returned buffer. NULL on error.
 * Note aes_256 needs 256 bit keys */
uint8_t* blowfish_enc(uint8_t* key, size_t key_len,
                      const uint8_t* input, size_t in_len, size_t* out_len);
uint8_t* blowfish_dec(uint8_t* key, size_t key_len,
                      const uint8_t* input, size_t in_len, size_t* out_len);
uint8_t*  aes_256_enc(uint8_t* key, size_t key_len /*32*/,
                      const uint8_t* input, size_t in_len, size_t* out_len);
uint8_t*  aes_256_dec(uint8_t* key, size_t key_len /*32*/,
                      const uint8_t* input, size_t in_len, size_t* out_len);

/* decrypt data with RSA public key in PEM format.
 * For example, the following key could be specified:
const char* pub_key=
"-----BEGIN PUBLIC KEY-----\n"
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCtCWzctFVrrkw1P3ruCZbMhVZZ\n"
"lbvYVTD6793h4aOCDWACq34aJhq/tQM87kTTfuagJmXW1GcIw+wrI0Q6dIF8hX4/\n"
2H4P7gbk/FmQTUSoKBR0txopl00I5h90FA+75NqTAXnhAJOlMorpo7LRDQxf5FaGw\n"
"6UeK98OWHDlA3cDfmQIDAQAB\n"
"-----END PUBLIC KEY-----\n";

 * To generate a public key in the above format use:

openssl genrsa -out privkey.pem 1024
openssl rsa -in privkey.pem -out pubkey.pem -outform PEM -pubout

* To generate encrypted data with the private key, use:

openssl rsautl -sign -inkey privkey.pem

 * must free returned buffer. NULL on error. */
uint8_t* rsa_pub_dec(uint8_t* pub_key, size_t key_len,
                     uint8_t* input, size_t in_len);

#endif
