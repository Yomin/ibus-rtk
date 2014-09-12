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

#include <stdio.h>
#include <stdlib.h>
#include "lookup.h"

int main(int argc, char *argv[])
{
    struct rtkinput *input;
    struct rtkresult *result;
    int x;
    
    if(argc < 3)
    {
        fprintf(stderr, "Usage: %s <kanjifile> <primitive> [<primitive> ...]\n", argv[0]);
        return 1;
    }
    
    if(rtk_lookup_init(argv[1]))
        return 2;
    
    input = malloc((argc-2)*sizeof(struct rtkinput));
    for(x=0; x<argc-2; x++)
        input[x].primitive = argv[x+2];
    
    result = rtk_lookup(argc-2, input);
    
    if(result)
    {
        while(result->kanji)
        {
            printf("found: [%u] %s %s\n", result->number, result->kanji, result->meaning);
            result++;
        }
    }
    else
        for(x=0; x<argc-2; x++)
            if(!input[x].found)
                printf("not found: %s\n", input[x].primitive);
    
    free(input);
    rtk_lookup_free();
    
    return 0;
}
