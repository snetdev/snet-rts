/*
 * Generate an input record for the 'matrix' application.
 *
 * net matrix({A, B, <A_width>, <A_height>, <B_width>, <B_height>, <nodes>}
 *             -> {C, <C_width>, <C_height>});
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [ -w width | -h height | -n nodes | -o output ]\n",
            prog);
    exit(1);
}

static void output(FILE *fp, int width, int height, int nodes)
{
    int i;

    fputs("<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n", fp);
    fputs("<record xmlns=\"snet-home.org\" type=\"data\" mode=\"textual\" interface=\"C4SNet\">\n", fp);
    fprintf(fp, "<field label=\"A\">(int[%d])", width * height);
    for (i = 0; i < width * height; ++i) {
        fprintf(fp, "%d", i);
        if (i + 1 < width * height) {
            putc(',', fp);
        }
    }
    fprintf(fp, "</field>\n");
    fprintf(fp, "<field label=\"B\">(int[%d])", width * height);
    for (i = 0; i + 1 < width * height; ++i) {
        if (i % width == i / height) {
            putc('1', fp);
        } else {
            putc('0', fp);
        }
        putc(',', fp);
    }
    putc('1', fp);
    fprintf(fp, "</field>\n");
    fprintf(fp, "<tag label=\"A_width\">%d</tag>\n", width);
    fprintf(fp, "<tag label=\"A_height\">%d</tag>\n", height);
    fprintf(fp, "<tag label=\"B_width\">%d</tag>\n", width);
    fprintf(fp, "<tag label=\"B_height\">%d</tag>\n", height);
    fprintf(fp, "<tag label=\"nodes\">%d</tag>\n", nodes);
    fputs("</record>\n", fp);
    fputs("<record type=\"terminate\" />\n", fp);

}

int main(int argc, char **argv)
{
    int         c;
    int         width = 500;
    int         height = 500;
    int         nodes = 1;
    char*       outf = NULL;
    FILE*       fp;

    while ((c = getopt(argc, argv, "w:h:n:o:")) != EOF) {
        switch (c) {
            case 'w':
                if (sscanf(optarg, "%d", &width) < 1 || width <= 0) {
                    usage(*argv);
                }
                break;
            case 'h':
                if (sscanf(optarg, "%d", &height) < 1 || height <= 0) {
                    usage(*argv);
                }
                break;
            case 'n':
                if (sscanf(optarg, "%d", &nodes) < 1 || nodes <= 0) {
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
    output(fp, width, height, nodes);
    if (fp != stdout) {
        fclose(fp);
    }

    return 0;
}

