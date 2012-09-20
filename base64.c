#include "base64.h"
#include <string.h>
#include <stdlib.h>

/* WEBcharset is safe for URLs (web forms). See (in python) urllib.always_safe. Note "." is safe also.
   Note WEBcharset (including .) is also the full set of POSIX portable filename chars */
const char* WEBcharset="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"; /* as per RFC 3548 */
const char* B64charset="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char* UU_charset="`!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";

static unsigned char* B64_allocEncodedBuffer(const unsigned int data_size)
{
    if (!data_size) {
        return NULL;
    }

    {
    //unsigned int mem_size = (unsigned int) ceil((double)data_size / 3 * 4);
    unsigned int mem_size = (data_size * 8 + 5) / 6; //8 bit to 6 bit conversion (not as exact as float above but always big enough)
    unsigned int linefeed =  0;//(mem_size + 63) / 64;
    unsigned char* buffer = (char*) malloc(mem_size + linefeed + 1);
    return buffer;
    }
}

static unsigned char* B64_allocDecodedBuffer(const unsigned int data_size)
{
    if (!data_size) {
        return NULL;
    }

    {
    unsigned int mem_size = (data_size * 6 + 7) / 8; //6 to 8 bit conversion (not exact but always big enough)
    unsigned char* buffer = (char*) malloc(mem_size);
    return buffer;
    }
}

unsigned Base64Decode(const unsigned char* data, unsigned char** decoded_text, const char* charset)
{
    unsigned int char_pos;
    unsigned int byte_num;
    unsigned lastbyte;
    int size;
    unsigned char* ptr;
    unsigned char bits;

    unsigned char* sixbitbuffer;
    unsigned int data_size=strlen((char*)data);
    *decoded_text = B64_allocDecodedBuffer(data_size);
    if (*decoded_text == 0) {
        return 0;
    }

    if (strlen(charset) != 64) {
        return 0;//sanity check
    }

    sixbitbuffer = (char*) malloc(data_size);
    if (sixbitbuffer == 0) {
        return 0;
    }

    for (char_pos=0; char_pos<data_size; char_pos++)
        sixbitbuffer[char_pos] = strchr(charset, data[char_pos]) - charset;

    ptr = *decoded_text;
    bits = 0;
    data = sixbitbuffer;

    for (byte_num=0; byte_num<=data_size-4; byte_num+=4) {
        bits = *data++ << 2;
        *ptr++ = bits | (*data >> 4);
        bits = *data++ << 4;
        *ptr++ = bits | (*data >> 2);
        bits = *data++ << 6;
        *ptr++ = bits | (*data++);
    }

    for (lastbyte=2; lastbyte<=(data_size-byte_num); lastbyte++) {
        //Can only get 8 bits if 2 or 3 bytes left
        switch (lastbyte) {
            case 2:
                bits = *data++ << 2;
                *ptr++ = bits | (*data >> 4);
                break;
            case 3:
                bits = *data++ << 4;
                *ptr++ = bits | (*data >> 2);
                break;
        }
    }

    size = ptr - *decoded_text;
    free(sixbitbuffer);
    return size;
}

//Returns NULL terminated buffer that must be free().
unsigned char* Base64Encode(const unsigned char* data, const unsigned int data_size, const char* charset)
{
    unsigned char* ptr;
    int shift;
    unsigned char bits;
    unsigned int i;

    unsigned char* buffer = B64_allocEncodedBuffer(data_size);
    if (buffer == 0) {
        return 0;
    }

    if (strlen(charset) != 64) {
        return 0;//sanity check
    }

    ptr = buffer;
    shift = -2;
    bits = 0;
    for (i = 0; i < data_size; i++) {
        switch (shift) {
        case 0:/* bits is full */
            *ptr = charset[bits]; ptr++;
        case -2: /* load new character with fresh bits */
            bits = *data & 0x03;
            *ptr = charset[*data >> 2]; ptr++;
            shift = 4;
            data++;
            break;
        default:
            *ptr = charset[(bits << shift) | (*data >> (8 - shift))]; ptr++;
            bits = *data & (0xff >> shift);
            shift -= 2;
            data++;
        }
    }
    if (shift)
        *ptr = charset[bits << shift];
    else
        *ptr = charset[bits];
    ptr++;
    *ptr='\0';

    return buffer;
}
