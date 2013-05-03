/*
 * Generate an input record for the cannon-algorithm application.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [ -s size | -d dimension | -o output ]\n",
            prog);
    exit(1);
}

static void output(FILE *fp, int size, int dims)
{
    int i, j;

    fputs("<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n", fp);
    fputs("<record xmlns=\"snet-home.org\" type=\"data\" mode=\"textual\" interface=\"C4SNet\">\n", fp);
    fprintf(fp, "<field label=\"matrix_A\">(int[%d])", size * size);
    for (i = 0; i < size * size; ++i) {
        fprintf(fp, "%d", i);
        if (i + 1 < size * size) {
            putc(',', fp);
        }
    }
    fprintf(fp, "</field>\n");
    fprintf(fp, "<field label=\"matrix_B\">(int[%d])", size * size);
    for (i = 1; i < size; ++i) {
        putc('1', fp);
        putc(',', fp);
        for (j = 0; j < size; ++j) {
            putc('0', fp);
            putc(',', fp);
        }
    }
    putc('1', fp);
    fprintf(fp, "</field>\n");
    fprintf(fp, "<field label=\"size_matrix\">(int)%d</field>\n", size);
    fprintf(fp, "<field label=\"division\">(int)%d</field>\n", dims);
    fputs("</record>\n", fp);
    fputs("<record type=\"terminate\" />\n", fp);

}

int main(int argc, char **argv)
{
    int         c;
    int         size = 100;
    int         dims = 4;
    char*       outf = NULL;
    FILE*       fp;

    while ((c = getopt(argc, argv, "s:d:o:")) != EOF) {
        switch (c) {
            case 's':
                if (sscanf(optarg, "%d", &size) < 1 || size <= 0) {
                    usage(*argv);
                }
                break;
            case 'd':
                if (sscanf(optarg, "%d", &dims) < 1 || size <= 0) {
                    usage(*argv);
                }
                break;
            case 'o':
                outf = optarg;
                break;
            default:
                usage(*argv);
        }
    }

    if (outf) {
        if ((fp = fopen(outf, "w")) == NULL) {
            fprintf(stderr, "%s: failed to open %s: %s\n",
                    *argv, outf, strerror(errno));
            exit(1);
        }
    } else {
        fp = stdout;
    }
    output(fp, size, dims);
    if (fp != stdout) {
        fclose(fp);
    }

    return 0;
}

