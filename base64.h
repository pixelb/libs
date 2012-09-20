#ifndef BASE64_CODEC_H
#define BASE64_CODEC_H

extern const char* WEBcharset;
extern const char* B64charset;
extern const char* UU_charset;
unsigned char* Base64Encode(const unsigned char* data, unsigned int data_size, const char* charset);
unsigned int Base64Decode(const unsigned char* data, unsigned char** decoded_text, const char* charset);

#endif //BASE64_CODEC_H
