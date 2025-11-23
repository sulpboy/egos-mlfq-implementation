//grep.c

#include "app.h"
#include <string.h>

int main(int argc, char *argv[]) {

    if (argc != 3) {
        INFO("usage: grep [WORD] [FILENAME]");
        return -1;
    }

    char *wrd = argv[1];

    int fh = dir_lookup(workdir_ino, argv[2]);
    if (fh < 0) {
        INFO("grep: file '%s' not found", argv[2]);
        return -1;
    }

    static char filebuf[16*1024];
    int pos = 0;

    char buf[BLOCK_SIZE + 1];
    int off = 0;

    while (1) {

        memset(buf, 0, BLOCK_SIZE + 1);
        int ret = file_read(fh, off, buf);

        if (ret < 0)
            break;

        int i = 0;
        while (i < BLOCK_SIZE && buf[i] != '\0') {
            filebuf[pos++] = buf[i];
            i++;
        }

        if (i < BLOCK_SIZE && buf[i] == '\0')
            break;

        off++;
    }

    filebuf[pos] = '\0';

    char *line = strtok(filebuf, "\n");
    while (line != NULL) {
        if (strstr(line, wrd))
            printf("%s\n", line);
        line = strtok(NULL, "\n");
    }

    return 0;
}