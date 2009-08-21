/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

         * * * * ! SVP4SNetc (S-Net compiler plugin for SVP) ! * * * *
 
                   Computer Systems Architecture (CSA) Group
                             Informatics Institute
                         University Of Amsterdam  2009
                         
      -------------------------------------------------------------------

    File Name      : svp4snetc.c

    File Type      : Code File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

#include "svp4snetc.h"

/*---*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*----------------------------------------------------------------------------*/

#define MAX_ARGS 99

/*---*/

#define STRAPPEND(dest, src) dest = strappend(dest, src)

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

static char* itoa(int val)
{
    int i;

    char *str;
    char *result;

    int len    = 0;
    int offset = (int)('0');

    result = (char *) calloc((MAX_ARGS + 1), sizeof(char));

    for(i=0; i < MAX_ARGS; i++) {
        result[MAX_ARGS - 1 - i] = (char)((val % 10) + offset);

        val /= 10;

        if(val == 0) {
            len = i + 1;
            break;
        }
    }

    str = (char *) malloc((len + 1) * sizeof(char));
    
    strcpy(str, result + (MAX_ARGS - len));
    free(result);

    return str;
}

/*----------------------------------------------------------------------------*/

static char* strappend(char *dest, const char *src)
{
    char *res = (char *) malloc(
        sizeof(char) *
        ((dest == NULL ? 0 : strlen(dest)) + strlen(src) + 1));

    (*res) = 0;

    if (dest != NULL)
        strcpy(res, dest);

    strcat(res, src);

    if (dest != NULL) {
        free(dest);
    }

    return res;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Default generator functions for box wrapper 
 * declarations / definitions and calling statements.
 */

static char*
default_decldef_genwrapper(
    char *box_name,
    snet_input_type_enc_t *t,
    snet_meta_data_enc_t  *meta_data)
{
    int i;

    char *code    = NULL;
    char *tmp_str = NULL;

    STRAPPEND(code, "#pragma weak ");
    STRAPPEND(code, box_name);
    STRAPPEND(code, "\nvoid ");
    STRAPPEND(code, box_name);
    STRAPPEND(code, "(void *hnd");

    if (t != NULL) {
        for (i=0; i < t->num; i++) {
            switch (t->type[i]) {
                case field:
                    STRAPPEND(code, ", void *arg_");
                    break;

                case tag :
                case btag:
                    STRAPPEND(code, ", int arg_");
                    break;
            }

            tmp_str = itoa(i);

            STRAPPEND(code, tmp_str);
            free(tmp_str);
        }
    }
    
    STRAPPEND(code, ")\n{\n");
    STRAPPEND(code, "  // SNetEmptyBox(hnd);\n}");

    return code;
}

/*----------------------------------------------------------------------------*/

static char*
default_callstmnt_genwrapper(
    char *box_name,
    snet_input_type_enc_t *t,
    snet_meta_data_enc_t  *meta_data)
{
    int i;

    char *code    = NULL;
    char *tmp_str = NULL;

    STRAPPEND(code, box_name);
    STRAPPEND(code, "(hnd");

    if (t != NULL) {
        for (i=0; i < t->num; i++) {
            tmp_str = itoa(i);

            STRAPPEND(code, ", arg_");
            STRAPPEND(code, tmp_str);

            if (t->type[i] == field)
                STRAPPEND(code, "_data");

            free(tmp_str);
        }
    }

    STRAPPEND(code, ");");

    return code;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

char* SVP4SNetGenBoxWrapper(
    char *box_name,
    snet_input_type_enc_t *t,
    snet_meta_data_enc_t  *meta_data)
{
    return SVP4SNetGenBoxWrapperEx(
            box_name,
            t,
            meta_data,
            &default_decldef_genwrapper,
            &default_callstmnt_genwrapper);
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

char* SVP4SNetGenBoxWrapperEx( 
    char *box_name,
    snet_input_type_enc_t *t,
    snet_meta_data_enc_t  *meta_data,
    snet_ifgenwrapper_fptr_t decldef_genwrapper_fun,
    snet_ifgenwrapper_fptr_t callstmnt_genwrapper_fun)
{
    int i;

    int fields_cnt = 0;
    int tags_cnt   = 0;

    char *code     = NULL;
    char *tmp_str  = NULL;

    // Generate def or decl of wrapper function (whatever
    // it is required; if at all).
    if (decldef_genwrapper_fun != NULL) {
        STRAPPEND(code, (*decldef_genwrapper_fun)(box_name, t, meta_data));
        STRAPPEND(code, "\n\n");
    }
    
    // Thread definition
    STRAPPEND(code, "#pragma weak SNetBox__");
    STRAPPEND(code, box_name);
    STRAPPEND(code, "\n");

    STRAPPEND(code, "thread void SNetBox__");
    STRAPPEND(code, box_name);
    STRAPPEND(code, "(snet_handle_t *hnd");

    if (t != NULL) {
        for (i=0; i < t->num; i++) {
            switch (t->type[i]) {
                case field:
                    STRAPPEND(code, ", void *arg_");
                    
                    fields_cnt++;
                    break;

                case tag :
                case btag:
                    STRAPPEND(code, ", int arg_");

                    tags_cnt++;
                    break;
            }

            tmp_str = itoa(i);

            STRAPPEND(code, tmp_str);
            free(tmp_str);
        }
    }

    STRAPPEND(code, ")\n{\n");

    if (t != NULL) {
        for (i=0; i < t->num; i++) {
            if (t->type[i] == field) {
                tmp_str = itoa(i);

                STRAPPEND(code, "  void *arg_");
                STRAPPEND(code, tmp_str);
                STRAPPEND(code, "_data = SNetRecFieldGetData(arg_");
                STRAPPEND(code, tmp_str);
                STRAPPEND(code, ");\n");

                free(tmp_str);
            }
        }
    }

    if (callstmnt_genwrapper_fun != NULL) {
        STRAPPEND(code, "\n  ");
        STRAPPEND(code, (*callstmnt_genwrapper_fun)(box_name, t, meta_data));
        STRAPPEND(code, "\n");
    }

    STRAPPEND(code, "}\n\n");

    // Thread I/O function
    STRAPPEND(code, "#ifdef SVPSNETGWRT_SVP_PLATFORM_DUTCPTL\n");

    STRAPPEND(code, "#pragma weak _utctmp_SNetBox__");
    STRAPPEND(code, box_name);
    STRAPPEND(code, "\n");

    STRAPPEND(code, "#pragma weak SNetBox__");
    STRAPPEND(code, box_name);
    STRAPPEND(code, "_io_\n\n");

    STRAPPEND(code, "DISTRIBUTABLE_THREAD(SNetBox__");
    STRAPPEND(code, box_name);
    STRAPPEND(code, ")(uTC::DataBuffer* db, snet_handle_t*& hnd");

    if (t != NULL) {
        for (i=0; i < t->num; i++) {
            switch (t->type[i]) {
                case field:
                    STRAPPEND(code, ", void*& arg_");
                    break;

                case tag :
                case btag:
                    STRAPPEND(code, ", int& arg_");
            }

            tmp_str = itoa(i);

            STRAPPEND(code, tmp_str);
            free(tmp_str);
        }
    }

    STRAPPEND(code, ")\n{\n");
    STRAPPEND(code, "  // SNetRecItemsXDR(db, &hnd, ");

    tmp_str = itoa(fields_cnt);

    STRAPPEND(code, tmp_str);
    free(tmp_str);

    STRAPPEND(code, ", ");

    tmp_str = itoa(tags_cnt);

    STRAPPEND(code, tmp_str);
    free(tmp_str);

    if (t != NULL) {
        for (i=0; i < t->num; i++) {
            tmp_str = itoa(i);

            STRAPPEND(code, ", &arg_");
            STRAPPEND(code, tmp_str);

            free(tmp_str);
        }
    }

    STRAPPEND(code, ");\n}\nASSOCIATE_THREAD(SNetBox__");
    STRAPPEND(code, box_name);
    STRAPPEND(code, ");\n");

    STRAPPEND(code, "#endif\n\n");

    // Box Wrapper
    STRAPPEND(code, "void SNetCall__");
    STRAPPEND(code, box_name);
    STRAPPEND(code, "(snet_handle_t *hnd");

    if(t != NULL) {
        for( i=0; i < t->num; i++) {
            switch(t->type[i]) {
                case field:
                    STRAPPEND(code, ", void *arg_");
                    break;
                
                case tag :
                case btag:
                    STRAPPEND(code, ", int arg_");
                    break;
            }

            tmp_str = itoa(i);

            STRAPPEND(code, tmp_str);
            free(tmp_str);
        }
    }

    STRAPPEND(code, ")\n{\n");

    STRAPPEND(code, "  family fid;\n");
    STRAPPEND(code, "  place  plc = SNetGetBoxSelectedResource(hnd);\n\n");

    STRAPPEND(code, "  create (fid; plc; 0; 0; 1; 1;;) SNetBox__");
    STRAPPEND(code, box_name);
    STRAPPEND(code, "(hnd");

    if(t != NULL) {
        for(i=0; i < t->num; i++) {
            tmp_str = itoa(i);

            STRAPPEND(code, ", arg_");
            STRAPPEND(code, tmp_str);

            free(tmp_str);
        }
    }

    STRAPPEND(code, ");\n  sync(fid);\n}");

    return code;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

