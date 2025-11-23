//wcl.c

#include "app.h"
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        INFO("usage: wcl [FILE1] [FILE2] ...");
        return -1;
    }

    int total = 0;
    for (int f = 1; f < argc; f++) {
        int fh = dir_lookup(workdir_ino, argv[f]);

        if (fh < 0) {
            INFO("wcl: file %s not found", argv[f]);
            continue;
        }

        char buf[BLOCK_SIZE + 1];
        int off = 0;
        int lines = 0;
        int flag = 1;  

        while (1) {
            memset(buf, 0, BLOCK_SIZE + 1);
            int ret = file_read(fh, off, buf);

            if (ret < 0) {
                break;
            }

            int actual = 0;
            while (actual < BLOCK_SIZE && buf[actual] != '\0') {
                if (buf[actual] == '\n') {
                    lines++;
                    flag = 1;
                } else {
                    flag = 0;
                }
                actual++;
            }

            if (actual < BLOCK_SIZE) {
                break;
            }

            off += 1;
        }
        
        if (!flag) {
            lines++;
        }

        total += lines;
    }

    printf("%d\n", total);
    return 0;
}
