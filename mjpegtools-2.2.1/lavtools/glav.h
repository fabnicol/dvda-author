/** Header file generated with fdesign on Sun Mar  5 18:11:00 2000.**/

#ifndef GTK_xlav_h_
#define GTK_xlav_h_

/** Callbacks, globals and object handlers **/
extern void timeslider_cb(GtkAdjustment *, gpointer );
extern void timehscale_button_pressed_cb(GtkAdjustment *, gpointer );
extern void timehscale_button_released_cb(GtkAdjustment *, gpointer );
/* extern void timeslider_cb(GtkWidget*, long); */
extern void button_cb(GtkWidget*, long);
extern void rb_cb(GtkWidget *, long);
extern void Exit_cb(GtkWidget *, long);
extern void selection_cb(GtkWidget *, long);
extern void frame_skip_released(GtkWidget *, long );
extern void frame_skip_pressed(GtkWidget *, long );
extern gint key_press_cb(GtkWidget * , GdkEventKey* , gpointer data );
extern GtkWidget* glav_add_button(GtkWidget *, gchar *, GtkSignalFunc , gpointer);


/**** Forms and Objects ****/
typedef struct {
	GtkWidget *xlav;
	void *vdata;
	char *cdata;
	long  ldata;
	GtkObject *timeslider;
   GtkWidget *timehscale;
	GtkWidget *ss;
	GtkWidget *se;
	GtkWidget *stepr;
	GtkWidget *stepf;
	GtkWidget *fr;
	GtkWidget *rew;
	GtkWidget *stop;
	GtkWidget *play;
	GtkWidget *ff;
	GtkWidget *Timer;
	GtkWidget *Exit;
	GtkWidget *EditFrame;
	GtkWidget *TSelStart;
	GtkWidget *TSelEnd;
	GtkWidget *FSelStart;
	GtkWidget *FSelEnd;
	GtkWidget *BSSelStart;
	GtkWidget *BSSelEnd;
	GtkWidget *BClearSel;
	GtkWidget *BECut;
	GtkWidget *BECopy;
	GtkWidget *BEPaste;
	GtkWidget *BSaveAll;
	GtkWidget *BSaveSel;
	GtkWidget *StatDisp;
	GtkWidget *BGotoSelStart;
	GtkWidget *BGotoSelEnd;
} GTK_xlav;

extern GTK_xlav * create_form_xlav(void);
extern	void dispatch_input(void);
extern  void do_real_exit(int ID, void *data);
extern	void signal_cb(int signum, void *data);


#endif /* GTK_xlav_h_ */
