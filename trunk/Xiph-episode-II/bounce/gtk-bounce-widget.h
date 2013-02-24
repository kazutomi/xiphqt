#ifndef _BOUNCE_WIDGET_H_
#define _BOUNCE_WIDGET_H_

#include <gtk/gtk.h>
#include "twopole.h"

typedef struct rowpanel {
  int w;
  int h;
  GtkWidget *toplevel;

  int num_panels;
  int current_panel;
  pole2 display_filter;
  double display_now;
  int display_animating;
  GtkWidget *panel_scrollfix;
  GtkWidget *panel_lefttable;
  GtkWidget *upbutton;
  GtkWidget *downbutton;
  void (*callback)(struct rowpanel *);
} rowpanel;

typedef struct rowwidget {
  GtkBox *parent;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *vbox;
  GtkWidget *label[5];
  GtkWidget *alignment;
  int radius;
  int rounding;
  void (*callback)(struct rowwidget *);
  int pressed;
  int lit;
} rowwidget;

typedef struct rowslider {
  GtkBox *parent;
  GtkWidget *button;

  PangoLayout *caption;
  PangoLayout **labels;
  float *values;
  int *stops;
  int label_n;
  int capwidth;
  int labelwidth;
  int labelheight;

  float value;
  float delta;
  int x;
  void (*callback)(struct rowslider *);


  double radiusS;
  double radiusL;
  double lpad;
  double pastdot;
  double pipw;
  double pipx;
  int w;
  int h;
  int y0;
  int y1;

} rowslider;

extern GtkWidget *bounce_toplevel(void);

extern rowwidget *rowtoggle_new(GtkBox *box, char *text,
                                void (*callback)(rowwidget *));
extern void rowwidget_add_label(rowwidget *t, char *label, int row);

extern int rowtoggle_get_active(rowwidget *w);
extern void rowtoggle_set_active(rowwidget *w, int state);
extern rowwidget *rowreadout_new(GtkBox *box, char *text);
extern void rowreadout_light(rowwidget *t,int state);
extern rowwidget *rowspacer_new(GtkBox *box,int pad);
extern rowwidget *rowlabel_new(GtkBox *box,char *text);
extern rowslider *rowslider_new(GtkBox *box,
                                char *caption,
                                void (*callback)(rowslider *));
extern void rowslider_add_stop(rowslider *t,
                               char *ltext,
                               float lvalue,
                               int snap);
extern void rowslider_set(rowslider *t,float lvalue);
extern rowpanel *rowpanel_new(int w, int h, void (*callback)(rowpanel *));
extern GtkBox *rowpanel_new_row(rowpanel *p, int fill_p);

#endif
