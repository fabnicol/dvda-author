/* ==========================================================================
*
*   auxiliary.c
*   originally designed by Pigiron, 2007.
*   current rewrite: Copyright Fabrice Nicol, 2008.
*
*   Description
*        Auxiliary input-output subs
* ========================================================================== */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "libiberty.h"
#include "fixwav.h"
#include "fixwav_manager.h"
#include "fixwav_auxiliary.h"
#include "c_utils.h"
#include "structures.h"

extern globalData globals;

/*********************************************************************
* Function: isok
*
* Purpose:  This function displays a yes/no prompt
*********************************************************************/

_Bool isok()
{

    char buf[FIXBUF_LEN]={0};

  get_input(buf);

  printf("%c", '\n');

  // With silent mode, replies are implicitly OK.
  switch (toupper(buf[0]))
    {
    case 'Y':
      return 1;
      break;
    case 'N' :
      return 0;
      break;

    default  :
      fprintf(stderr, "%s\n", "[WAR]  Unknown--Enter reply again");
      return(isok());
    }
}

/*********************************************************************/
/* Function: get_input                                               */
/*                                                                   */
/* Purpose:  This function performs a "safe" read of the user's      */
/*           input and puts the results in the caller's buffer       */
/*********************************************************************/
void get_input( char* buf )
{
  if (fgets(buf, FIXBUF_LEN, stdin) == NULL)
    printf("%s\n", "[ERR]  fgets crash");
  return;
}

