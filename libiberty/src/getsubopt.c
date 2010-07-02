

#include <stdlib.h>
#include <string.h>
#include <string.h>
#ifdef getsubopt
#undef getsubopt
#endif
#ifdef strchrnul
#undef strchrnul
#endif
#include "strchrnul.h"
#include "getsubopt.h"

int
getsubopt (char **optionp, char *const *tokens, char **valuep)
{
    char *endp, *vstart;
    int cnt;

    if (**optionp == '\0')
        return -1;

    /* Find end of next token.  */
    endp = strchrnul (*optionp, ',');

    /* Find start of value.  */
    vstart = memchr (*optionp, '=', endp - *optionp);
    if (vstart == NULL)
        vstart = endp;

    /* Try to match the characters between *OPTIONP and VSTART against
       one of the TOKENS.  */
    for (cnt = 0; tokens[cnt] != NULL; ++cnt)
        if (strncmp (*optionp, tokens[cnt], vstart - *optionp) == 0
                && tokens[cnt][vstart - *optionp] == '\0')
        {
            /* We found the current option in TOKENS.  */
            *valuep = vstart != endp ? vstart + 1 : NULL;

            if (*endp != '\0')
                *endp++ = '\0';
            *optionp = endp;

            return cnt;
        }

    /* The current suboption does not match any option.  */
    *valuep = *optionp;

    if (*endp != '\0')
        *endp++ = '\0';
    *optionp = endp;

    return -1;
}
