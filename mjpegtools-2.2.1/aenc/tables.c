#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include "common.h"
#include "encoder.h"

/***********************************************************************
 *
 * Read one of the data files ("alloc_*") specifying the bit allocation/
 * quatization parameters for each subband in layer II encoding
 *
 **********************************************************************/

#include "table_alloc.h"

int read_bit_alloc(table, alloc)        /* read in table, return # subbands */
int table;
al_table *alloc;
{
   int i, j, n;

   if( table>3 || table<0 ) table = 0;

   for(n=0;n<alloc_len[table];n++)
   {
      i = alloc_tab[table][n].i;
      j = alloc_tab[table][n].j;
      (*alloc)[i][j].steps = alloc_tab[table][n].steps;
      (*alloc)[i][j].bits  = alloc_tab[table][n].bits;
      (*alloc)[i][j].group = alloc_tab[table][n].group;
      (*alloc)[i][j].quant = alloc_tab[table][n].quant;
   }
   return alloc_sblim[table];
}
 
/************************************************************************
 *
 * read_ana_window()
 *
 * PURPOSE:  Reads encoder window file "enwindow" into array #ana_win#
 *
 ************************************************************************/
 
#include "table_enwindow.h"

void read_ana_window(ana_win)
double ana_win[HAN_SIZE];
{
    int i;

    for(i=0;i<512;i++) ana_win[i] = enwindow_tab[i];
}

/******************************************************************************
routine to read in absthr table from a file.
******************************************************************************/

#include "table_absthr.h"

void read_absthr(absthr, table)
FLOAT *absthr;
int table;
{
   int i;
   for(i=0; i<HBLKSIZE; i++) absthr[i] = absthr_tab[table][i];
}

int crit_band;
int *cbound;
int sub_size;

#include "table_cb.h"

void read_cbound(lay,freq)  /* this function reads in critical */
int lay, freq;              /* band boundaries                 */
{
 int i,n;

 n = (lay-1)*3 + freq;

 crit_band = cb_len[n];
 cbound = (int *) mem_alloc(sizeof(int) * crit_band, "cbound");
 for(i=0;i<crit_band;i++) cbound[i] = cb_tab[n][i];

}        

#include "table_th.h"

void read_freq_band(ltg,lay,freq)  /* this function reads in   */
int lay, freq;                     /* frequency bands and bark */
g_ptr *ltg;                /* values                   */
{
 int i,n;

 n = (lay-1)*3 + freq;

 sub_size = th_len[n];
 *ltg = (g_ptr ) mem_alloc(sizeof(g_thres) * sub_size, "ltg");
 (*ltg)[0].line = 0;          /* initialize global masking threshold */
 (*ltg)[0].bark = 0;
 (*ltg)[0].hear = 0;
 for(i=1;i<sub_size;i++){
    (*ltg)[i].line = th_tab[n][i-1].line;
    (*ltg)[i].bark = th_tab[n][i-1].bark;
    (*ltg)[i].hear = th_tab[n][i-1].hear;
 }
}
