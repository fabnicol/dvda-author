#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#if !defined HAVE_core_BUILD || !HAVE_core_BUILD
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "structures.h"
#include "c_utils.h"
#include "commonvars.h"
#include "menu.h"
#include "launch_manager.h"
#include "winport.h"
#include "auxiliary.h"

extern globalData globals;
extern uint16_t norm_x, norm_y, totntracks;
extern uint8_t maxbuttons, resbuttons;

int  generate_amgm_xml(uint8_t ngroups, uint8_t *ntracks, pic* img)
{

    errno=0;
    uint8_t arrowbuttons=1,buttons=0, menu=0, track=0, groupcount=0, group=0;
    uint8_t menubuttons;

    // Writing XML code
    FILE *xmlfile;

    if (globals.xml == NULL)
    {
        char xmlfilepath[strlen(globals.settings.tempdir)+8+STRLEN_SEPARATOR];
        memset(xmlfilepath, 0, sizeof(xmlfilepath));
        sprintf(xmlfilepath,"%s"SEPARATOR"%s", globals.settings.tempdir,"xmltemp");
        globals.xml=strdup(xmlfilepath);
    }

    xmlfile = fopen(globals.xml, "wb");
    if (xmlfile == NULL)        
    {
      EXIT_ON_RUNTIME_ERROR_VERBOSE("Could not open xmlfile")        
    }
            
    fprintf(xmlfile, "%s%s%s%s%s\n",
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\
<dvdauthor jumppad=\"1\">\n\
 <amgm>\n\
   <menus>\n\
   <video format=\"",img->norm,"\" />\n\
   <audio format=\"", img->audioformat, "\" lang=\"en\" />");
    fprintf(xmlfile, "%s\n","   <pgc>");
    do
    {
        if (img->hierarchical)
        {
            maxbuttons=(menu == 0)? ngroups: Min(MAX_BUTTON_Y_NUMBER-2,ntracks[groupcount]);
            resbuttons=0;
        }

        arrowbuttons=(menu < img->nmenus-1)+(menu > 0);
        menubuttons=(menu < img->nmenus-1)? maxbuttons : maxbuttons+resbuttons;

        if (img->hierarchical)
        {
            if (menu == 0)
            {
                do
                {
                    groupcount++;
                    buttons++;
                    fprintf(xmlfile, "       %s%02d%s%d%s\n", "<button name=\"button",groupcount, "\">jump menu ",groupcount+1,";</button>");
                }
                while (groupcount < ngroups);
                groupcount=0;
            }

            else if (groupcount < ngroups)
            {

                do
                {
                    buttons++;
                    track++;
                    fprintf(xmlfile, "       %s%02d%s%d%s%d%s\n", "<button name=\"button",buttons, "\">jump group ",groupcount+1," track ",track,";</button>");
                }
                while ((buttons < menubuttons) && (track < ntracks[groupcount]));

                if (track == ntracks[groupcount])
                {

                    groupcount++;
                    track=0;
                }
            }

        }
        else
        {
            do
            {
                do
                {
                    buttons++;
                    track++;
                    fprintf(xmlfile, "       %s%02d%s%d%s%d%s\n", "<button name=\"button",buttons, "\">jump group ",groupcount+1," track ",track,";</button>");

                }
                while ((buttons < menubuttons) && (track < ntracks[groupcount]));



                if (track == ntracks[groupcount])
                {
                    group++;
                    groupcount++;
                    track=0;
                }
                else
                {
                    break;  // changing menus without completing the liste of tracks in the same group
                }

            }
            while ((group < img->ncolumns)&& (groupcount < ngroups));
        }


        if ((img->nmenus > 1) &&(menu < img->nmenus))
        {

            do
            {

                if (menu < img->nmenus -1) fprintf(xmlfile, "       %s%02d%s%d%s\n", "<button name=\"button",++buttons, "\">jump menu ",menu+2,";</button>");
                if (menu)
                {
                    fprintf(xmlfile, "       %s%02d%s%d%s\n", "<button name=\"button",++buttons, "\">jump menu ",menu,";</button>");
                }
            }
            while  (buttons < menubuttons+arrowbuttons);
            fprintf(xmlfile, "%s%s%s\n", "       <vob pause=\"inf\" file=\"", img->topmenu[menu], "\"/>\n\
   </pgc>\n");


            if (menu < img->nmenus -1) fprintf(xmlfile, "%s\n","   <pgc>");
        }
        else
            fprintf(xmlfile, "%s%s%s\n", "       <vob pause=\"inf\" file=\"", img->topmenu[menu], "\"/>\n\
   </pgc>\n");

        menu++;
        buttons=0;
        group=0;
    }
    while ((menu < img->nmenus)&& (groupcount < ngroups));


    fprintf(xmlfile, "\
   </menus>\n\
 </amgm>\n\
</dvdauthor>\n");

    fclose(xmlfile);

    if (errno) foutput("%s\n", ERR "Could not generate Xml project file properly for generating DVD-Audio menu");
    else if (globals.debugging) foutput("%s\n", MSG_TAG "Xml dvdauthor project file was generated.");

    return(errno);
}






