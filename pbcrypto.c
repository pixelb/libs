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

#include <inttypes.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

static unsigned char iv[EVP_MAX_IV_LENGTH]; /* init to 0 */

/* interface to symmetric ciphers. don't use directly */
static uint8_t* evp_crypt(const EVP_CIPHER* cipher, uint8_t* key,
                          size_t key_len, uint8_t* data, size_t data_len,
                          size_t* out_len, int encrypt)
{
    uint8_t* outbuf=NULL;
    EVP_CIPHER_CTX ctx;
    EVP_CIPHER_CTX_init(&ctx);

    do {
        if (!EVP_CipherInit_ex(&ctx, cipher, NULL, key, iv, encrypt))
            break;

        /* Some ciphers have fixed length keys */
        if (!EVP_CIPHER_CTX_set_key_length(&ctx,key_len))
            break;

        int bs=EVP_CIPHER_CTX_block_size(&ctx);
        int padding;
        if (encrypt) {
            padding=bs-(data_len%bs);
            /* according to docs this should be (bs*2)-1 ? */
        } else {
            padding=0;
            /* according to docs this should be bs ? */
        }
        outbuf=malloc(data_len + padding);
        if (!outbuf)
            break;

        if (!EVP_CipherUpdate(&ctx, outbuf, (int*)out_len, data, data_len))
            break;

        int fl;
        if (!EVP_CipherFinal_ex(&ctx, outbuf + *out_len, &fl))
            break;
        *out_len+=fl;

        EVP_CIPHER_CTX_cleanup(&ctx);
        return outbuf;
    } while(0);

    free(outbuf);
    outbuf=NULL;
    EVP_CIPHER_CTX_cleanup(&ctx);
    return outbuf;
}
uint8_t* blowfish_enc(uint8_t* key, size_t key_len, uint8_t* input,
                      size_t in_len, size_t* out_len) {
    return evp_crypt(EVP_bf_cbc(), key, key_len, input, in_len, out_len, 1);
}
uint8_t* blowfish_dec(uint8_t* key, size_t key_len, uint8_t* input,
                      size_t in_len, size_t* out_len) {
    return evp_crypt(EVP_bf_cbc(), key, key_len, input, in_len, out_len, 0);
}
uint8_t* aes_256_enc(uint8_t* key, size_t key_len, uint8_t* input,
                     size_t in_len, size_t* out_len) {
    return evp_crypt(EVP_aes_256_cbc(), key, key_len, input, in_len, out_len,1);
}
uint8_t* aes_256_dec(uint8_t* key, size_t key_len, uint8_t* input,
                     size_t in_len, size_t* out_len) {
    return evp_crypt(EVP_aes_256_cbc(), key, key_len, input, in_len, out_len,0);
}
/* `grep EVP_.*_cbc /usr/include/openssl/evp.h` for other symmetric ciphers */

/* decrypt data with RSA public key
 * must free returned buffer. NULL on error */
uint8_t* rsa_pub_dec(uint8_t* pub_key, size_t key_len,
                     uint8_t* input, size_t in_len)
{
    uint8_t* dst = NULL;

    BIO* bio = BIO_new_mem_buf(pub_key, key_len);
    if (bio) {
        RSA* rsa_key = 0;
        if (PEM_read_bio_RSA_PUBKEY(bio, &rsa_key, NULL, NULL)) {
            if (in_len == RSA_size(rsa_key)) {
                dst = malloc(in_len);
                RSA_public_decrypt(in_len, input, dst, rsa_key,
                                   RSA_PKCS1_PADDING);
            }
            RSA_free(rsa_key);
        }
        BIO_free(bio);
    }
    return dst;
}

/* Base 64 encode data
 * free returned string. NULL returned on error
 *
 * Note the BIO interface is a higher level interface
 * to EVP_EncodeBlock and EVP_EncodeUpdate etc. */
char *b64_enc(uint8_t* input, size_t in_len, int linebreaks)
{
    char *buf=NULL;
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    if (!b64) return buf;
    if (!linebreaks) {
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    }
    bmem = BIO_new(BIO_s_mem());
    if (bmem) {
        b64 = BIO_push(b64, bmem);
        if (BIO_write(b64, input, in_len)==in_len) {
            (void)BIO_flush(b64);
            BIO_get_mem_ptr(b64, &bptr);

            buf = malloc(bptr->length+1);
            if (buf) {
                memcpy(buf, bptr->data, bptr->length);
                buf[bptr->length] = '\0';
            }
        }
    }

    BIO_free_all(b64);

    return buf;
}

/* Base 64 decode data.
 * free returned data, NULL returned on error.
 *
 * Note the BIO interface is a higher level interface
 * to EVP_DecodeBlock and EVP_DecodeUpdate etc. */
uint8_t* b64_dec(const char *input, size_t* out_len, int linebreaks)
{
    BIO *bmem, *b64;

    int in_len=strlen(input);
    int out_max_len=(in_len*6+7)/8;
    unsigned char *buf = malloc(out_max_len);
    if (buf) {
        memset(buf, 0, out_max_len);

        b64 = BIO_new(BIO_f_base64());
        if (b64) {
            if (!linebreaks) {
                BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
            }
            bmem = BIO_new_mem_buf((char*)input, in_len);
            b64 = BIO_push(b64, bmem);
            *out_len=BIO_read(b64, buf, out_max_len);
            BIO_free_all(b64);
        }
    }

    return buf;
}

uint8_t* md5(const uint8_t* input, size_t* len, int hex)
{
    uint8_t* output=NULL;
    size_t out_len;
    if (hex) {
        out_len = MD5_DIGEST_LENGTH*2+1;
    } else {
        out_len = MD5_DIGEST_LENGTH;
    }
    output=malloc(out_len);

    if (output) {
        uint8_t digest[MD5_DIGEST_LENGTH];
        (void) MD5(input, *len, digest);
        if (hex) {
            int i;
            for (i=0; i<sizeof(digest); i++) {
                sprintf((char*)output+i*2,"%02x",digest[i]);
            }
        } else {
            memcpy(output, digest, sizeof(digest));
        }
        *len = out_len;
    }
    return output;
}

uint8_t* sha1(const uint8_t* input, size_t* len, int hex)
{
    uint8_t* output=NULL;
    size_t out_len;
    if (hex) {
        out_len = SHA_DIGEST_LENGTH*2+1;
    } else {
        out_len = SHA_DIGEST_LENGTH;
    }
    output=malloc(out_len);

    if (output) {
        uint8_t digest[SHA_DIGEST_LENGTH];
        (void) SHA1(input, *len, digest);
        if (hex) {
            int i;
            for (i=0; i<sizeof(digest); i++) {
                sprintf((char*)output+i*2,"%02x",digest[i]);
            }
        } else {
            memcpy(output, digest, sizeof(digest));
        }
        *len = out_len;
    }
    return output;
}
