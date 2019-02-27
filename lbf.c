
/* lbf
 * Copyright (C) Kamila Palaiologos Szewczyk, 2019.
 * License: MIT
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct block_t {
    struct block_t * next, *prev;
    int amount, used;
    char * label, * code;
};

typedef struct block_t* pblock_t;

pblock_t current = NULL;

error(s) char * s; {
    perror(s);
    abort();
    return -1; /* Shouldn't happen. */
}

blockid(label, target) char * label; pblock_t * target; {
    pblock_t cur = current;
    
    while (cur) {
        if (!strcmp(cur->label, label)) {
            if (target)
                *target = cur;
            return cur->amount;
        }
        cur = cur->next;
    }
    
    return -1;
}

extrntrace(blk) pblock_t blk; {
    char * code;
    int lcode, i = 0, j, o = 0;
    pblock_t tblk;
    
    code = strdup(blk->code);
    if (!code)
        error("strdup");
    
    for (; code[i]; i++) {
        switch (code[i]) {
            case '(':
                for (j = i + 1; code[j] && code[j] != ')'; j++);
                
                if (!code[j]) {
                    fprintf(stderr, "Invalid %s:%d in %s\n", code + i, i, blk->label);
                    abort();
                }
                
                code[j] = '\0';
                ++i;
                if (code[i] == '*')
                    ++i;
                
                if (blockid(code + i, &tblk) == -1) {
                    fprintf(stderr, "Unresolved external: %s:%d in %s\n", code + i, i, blk->label);
                    abort();
                }
                
                if (tblk->used == 0) {
                    tblk->used = 1;
                    extrntrace(tblk);
                }
                
                i = j;
                break;
            case '!':
                for (; code[i]; i++); --i;
                break;
        }
    }
    
    free(code);
    return 0;
}

process(blk) pblock_t blk; {
    char * ncode, * code;
    int lcode, amount, i = 0, j, o = 0;
    
    code = blk->code;
    lcode = strlen(code);
    ncode = malloc(lcode + 1);
    if (!ncode)
        error("malloc");
    
    for (; code[i]; i++) {
        switch (code[i]) {
            case '(':
                for (j = i + 1; code[j] && code[j] != ')'; j++);
                
                if (!code[j]) {
                    fprintf(stderr, "Invalid %s:%d in %s\n", code + i, i, blk->label);
                    abort();
                }
                
                code[j] = '\0';
                i++;
                
                switch (code[i]) {
                    case '*':
                        amount = blockid(code + i + 1, NULL);
                        if (amount == -1) {
                            fprintf(stderr, "Unresolved external: %s:%d in %s\n", code + i + 1, i + 1, blk->label);
                            abort();
                        }
                        
                        /* TODO: Create an algo to write digit instead of adding bunch of '+' */
                        
                        lcode += amount + 3;
                        ncode = realloc(ncode, lcode);
                        if (!ncode)
                            error("realloc");
                        sprintf(ncode + o, "[-]");
                        
                        o += 3;
                        for (; amount > 0; amount--) {
                            ncode[o] = '+';
                            o++;
                        }
                        break;
                    default:
                        amount = blockid(code + i, NULL);
                        if (amount == -1) {
                            fprintf(stderr, "Undefined symbol: %s:%d in %s\n", code + i, i, blk->label);
                            exit(1);
                        }
                        
                        lcode += 5 * amount + 25;
                        ncode = realloc(ncode, lcode);
                        if (!ncode)
                            error("realloc");
                        sprintf(ncode + o, "<<<[<<<<<]>>>");
                        
                        o += 13;
                        for (; amount > 0; amount--) {
                            sprintf(ncode + o, ">>>>>");
                            o += 5;
                        }
                        
                        ncode[o] = '+';
                        o++;
                        sprintf(ncode + o, ">>[>>>>>]<<");
                        o += 11;
                }
                i = j;
                break;
            case '!':
                for (; code[i]; i++); --i;
                break;
            default:
                ncode[o++] = code[i];
        }
    }
    
    ncode[o] = '\0';
    blk->code = ncode;
    return 0;
}

main() {
    char * line;
    pblock_t nblk, cur;
    int i, osl, bufsize, resv, startblock;
    
    while (!feof(stdin) && !ferror(stdin)) {
        bufsize = 2048;
        line = (char *) malloc(bufsize + 1);
        if (!line)
            error("malloc");
        line[bufsize] = '\0';
        
        if (!fgets(line, bufsize, stdin)) {
            free(line);
            continue;
        } else {
            while (line[bufsize - 2]) {
                line = realloc(line, (bufsize * 2) + 1);
                if (!line)
                    error("realloc");
                line[bufsize * 2] = '\0';
                if (!fgets(line + bufsize - 1, bufsize + 1, stdin))
                    break;
                bufsize *= 2;
            }
        }
        
        osl = strlen(line) - 1;
        while (line[osl] == '\n' || line[osl] == '\r') {
            line[osl] = '\0';
            osl--;
        }
        
        if (!line[0]) {
            free(line);
            continue;
        }
        osl++;
        
        line = realloc(line, osl);
        if (!line)
            error("realloc");
        
        nblk = malloc(sizeof(struct block_t));
        nblk->next = current;
        nblk->prev = NULL;
        if (current) current->prev = nblk;
        current = nblk;
        if (current->next)
            current->amount = current->next->amount + 1;
        else
            current->amount = 0;
        current->used = 0;
        
        for (i = 0; line[i] && line[i] != ':'; i++);
        if (!line[i]) {
            fprintf(stderr, "Error in: %s\n", line);
            free(line);
            continue;
        }
        line[i++] = '\0';
        
        for (; line[i] && (line[i] == ':' || line[i] == ' '); i++);
        current->label = line;
        current->code = line + i;
    }
    if (!current)
        return 0;
    
    if ((startblock = blockid("__start", &cur)) == -1) {
        fprintf(stderr, "Unresolved external: __start\n");
        exit(1);
    }
    
    cur->used = 1;
    extrntrace(cur);
    cur = current;
    
    while (cur) {
        process(cur);
        cur = cur->next;
    }
    
    printf(">>");
    for (i = 0; i < current->amount; i++)
        printf("[>+>>>>+<<<<<-]>[<+>-]>>>>+");
    printf(">>>");
    
    for (i = 0; i < current->amount; i++)
        printf("<<<<<");
    osl = startblock;
    
    for (; startblock > 0; startblock--)
        printf(">>>>>");
    startblock = osl;
    printf("+");
    
    for (; startblock > 0; startblock--)
        printf("<<<<<");
    printf("<<<<<+[");
    cur = current;
    
    while (cur->next)
        cur = cur->next;
    
    while (cur) {
        if (cur->used) {
            printf(">>>>>[->>[>>>>>]<<");
            printf(cur->code);
            printf("<<<[<<<<<]>>>");
            for (i = 0; i < cur->amount; i++)
                printf(">>>>>");
            printf("]");
        } else {
            printf(">>>>>");
        }
        cur = cur->prev;
    }
    
    printf("<<<[<<<<<]<<]");
    return 0;
}
