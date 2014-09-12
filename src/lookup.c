/*
 * Copyright (c) 2014 Martin Rödel aka Yomin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "lookup.h"

#ifndef IBUS_RTK
#   define print(format, ...) fprintf(stdout, format, __VA_ARGS__)
#   define warn(format, ...) fprintf(stderr, format, __VA_ARGS__)
#   define error(str) perror(str)
#else
#   include <glib.h>
#   define print(format, ...) if(verbose) g_print(format, __VA_ARGS__)
#   define warn(format, ...) if(verbose) g_printerr(format, __VA_ARGS__)
#   define error(str) g_printerr("%s: %s\n", str, g_strerror(errno))
    extern gboolean verbose;
#endif

#define DEFAULT_CAP 10

struct rtkprim
{
    char **prim;
    int count, cap;
};


FILE *rtk_dict;
struct rtkresult *rtk_results;
int rtk_result_count, rtk_result_cap;


void rtk_result_add(unsigned int number, char *kanji, char *meaning)
{
    if(rtk_result_count == rtk_result_cap-1)
    {
        rtk_result_cap += DEFAULT_CAP;
        rtk_results = realloc(rtk_results, rtk_result_cap*sizeof(struct rtkresult));
        memset(rtk_results+rtk_result_count, 0, (rtk_result_cap-rtk_result_count)*sizeof(struct rtkresult));
    }
    rtk_results[rtk_result_count].number = number;
    rtk_results[rtk_result_count].kanji = strdup(kanji);
    rtk_results[rtk_result_count].meaning = strdup(meaning);
    rtk_result_count++;
}

char* rtk_norm(char *str)
{
    char *s = str;
    
    // substitute upper for lower characters
    // and special characters with whitespace
    while(*s)
    {
        if(*s >= 'A' && *s <= 'Z')
            *s += 32;
        else switch(*s)
        {
        case '-':
        case '.':
        case '?':
        case '\'':
            *s = ' ';
        }
        ++s;
    }
    
    // remove trailing whitespace
    --s;
    while(*s == ' ' && s >= str)
        *s-- = 0;
    
    // remove plural 's'
    // non plural words are severed but two primitives
    // should not differ by only the trailing 's'
    if(*s == 's')
        *s = 0;
    
    return str;
}

void rtk_prim_add(char *str, struct rtkprim *p, int norm)
{
    int len = strlen(str);
    
    if(str[len-1] == '\n')
    {
        len--;
        str[len] = 0;
    }
    
    if(p->count == p->cap)
    {
        p->cap += DEFAULT_CAP;
        p->prim = realloc(p->prim, p->cap*sizeof(char**));
    }
    
    p->prim[p->count] = strdup(str);
    p->count++;
    
    if(norm)
        rtk_norm(p->prim[p->count-1]);
}

void rtk_prim_free(struct rtkprim *p)
{
    int x;
    for(x=0; x<p->count; x++)
        free(p->prim[x]);
    free(p->prim);
}

int rtk_lookup_init(const char *file)
{
    if(!(rtk_dict = fopen(file, "r")))
    {
        error("Failed to open kanjifile");
        return 1;
    }
    
    rtk_results = calloc(DEFAULT_CAP, sizeof(struct rtkresult));
    rtk_result_count = 0;
    rtk_result_cap = DEFAULT_CAP;
    
    return 0;
}

void rtk_lookup_free()
{
    int x;
    
    fclose(rtk_dict);
    rtk_dict = 0;
    
    for(x=0; x<rtk_result_count; x++)
    {
        free(rtk_results[x].kanji);
        free(rtk_results[x].meaning);
    }
    free(rtk_results);
    rtk_results = 0;
}

int rtk_number(char *str)
{
    if(strspn(str, "1234567890") == strlen(str))
        return atoi(str);
    return 0;
}

struct rtkresult* rtk_lookup(int argc, struct rtkinput *argv)
{
    struct rtkprim *prim, ptmp1, ptmp2;
    int x, y, z, found, foundpos, lnum, skip;
    size_t n;
    char *line, *tmpstr;
    char *num, *pskip, *kanji, *meaning, *alt, *kprim;
    
    if(!argc)
        return 0;
    
    prim = malloc((argc)*sizeof(struct rtkprim));
    
    if(rtk_result_count)
    {
        for(x=0; x<rtk_result_count; x++)
        {
            free(rtk_results[x].kanji);
            free(rtk_results[x].meaning);
        }
        memset(rtk_results, 0, rtk_result_count*sizeof(struct rtkresult));
        rtk_result_count = 0;
    }
    
    for(x=0; x<argc; x++)
    {
        prim[x].count = 1;
        prim[x].cap = DEFAULT_CAP;
        prim[x].prim = malloc(DEFAULT_CAP*sizeof(char**));
        prim[x].prim[0] = strdup(argv[x].primitive);
        rtk_norm(prim[x].prim[0]);
        argv[x].found = 0;
    }
    
    line = 0;
    lnum = 0;
    while(getline(&line, &n, rtk_dict) != -1)
    {
        lnum++;
        
        if(*line == '\n' || *line == '#')
            continue;
        if(    !(num = strtok(line, ":"))
            || !(pskip = strtok(0, ":"))
            || !(kanji = strtok(0, ":"))
            || !(meaning = strtok(0, ":"))
            || !(alt = strtok(0, ":"))
            || !(kprim = strtok(0, ":"))
            || *kprim == '\n')
        {
            warn("failed to parse line %i\n", lnum);
            continue;
        }
        
        ptmp1.count = 0;
        ptmp1.cap = DEFAULT_CAP;
        ptmp1.prim = malloc(DEFAULT_CAP*sizeof(char**));
        
        // create list of meaning and alternative meanings
        rtk_prim_add(meaning, &ptmp1, 1);
        tmpstr = strtok(alt, "/");
        while(tmpstr && (tmpstr[0] != '-' || tmpstr[1]))
        {
            rtk_prim_add(tmpstr, &ptmp1, 1);
            tmpstr = strtok(0, "/");
        }
        
        // ignore meaning/alternatives
        skip = rtk_number(pskip);
        
        // for every user entered primitive
        // if the primitive is found in the meaning or alt meanings
        // add those except the one found to the respective
        // primitive list
        found = 0;
        foundpos = -1;
        for(x=0; x<argc; x++)
            for(z=0; z<ptmp1.count; z++)
                if(!strcmp(prim[x].prim[0], ptmp1.prim[z]))
                {
                    found++;
                    argv[x].found = 1;
                    
                    // only add if found meaning/alt not skipped
                    // and add only those which are not skipped
                    if(found == 1 && z >= skip)
                    {
                        foundpos = z;
                        for(z=skip; z<ptmp1.count; z++)
                            if(z != foundpos)
                                rtk_prim_add(ptmp1.prim[z], &prim[x], 0);
                    }
                    break;
                }
        
        // continue if no sub primitives
        if(kprim[0] == '-' && kprim[1] == '\n')
        {
            if(found == argc && rtk_number(num))
                rtk_result_add(atoi(num), kanji, meaning);
            
            rtk_prim_free(&ptmp1);
            continue;
        }
        
        // create list of sub primitives
        ptmp2.count = 0;
        ptmp2.cap = DEFAULT_CAP;
        ptmp2.prim = malloc(DEFAULT_CAP*sizeof(char**));
        tmpstr = strtok(kprim, "/");
        while(tmpstr)
        {
            rtk_prim_add(tmpstr, &ptmp2, 1);
            tmpstr = strtok(0, "/");
        }
        
        // for every user entered primitive
        // if one of the respective primitive list matches one out
        // of the current list of sub primitives
        // add meaning and alt meaning of the current kanji to the
        // respective primitive list
        found = 0;
        for(x=0; x<argc; x++)
        {
            for(y=0; y<prim[x].count; y++)
                for(z=0; z<ptmp2.count; z++)
                    if(!strcmp(prim[x].prim[y], ptmp2.prim[z]))
                    {
                        found++;
                        y = prim[x].count;
                        z = -1;
                        break;
                    }
            if(z == -1)
            {
                // skip meaning/alt if already defined as primitive/alt
                for(z=skip; z<ptmp1.count; z++)
                    rtk_prim_add(ptmp1.prim[z], &prim[x], 0);
            }
            // mark found if meaning == user entered primitive
            else
                for(y=0; y<ptmp1.count; y++)
                    if(!strcmp(prim[x].prim[0], ptmp1.prim[y]))
                        found++;
        }
        
        // if for every primitve list a matching one is found
        // and the current kanji is not numberless
        if(found >= argc && rtk_number(num))
            rtk_result_add(atoi(num), kanji, meaning);
        
        rtk_prim_free(&ptmp1);
        rtk_prim_free(&ptmp2);
    }
    
    for(x=0; x<argc; x++)
        rtk_prim_free(&prim[x]);
    
    free(prim);
    free(line);
    
    fseek(rtk_dict, 0, SEEK_SET);
    
    if(!rtk_result_count)
        return 0;
    return rtk_results;
}
