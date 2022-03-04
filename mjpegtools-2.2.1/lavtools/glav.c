#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include "glav.h"

static GtkWidget* glav_create_button(GtkWidget * parent, const gchar * label, const gchar * tipstring, GtkSignalFunc callback, gpointer val) {
   GtkWidget * obj;
   GtkTooltips * tooltip;
   obj = gtk_button_new_with_label(label);
   gtk_box_pack_start(GTK_BOX(parent),obj, TRUE, TRUE, 0); 

   g_signal_connect(G_OBJECT(obj),"clicked",callback,val);

   tooltip = gtk_tooltips_new();
   gtk_tooltips_set_tip(tooltip, obj, tipstring, NULL);

   gtk_widget_show(obj);

   return obj;
}

static GtkWidget* glav_create_label(GtkWidget * parent, const gchar * label) {
   GtkWidget * obj;
   obj = gtk_label_new(label);
   gtk_box_pack_start(GTK_BOX(parent),obj, TRUE, TRUE, 0); 
   gtk_widget_show(obj);
   return obj;
}

GTK_xlav *create_form_xlav(void)
{
  GtkWidget *temphbox, *tempbighbox;
  GtkWidget *vbox;
  GtkWidget *obj;
  GTK_xlav *gui = (GTK_xlav *) malloc(sizeof(*gui));

  gui->xlav = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (gui->xlav), "key_press_event",
    GTK_SIGNAL_FUNC (key_press_cb), (gpointer)0);

  vbox = gtk_vbox_new(TRUE,4);
  /* first row */
  temphbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), temphbox, TRUE, TRUE, 0);
  gui->timeslider=gtk_adjustment_new (0.0, 0.0, 100.0, 0.01, 2.0, .10);
  gui->timehscale=gtk_hscale_new (GTK_ADJUSTMENT (gui->timeslider));
  gtk_scale_set_draw_value(GTK_SCALE(gui->timehscale),FALSE);
  gtk_range_set_update_policy(GTK_RANGE(gui->timehscale),GTK_UPDATE_CONTINUOUS);
    gtk_box_pack_start(GTK_BOX(temphbox), gui->timehscale, TRUE, TRUE, 0);
  gtk_widget_show(gui->timehscale);

  gtk_signal_connect (GTK_OBJECT (gui->timeslider), "value_changed",
    GTK_SIGNAL_FUNC (timeslider_cb), (gpointer)0);

  gtk_widget_show(temphbox);

  /* second row */
  tempbighbox=gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(vbox), tempbighbox, TRUE, TRUE, 0);
  temphbox = gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start(GTK_BOX(tempbighbox), temphbox, TRUE, TRUE, 0);
  /* gtk_box_pack_start(GTK_BOX(vbox), temphbox, TRUE, TRUE, 0); */

   gui->ss = obj = glav_create_button(temphbox,"|<","Skip to Start",GTK_SIGNAL_FUNC(button_cb),(gpointer)1);
   gui->fr = obj = glav_create_button(temphbox,"<<","Fast Reverse",GTK_SIGNAL_FUNC(rb_cb),(gpointer)1);
   gui->rew = obj = glav_create_button(temphbox,"<","Play Reverse",GTK_SIGNAL_FUNC(rb_cb),(gpointer)2);
   gui->stop = obj = glav_create_button(temphbox,"||","Pause",GTK_SIGNAL_FUNC(rb_cb),(gpointer)3);
   gui->play = obj = glav_create_button(temphbox,">","Play Forward",GTK_SIGNAL_FUNC(rb_cb),(gpointer)4);
   gui->ff = obj = glav_create_button(temphbox,">>","Fast Forward",GTK_SIGNAL_FUNC(rb_cb),(gpointer)5);
   gui->se = obj = glav_create_button(temphbox,">|","Skip to End",GTK_SIGNAL_FUNC(button_cb),(gpointer)2);
   gui->stepr = obj = glav_create_button(temphbox,"<|","Frame Reverse",GTK_SIGNAL_FUNC(rb_cb),(gpointer)0);
     /* unmap the click, map press and release */
    g_signal_connect(G_OBJECT(obj),"pressed",G_CALLBACK(frame_skip_pressed),(gpointer)3);
    g_signal_connect(G_OBJECT(obj),"released",G_CALLBACK(frame_skip_released),(gpointer)3);
  gui->stepf = obj = glav_create_button(temphbox,"|>","Frame Forward",GTK_SIGNAL_FUNC(button_cb),(gpointer)0);
     /* unmap the click, map press and release */
    g_signal_connect(G_OBJECT(obj),"pressed",G_CALLBACK(frame_skip_pressed),(gpointer)4);
    g_signal_connect(G_OBJECT(obj),"released",G_CALLBACK(frame_skip_released),(gpointer)4);

  /*
  gtk_widget_show(temphbox);
  temphbox = gtk_hbox_new(TRUE, 0);
  gtk_box_pack_start(GTK_BOX(tempbighbox), temphbox, TRUE, TRUE, 0);
  */

  gui->StatDisp = obj = glav_create_label(tempbighbox,"Play");
  gui->Timer = obj = glav_create_label(tempbighbox,"0:00:00:00");

  /* temphbox = gtk_hbox_new(TRUE, 0); */
  /* gtk_box_pack_start(GTK_BOX(tempbighbox), temphbox, TRUE, TRUE, 0); */

  /* gui->Exit = obj = glav_create_button(temphbox,"Exit","Exit Glav",GTK_SIGNAL_FUNC(Exit_cb),(gpointer)1); */
  gui->Exit = obj = glav_create_button(tempbighbox,"Exit","Exit Glav",GTK_SIGNAL_FUNC(Exit_cb),(gpointer)1);

  /* third row  */
  gtk_widget_show(temphbox);
  gtk_widget_show(tempbighbox);
  temphbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), temphbox, TRUE, TRUE, 0);

  gui->TSelStart = obj = glav_create_label(temphbox,"Select.\nStart:");
  gui->FSelStart = obj = glav_create_label(temphbox,"-:--:--:--");
  gui->TSelEnd = obj = glav_create_label(temphbox,"Select.\nEnd:");
  gui->FSelEnd = obj = glav_create_label(temphbox,"-:--:--:--");
  gtk_widget_set_size_request(GTK_WIDGET(gui->FSelStart), 100, -1);
  gtk_widget_set_size_request(GTK_WIDGET(gui->FSelEnd), 100, -1);

  gui->BSSelStart = obj = glav_create_button(temphbox,"|<-","Select Start",GTK_SIGNAL_FUNC(selection_cb),(gpointer)1);
  gui->BSSelEnd = obj = glav_create_button(temphbox,"->|","Select End",GTK_SIGNAL_FUNC(selection_cb),(gpointer)2);
  gui->BGotoSelStart = obj = glav_create_button(temphbox,"|<","Skip to Selected Start",GTK_SIGNAL_FUNC(selection_cb),(gpointer)11);
  gui->BGotoSelEnd = obj = glav_create_button(temphbox,">|","Skip to Selected End",GTK_SIGNAL_FUNC(selection_cb),(gpointer)12);
  gui->BClearSel = obj = glav_create_button(temphbox,"X","Clear Selection",GTK_SIGNAL_FUNC(selection_cb),(gpointer)3);
  gui->BECut = obj = glav_create_button(temphbox,"Cut","Cut Selected",GTK_SIGNAL_FUNC(selection_cb),(gpointer)4);
  gui->BECopy = obj = glav_create_button(temphbox,"Copy","Copy Selected",GTK_SIGNAL_FUNC(selection_cb),(gpointer)5);
  gui->BEPaste = obj = glav_create_button(temphbox,"Paste","Paste Selected",GTK_SIGNAL_FUNC(selection_cb),(gpointer)6);
  gui->BSaveAll = obj = glav_create_button(temphbox,"Save all ...","Save All",GTK_SIGNAL_FUNC(selection_cb),(gpointer)7);
  gui->BSaveSel = obj = glav_create_button(temphbox,"Save ...","Save Selected",GTK_SIGNAL_FUNC(selection_cb),(gpointer)8);

  /* end */

  gtk_widget_show(temphbox);
  gtk_container_add(GTK_CONTAINER(gui->xlav),vbox);
  gtk_widget_show(vbox);

  return gui;
}
/*---------------------------------------*/
