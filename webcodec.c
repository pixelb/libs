#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include "base64.h"

/*
   This describes how to encode arbitrary information for
   safe entry in web forms (URLs) for e.g. It's especially useful for
   sending small amounts of binary data (e.g. encrypted keys).

   The (WEB)charset used below is only slightly different
   than base64. This means that you can use the standard base64 tools
   in your web environment and just interchange . with + and - with /
   before you base64_decode.

   So your probably asking why this util is needed at all, when you
   could just use standard base64 encoding tools on the data and replace
   + with . and / with -
   I.E. something like: uuencode -m file.bin - | tr -- +/ .-
   Hmm well this is handier and it's an unwritten law that every
   programmer must write a base64 codec (and also a hex dump util),
   so here's mine.
*/

static unsigned char buf[15];

int main(int argc, char** argv) {
    int num_read;
    int fd=fileno(stdin);

    //buf size should be a multiple of 3 so that 8 to 6 bit conversion gives an integer
    assert((sizeof(buf)%3)==0);

    while ((num_read=read(fd, buf, sizeof(buf)))>0) {
        char* url_text=Base64Encode(buf, num_read, WEBcharset);
	fputs(url_text,stdout);
	free(url_text);
    }
    putchar('\n');
    return EXIT_SUCCESS;
}
