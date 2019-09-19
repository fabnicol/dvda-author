/*
 * Simple config lexer
 *
 * Copyright (c) 2008 fabnicol@users.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, write to the Free Software Foundation, Fifth
 * Floor, 51 Franklin Street, Boston, MA 02111-1301, USA.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "structures.h"
#include "lexer.h"
#include "dvda-author.h"
#include "auxiliary.h"
#include "c_utils.h"

lexer_t *config_lexer(const char* path, lexer_t *lexer, globalData* globals)
{

    int i=0,j=1,u=0, dataflag=0, flag=0;
    bool first=0, delta=0;
    enum {GROUP, TITLE, OTHER, SCREENTEXT, SCREENTEXT_GROUP, SCREENTEXT_TRACK,STILLPICS_TITLE,STILLPICS_TRACK,STILLOPTIONS_RANK,STILLOPTIONS_TRACK,SHORTOPTION,LONGOPTION,ARG};
    char T[MAX_OPTION_LENGTH];
    memset(T, 0, MAX_OPTION_LENGTH);
    // leaving MAX_OPTION_LENGTH as maximal white space between switch and option


    char tab[MAX_OPTION_LENGTH];
    char *chain=&tab[0];
    if (NULL == chain) perror(ERR "lexer.c chain");

    unsigned long s0=0, mem_s1=0, mem_s2=0;


    FILE* defaults=fopen(path, "rb");

    if (defaults == NULL)
    {
       fprintf(stderr, ERR "fopen(%s, \"rb\") crashed\n", path);
       fprintf(stderr, "%s\n", ERR "Could not open default file dvda-author.conf");
       exit(-1);
    }
        clearerr(defaults);

    strcpy(lexer->commandline[0],"dvda-author");

    do
    {

        if (feof(defaults)) break;

        if (NULL==fgets(chain, 500, defaults))
          {
            if (globals->debugging) foutput("Could not get chain at line %d\n",i);
            continue;
          }

            /* skipping white space : space or tab */
            while (isblank(*chain)) chain++;


        /* skipping empty lines and comments */

        switch (*chain)
        {
        case '\n':
            switch (dataflag)
            {

	    case STILLPICS_TRACK:
                dataflag=STILLPICS_TITLE;
                break;

	    case STILLOPTIONS_TRACK:
                dataflag=STILLOPTIONS_RANK;
                break;

           case SCREENTEXT:
		dataflag=SCREENTEXT_GROUP;
		break;

            case SCREENTEXT_GROUP:
		dataflag=SCREENTEXT_TRACK;
		break;

            case SCREENTEXT_TRACK:
                dataflag=SCREENTEXT_GROUP;
                break;
            }
            continue;

        case '#' :
            continue;

       case '[':
             chain++;
             while ((chain[u] != ']') && (chain[u] != '\0'))
             {
                 T[u] = chain[u];
                 ++u;
             }
             if (chain[u] != ']')  EXIT_ON_RUNTIME_ERROR
	         flag =( u== 1)?SHORTOPTION:LONGOPTION;
             T[u]='\0';
             u = 0;
             mem_s1=mem_s2=0;
             j++;i++;
             break;

        case '/' :
             if (*(chain+1) == '/') continue;
             /* fall through */
             __attribute__((fallthrough));

        default:
             if ((flag == SHORTOPTION) ||  (flag == LONGOPTION)) flag=ARG;
             break;
        }


       switch (flag)
       {


        case ARG:

		strcpy(T, chain);

        u = (int) strlen(T) - 1;
		while((u) && isblank(T[u])) --u;
		if (u) T[u] = '\0';
                break;

        case SHORTOPTION:

	    switch (T[0])
		    {
		    case 'j':
		    case 'g':
		    case 's':
			dataflag=GROUP;

			break;
		    case 'z':
			dataflag=TITLE;

			break;
		    case 'O':
			dataflag=SCREENTEXT;
			++i; ++j;

			break;
            case 3:
			dataflag=STILLPICS_TITLE;
			mem_s2=0;
			++i; ++j;
			break;
            case 2:
			dataflag=STILLOPTIONS_RANK;
			mem_s2=0;
			++i; ++j;
			break;

		    default :

			dataflag=OTHER;
		    }
	    break;


#ifdef LONG_OPTIONS
        case LONGOPTION:

                if (strcmp(T, "title") == 0)
                  { dataflag = TITLE;  }
                else if  (strcmp(T, "screentext") == 0)
                  { dataflag = SCREENTEXT; ++i; ++j;}
                else if (strcmp(T, "stillpics") == 0)
                  {dataflag=STILLPICS_TITLE;mem_s2=0; ++i; ++j;}
                else if (strcmp(T, "stilloptions") == 0)
                  {dataflag = STILLOPTIONS_RANK;mem_s2=0; ++i; ++j;}
		else dataflag = OTHER;
                break;


#endif

       }

       s0 = strlen(T);
       switch (flag)
       {
        case ARG:
            switch (dataflag)
            {
            case GROUP:
            case TITLE:
            case OTHER:
		        ++i; ++j;
                memmove(lexer->commandline[j], T, s0+1);
                break;

            case SCREENTEXT:
                ++i; ++j;
                /* fall through */
                __attribute__((fallthrough));

            case SCREENTEXT_GROUP:

                if ((mem_s1>1) && (lexer->commandline[j][mem_s1-1] != '='))
                {
		          lexer->commandline[j][mem_s1] = ':';
		          delta=1;
                }
                memmove(lexer->commandline[j] + mem_s1 + delta, T, s0);
                lexer->commandline[j][mem_s1 + s0 + delta] = '=' ;
                mem_s1 += s0 + 1 + delta;
                first = 1;
                delta = 0;
                break;

            case STILLPICS_TITLE:
            case STILLOPTIONS_RANK:
	    case STILLPICS_TRACK:
            case STILLOPTIONS_TRACK:

                if (mem_s2>0)
		    lexer->commandline[j][mem_s2-1] = (dataflag == STILLPICS_TITLE)? ':' : ',' ;
		else {++i; ++j;}

                memmove(lexer->commandline[j] + mem_s2, T, s0);
                mem_s2 += s0 + 1;
                if (dataflag == STILLPICS_TITLE)   dataflag = STILLPICS_TRACK;
                if (dataflag == STILLOPTIONS_RANK) dataflag = STILLOPTIONS_TRACK;
                break;

            case SCREENTEXT_TRACK:
                memmove(lexer->commandline[j]+mem_s1+(!first), T, s0);
                if (!first) lexer->commandline[j][mem_s1] = ',' ;
                mem_s1 += s0 + (! first);
                first = 0;
                break;

            default:

                continue;
            }
           break;

        case SHORTOPTION:

                lexer->commandline[j][0] = '-';
                memmove(lexer->commandline[j] + 1, T, s0 + 1);
             break;
#ifdef LONG_OPTIONS
       case LONGOPTION:
                lexer->commandline[j][0] = lexer->commandline[j][1] = '-';
                memmove(lexer->commandline[j] + 2, T, s0 +1);

#endif
       }
            errno=0;

    }
    while (!feof(defaults) && (i < MAX_LEXER_LINES));
    lexer->nlines=(uint16_t) j+1;

    /* One needs to wipe out empty strings in command line.
      The following works yet is not ideal.
      Tightening up index incrementation above should in principle solve the issue whithut resorting to this code chunk below */

    char exch[lexer->nlines][MAX_OPTION_LENGTH*2];
    memset(&exch[0][0], 0, MAX_OPTION_LENGTH*2*lexer->nlines);
    u=0;
    for (j = 0; j < lexer->nlines; ++j)
    {

      if (lexer->commandline[j][0])  strcpy(exch[u],lexer->commandline[j]) ;
      else
	continue;
      u++;
    }

    for (j=0; j < u; ++j) strcpy(lexer->commandline[j], exch[j]);

    lexer->nlines=(uint16_t)u;

    clearerr(defaults);
    fclose(defaults);

    print_commandline(lexer->nlines, lexer->commandline, globals);

    return lexer;
}