static inline void compute_coordinates(uint8_t ncol, uint8_t maxntracks, uint16_t* x0, uint16_t* y0, uint16_t* x1, uint16_t* y1)
{

    int i,j;
    uint16_t delta=0;

    delta=EVEN((norm_y-60)/((maxntracks+4)*2));
    x1[0]=EVEN(x(1,ncol))-12;
    x0[0]=EMPIRICAL_X_SHIFT+20-12;
    y0[0]=EVEN(y(1,maxntracks+4)-delta);
    y1[0]=EVEN(y(2,maxntracks+4)-delta);

    for (i=1; i < ncol; i++)
    {
        x1[i]=EVEN(x(i+1, ncol))-12 ;
        x0[i]=x1[i-1] ;
    }

    for ( j=1; j < maxntracks+2; j++)
    {
        y1[j]=EVEN(y(j+2, maxntracks+4)-delta);
        y0[j]=y1[j-1];
    }

}


int  generate_spumux_xml(uint8_t ngroups, uint8_t *ntracks, uint16_t maxntracks, pic* img)
{

    uint8_t buttons=0, arrowbuttons, menubuttons, menu=0, track=0, group=0, groupcount=0, offset=0;
    uint16_t x0[ngroups], y0[MAX_BUTTON_NUMBER], x1[ngroups], y1[MAX_BUTTON_NUMBER];
    errno=0;
    FILE *spu_xmlfile=NULL;
    if (globals.debugging) foutput(MSG_TAG "Max ntracks: %d\n", maxntracks);

    if (globals.spu_xml == NULL) globals.spu_xml=calloc(img->nmenus,sizeof(char *));
    if (globals.spu_xml == NULL) perror(ERR "spuxml\n");
    if (globals.debugging) foutput("%s\n", INF "Generating Xml project for spumux...");


    do
    {
        // Writing XML code

        if (globals.spu_xml[menu] == NULL)
        {
            char spu_xmlfilepath[strlen(globals.settings.tempdir)+20];
            memset(spu_xmlfilepath,0,sizeof(spu_xmlfilepath));
            sprintf(spu_xmlfilepath, "%s"SEPARATOR"%s%d%s", globals.settings.tempdir,"spu_xmltemp_",menu,".xml");
            globals.spu_xml[menu]=strdup(spu_xmlfilepath);
        }

        if (!globals.nooutput) spu_xmlfile=fopen(globals.spu_xml[menu], "wb");

        /*  We take a basic picture of 720x576 and divide it into a maximum of 3 columns (max 3 groups) and 20 tracks per group
         *  Left/Right Border= 20, top border=56 pixels, bottom border=16 pix. Inter-column spacing=20 pixels, inter-line spacing=12 pixels
         *  Let G be the number of groups and T the maximum of the number of titles in all groups,
         *  button(g, t(g)) the button for track t(g) in group g,
         *  the width of each button will be (720-2*20-(G-1)*20)/G=700/G-20 i.e. 680, 330 or 213 for PAL
         *  more generally: (norm_x-2*20-(G-1)*20)/G
         *  the height is: (576-72-(T-1)*12)/T=516/T-12 i.e a minimum of 7 pixels
         *  more generally: (norm_y-72-(T-1)*12)/T
         *  the coordinates of this button will be (x0,y0,x1,y1)=(left x,top y,right x,bottom y) :
         *   button(g, t(g))=floor(20+ 700*(g-1)/G, 56 + 516*(t(g)-1)/T, 700*g/G, 56 + 516*t(g)/T) */

        if (globals.debugging)     foutput(INF "Creating spumux xml file %s for menu %d\n", globals.spu_xml[menu], menu);

        if (spu_xmlfile == NULL)   foutput(ERR "spumux xml file %s for menu %d could not be opened\n", globals.spu_xml[menu], menu);

        fprintf(spu_xmlfile, "%s%s%s%s%s%s%s%s%s%s%s%s",
                "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\
<subpictures>\n\
  <stream>\n\
    <spu ", (img->highlightpic[menu])? " highlight=\"" : "",(img->highlightpic[menu])? img->highlightpic[menu] : "",(img->highlightpic[menu])? "\"" :""," force=\"yes\""," start=\"00:00:00.00\"",(img->selectpic[menu])? " select=\"":"",(img->selectpic[menu])?img->selectpic[menu]:"",(img->selectpic[menu])?"\"":"",(img->imagepic[menu])?" image=\"":"",(img->imagepic[menu])?img->imagepic[menu]:"", (img->imagepic[menu])?"\"":"");

        fprintf(spu_xmlfile, "%s\n", ">" );

        // We add group labels as non-buttons, so j->j+1 and maxnumttracsk->maxntracks+1

        if (img->hierarchical)
        {
            maxbuttons=(menu == 0)? ngroups: Min(MAX_BUTTON_Y_NUMBER-2,ntracks[groupcount]);
            resbuttons=0;
        }

        arrowbuttons=(menu < img->nmenus-1)+(menu > 0);
        menubuttons=(menu < img->nmenus-1)? maxbuttons : maxbuttons+resbuttons;
        compute_coordinates(img->ncolumns, maxntracks, x0, y0, x1, y1);

        if (img->hierarchical)
        {

            if (menu == 0)
            {
                do
                {
                    buttons++;
                    fprintf(spu_xmlfile, "%s%d%s%s%d%s%s%d%s%s%02d%s%s%d%s\n", "       <button x0=\"", x0[0],"\""," y0=\"",y0[groupcount],"\""," x1=\"",x1[0],"\""," name=\"button", buttons, "\""," y1=\"",y1[groupcount],"\"/>");
                    groupcount++;
                }
                while (groupcount < ngroups);
                groupcount=0;
            }
            else if (groupcount < ngroups)
            {
                do
                {
                    buttons++;
                    fprintf(spu_xmlfile, "%s%d%s%s%d%s%s%d%s%s%02d%s%s%d%s\n", "       <button x0=\"", x0[group],"\""," y0=\"",y0[track],"\""," x1=\"",x1[group],"\""," name=\"button", buttons, "\""," y1=\"",y1[track],"\"/>");
                    track++;
                }
                while ((buttons < menubuttons) && (track < ntracks[groupcount]));

                if (track == ntracks[groupcount])
                {
                    groupcount++;
                    track=0;
                }
            }
        }
        else
        {

            do
            {
                offset=track;
                do
                {
                    buttons++;
                    fprintf(spu_xmlfile, "%s%d%s%s%d%s%s%d%s%s%02d%s%s%d%s\n", "       <button x0=\"", x0[group],"\""," y0=\"",y0[track-offset],"\""," x1=\"",x1[group],"\""," name=\"button", buttons, "\""," y1=\"",y1[track-offset],"\"/>");
                    track++;
                }
                while ((buttons < menubuttons) && (track < ntracks[groupcount]));

                if (track == ntracks[groupcount])
                {
                    group++;
                    groupcount++;
                    track=0;
                    offset=0;
                }
                else
                {
                    break;  // changing menus without completing the liste of tracks in the same group
                }
            }
            while ((group < img->ncolumns)&& (groupcount < ngroups));
        }

        if (img->nmenus > 1)
        {
            do
            {
                buttons++;
                fprintf(spu_xmlfile, "%s%d%s%s%d%s%s%d%s%s%02d%s%s%d%s\n", "       <button x0=\"", x0[img->ncolumns-1],"\""," y0=\"",y0[maxntracks],"\""," x1=\"",x1[img->ncolumns-1],"\""," name=\"button", buttons, "\""," y1=\"",y1[maxntracks],"\"/>");

                if ((menu) && (menu < img->nmenus-1))
                {
                    buttons++;
                    fprintf(spu_xmlfile, "%s%d%s%s%d%s%s%d%s%s%02d%s%s%d%s\n", "       <button x0=\"",x0[img->ncolumns-1],"\""," y0=\"",y0[maxntracks+1],"\""," x1=\"",x1[img->ncolumns-1],"\""," name=\"button", buttons, "\""," y1=\"",y1[maxntracks+1],"\"/>");
                }
            }
            while  (buttons < menubuttons+arrowbuttons);
        }
        menu++;
        buttons=0;
        group=0;

        fprintf(spu_xmlfile, "%s\n", "    </spu>\n\
  </stream>\n\
</subpictures>\n");

        fclose(spu_xmlfile);
    }
    while ((menu < img->nmenus)&& (groupcount < ngroups));


    if (errno) foutput("%s\n", ERR "Could not generate spumux xml project file properly for generating DVD-Audio menu");
    else if (globals.debugging) foutput("%s\n", MSG_TAG "spumux xml dvdauthor project file was generated.");

    return(errno);
}
#endif
