#include "gtk-bounce.h"
#include "gtk-bounce-widget.h"

/* implement a pseudowidget (rowwidget) for most entries in the panel.
   We want a darm gtk3-ish look, though we're actually using gtk2. */

/* Because we're in a GtkFixed that's being scrolled around, but
   drawing is being done to the GdkWindow owned by the toplevel, the
   clip region is set incorrectly to allow us to overdraw things in
   the toplevel outside our parents' bounds.  Follow the tree up and
   reclip. */

static void narrow_clip(GtkWidget *w,GdkRectangle *area){
  int x1 = w->allocation.x;
  int y1 = w->allocation.y;
  int x2 = w->allocation.x+w->allocation.width;
  int y2 = w->allocation.y+w->allocation.height;

  if(x1>area->x)area->x=x1;
  if(x2<area->width)area->width=x2;
  if(y1>area->y)area->y=y1;
  if(y2<area->height)area->height=y2;

  if(w->parent && w->parent->window == w->window)
    narrow_clip(w->parent,area);
}

static void clip_to_ancestors(GtkWidget *w, cairo_t *cr){
  GdkRectangle a;
  a.x = w->allocation.x;
  a.width = a.x + w->allocation.width; /* overload */
  a.y = w->allocation.y;
  a.height = a.y + w->allocation.height; /* overload */
  if(w->parent && w->parent->window == w->window)
    narrow_clip(w->parent,&a);
  cairo_rectangle(cr,a.x,a.y,a.width-a.x,a.height-a.y);
  cairo_clip(cr);
}

static void expose_a_widget(gpointer a, gpointer b){
  GtkWidget *widget = (GtkWidget *)a;
  GdkEventExpose *event = (GdkEventExpose *)b;
  GTK_WIDGET_CLASS(GTK_WIDGET_GET_CLASS(widget))->
    expose_event (widget, event);
}

static gboolean expose_rectarea(GtkWidget *widget,
                                GdkEventExpose *event,
                                gpointer userdata){
  if(!widget->window)return TRUE;
  cairo_t *cr = gdk_cairo_create(widget->window);
  rowwidget *t = (rowwidget *)userdata;
  int w=widget->allocation.width;
  int h=widget->allocation.height;
  GtkStateType state = gtk_widget_get_state(widget);
  int lit=t->lit;
  /* rounded rectangle path */

  double degrees = M_PI / 180.0;
  double radius = t->radius;
  double rounding = t->rounding;
  double height = h-2;
  double rxd = radius-cos(rounding*degrees)*radius;
  double ryd = sin(rounding*degrees)*radius;
  double R = (height/2-radius+ryd)/sin(rounding*degrees);
  double RxD = (isinf(R)?0:R-cos(rounding*degrees)*R);

  double x = widget->allocation.x+RxD-rxd+1;
  double width = w - RxD*2 + rxd*2 - 2;
  double y = widget->allocation.y+1;

  double Rx1 = widget->allocation.x+R+1;
  double Rx2 = widget->allocation.x+w-R-1;

  if(state == GTK_STATE_INSENSITIVE){
    y+=.5;
    x+=.5;
    height-=1;
    width-=1;
  }

  clip_to_ancestors(widget,cr);

  cairo_new_sub_path (cr);
  cairo_arc (cr, x+width-radius, y+radius, radius, -90*degrees, -rounding*degrees);
  if(!isinf(R))
    cairo_arc (cr, Rx2, y+height/2, R, -rounding*degrees, rounding*degrees);
  cairo_arc (cr, x+width-radius, y+height-radius, radius, rounding*degrees, 90*degrees);

  cairo_arc (cr, x+radius, y+height-radius, radius, 90*degrees, (180-rounding)*degrees);
  if(!isinf(R))
    cairo_arc (cr, Rx1, y+height/2, R, (180-rounding)*degrees, (180+rounding)*degrees);
  cairo_arc (cr, x+radius, y+radius, radius, (180+rounding)*degrees, 270*degrees);
  cairo_close_path (cr);

  /* fill background */
  if(state != GTK_STATE_INSENSITIVE){
    /* won't get here is not togglebutton */
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
      cairo_set_source_rgba (cr, .1, .3, .6, 1);
    else
      cairo_set_source_rgba (cr, .1, .1, .1, 1);
  }else{
    if(lit)
      cairo_set_source_rgba (cr, .18, .5, 1, .3);
    else
      cairo_set_source_rgba (cr, 0, 0, 0, .2);
  }
  cairo_fill_preserve (cr);

  /* gradient */
  if(state != GTK_STATE_INSENSITIVE && !t->pressed){
    cairo_pattern_t *pat = cairo_pattern_create_linear(x, y, x, y+h);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0, 0, 0, .2);
    cairo_pattern_add_color_stop_rgba(pat, 0.8, 0, 0, 0, 0);
    cairo_pattern_add_color_stop_rgba(pat, 0.3, 1, 1, 1, 0);
    cairo_pattern_add_color_stop_rgba(pat, 0, 1, 1, 1, .15);
    cairo_set_source(cr, pat);
    cairo_fill_preserve (cr);
    cairo_pattern_destroy(pat);
  }

  /* stroke border */
  switch(state){
  case GTK_STATE_ACTIVE:
  case GTK_STATE_NORMAL:
    cairo_set_source_rgba (cr, .8, .8, .8, 1);
    cairo_set_line_width(cr,2.0);
    break;
  case GTK_STATE_PRELIGHT:
  case GTK_STATE_SELECTED:
    cairo_set_source_rgba (cr, 1, 1, 1, 1);
    cairo_set_line_width(cr,2.0);
    break;
  case GTK_STATE_INSENSITIVE:
    if(lit)
        cairo_set_source_rgba (cr, .18, .5, 1, .9);
      else
        cairo_set_source_rgba (cr, 1, 1, 1, .5);
    cairo_set_line_width(cr,1.0);
    break;
  }

  cairo_stroke(cr);
  cairo_destroy(cr);

  return TRUE;
}

static void rowslider_update_size(rowslider *t){
  int i,w,h;

  pango_layout_get_pixel_size(t->caption,&w,&h);
  t->capwidth = w;
  t->labelheight = h*1.2;

  for(i=0;i<t->label_n;i++){
    pango_layout_get_pixel_size(t->labels[i],&w,&h);
    if(w>t->labelwidth)t->labelwidth=w;
  }

  gtk_widget_set_size_request
    (t->button,
     t->capwidth + t->labelwidth*t->label_n*1.5,
     t->labelheight*2.8);

  while (gtk_events_pending())
   gtk_main_iteration();

  w = t->w = t->button->allocation.width;
  h = t->h = t->button->allocation.height;

  int nh = t->labelheight*2.5;
  t->y0 = (h-nh)/2;
  t->y1 = t->y0 + nh;

  double capw = t->capwidth;
  double cellw = t->labelwidth;

  t->radiusS = (nh-t->labelheight)/4;
  t->radiusL = (nh-t->labelheight)/2-1.5;
  t->lpad = t->radiusL;

  t->pastdot = (t->radiusL*2-cellw/2 > 0 ?
                t->radiusL*2-cellw/2 : 0);

  double padw = (w-capw-t->lpad-t->pastdot-1) / t->label_n - cellw;

  t->pipw = cellw+padw;
  t->pipx = t->lpad+capw+padw+(cellw/2);
  t->pastdot += cellw/2;

}

static gboolean expose_slider(GtkWidget *widget,
                                GdkEventExpose *event,
                                gpointer userdata){
  if(!widget->window)return TRUE;
  cairo_t *cr = gdk_cairo_create(widget->window);
  rowslider *t = (rowslider *)userdata;
  int w = widget->allocation.width;
  int h = widget->allocation.height;
  int x = widget->allocation.x;
  int y = widget->allocation.y;
  GtkStateType state = gtk_widget_get_state(widget);

  if(t->w!=w || t->h!=h)
    rowslider_update_size(t);

  double dif = (t->radiusL-t->radiusS);
  double pipw = t->pipw;
  double pipx = x+t->pipx;

  /* rounded slider track */
  double degrees = M_PI / 180.0;
  double yy = y + t->y0 + t->labelheight+t->radiusL;
  double sx = pipx + (t->label_n-1)*pipw;

  clip_to_ancestors(widget,cr);

  cairo_new_sub_path (cr);
  cairo_arc (cr, pipx+(t->label_n-1)*t->pipw+t->pastdot-t->radiusL,
               yy, t->radiusS, -90*degrees, 90*degrees);
  cairo_arc (cr, x+dif+t->radiusS, yy, t->radiusS, 90*degrees, 270*degrees);
  cairo_close_path (cr);

  cairo_set_source_rgba (cr, .1, .1, .1, 1);
  cairo_fill_preserve(cr);

  cairo_set_source_rgba (cr, 1, 1, 1, .6);
  cairo_set_line_width(cr,1.0);
  cairo_stroke(cr);

  /* pips */
  int i;
  cairo_set_source_rgba (cr, 1, 1, 1, .5);
  for(i=0;i<t->label_n;i++){
    sx = pipx + i*pipw;
    cairo_new_sub_path (cr);
    cairo_move_to(cr,rint(sx)+.5, yy-t->radiusL);
    cairo_line_to(cr,rint(sx)+.5, yy-t->radiusS);
    cairo_move_to(cr,rint(sx)+.5, yy+t->radiusL);
    cairo_line_to(cr,rint(sx)+.5, yy+t->radiusS);
  }
  cairo_stroke(cr);

  /* slider grip */
  sx = pipx + t->delta*pipw;

  cairo_new_sub_path (cr);
  cairo_arc (cr, sx + t->pastdot - t->radiusL-1, yy,
             t->radiusL, -90*degrees, 90*degrees);
  cairo_arc (cr, x+t->radiusL+1, yy, t->radiusL, 90*degrees, 270*degrees);
  cairo_close_path (cr);

  cairo_set_source_rgba (cr, .1, .3, .6, 1);
  cairo_fill_preserve(cr);

  if(state!=GTK_STATE_ACTIVE){
    cairo_pattern_t *pat =
      cairo_pattern_create_linear(x, y+t->y0+t->labelheight, x, y+t->y1);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0, 0, 0, .3);
    cairo_pattern_add_color_stop_rgba(pat, 0.6, 0, 0, 0, 0);
    cairo_pattern_add_color_stop_rgba(pat, 0.4, 1, 1, 1, 0);
    cairo_pattern_add_color_stop_rgba(pat, 0, 1, 1, 1, .2);
    cairo_set_source(cr, pat);
    cairo_fill_preserve (cr);
    cairo_pattern_destroy(pat);
  }

  /* slider pips */
  cairo_save(cr);
  cairo_clip(cr);

  cairo_set_source_rgba(cr,.9, .9, .9, .8);

  for(i=0;i<t->label_n;i++){
    double psx = pipx + i*pipw;
    cairo_move_to(cr,rint(psx)+.5, yy-t->radiusL);
    cairo_line_to(cr,rint(psx)+.5, yy-t->radiusL*.6);

    cairo_move_to(cr,rint(psx)+.5, yy+t->radiusL);
    cairo_line_to(cr,rint(psx)+.5, yy+t->radiusL*.6);

  }
  cairo_stroke(cr);
  cairo_restore(cr);

  cairo_new_sub_path (cr);
  cairo_arc (cr, sx + t->pastdot - t->radiusL-1, yy,
             t->radiusL, -90*degrees, 90*degrees);
  cairo_arc (cr, x+t->radiusL+1, yy, t->radiusL, 90*degrees, 270*degrees);
  cairo_close_path (cr);

  switch(state){
  default:
  case GTK_STATE_ACTIVE:
  case GTK_STATE_NORMAL:
    cairo_set_source_rgba (cr, .8, .8, .8, 1);
    cairo_set_line_width(cr,2.0);
    break;
  case GTK_STATE_PRELIGHT:
  case GTK_STATE_SELECTED:
    cairo_set_source_rgba (cr, 1, 1, 1, 1);
    cairo_set_line_width(cr,2.0);
    break;
  }

  cairo_stroke(cr);

  /* slider dot */
  cairo_new_sub_path (cr);
  cairo_arc (cr, rint(sx)+.5, yy, 3, 0*degrees, 360*degrees);
  cairo_fill(cr);


  /* labels */
  {
    int w,d;

    cairo_set_source_rgba (cr, 1, 1, 1, 1);
    cairo_move_to(cr,x+t->lpad,y+t->y0);
    pango_cairo_show_layout(cr,t->caption);

    for(i=0;i<t->label_n;i++){
      sx = pipx + i*pipw;
      pango_layout_get_pixel_size(t->labels[i],&w,&d);
      cairo_move_to(cr,sx-w/2,y+t->y0);
      pango_cairo_show_layout(cr,t->labels[i]);
    }
  }

  cairo_destroy(cr);
  return TRUE;
}

static gboolean expose_rowwidget(GtkWidget *widget,
                                 GdkEventExpose *event,
                                 gpointer userdata){
  rowwidget *t = (rowwidget *)userdata;

  /* be sure of the expose order as the widgets overlap */
  /* toggle button is not a class native expose, call the draw directly */
  expose_rectarea(t->button,event,t);
  /* expose labels */
  expose_a_widget(t->vbox,event);

  return TRUE;
}

/* modify toggle button behavior that assumes an activated button ==
   pressed.  I don't want active toggles to be down, just lit */
static gboolean press_rowtoggle(GtkWidget *widget,
                                gpointer userdata){
  rowwidget *t = (rowwidget *)userdata;
  t->pressed=1;
  return FALSE;
}
static gboolean release_rowtoggle(GtkWidget *widget,
                                gpointer userdata){
  rowwidget *t = (rowwidget *)userdata;
  t->pressed=0;
  return FALSE;
}

static gboolean toggle_rowtoggle(GtkWidget *widget,
                                 gpointer userdata){
  rowwidget *t = (rowwidget *)userdata;
  if(t->callback)t->callback(t);
  return FALSE;
}

void rowwidget_add_label(rowwidget *t, char *label, int row){
  if(!t)return;
  if(row<0 || row>4)return;
  if(t->label[row] && label==NULL){
    gtk_widget_destroy(t->label[row]);
    t->label[row]=NULL;
  }else if(label){
    if(!t->label[row]){
      t->label[row]=gtk_label_new(NULL);
      gtk_box_pack_start(GTK_BOX(t->vbox),t->label[row],0,0,0);
      gtk_widget_show(t->label[row]);
    }
    gtk_label_set_markup(GTK_LABEL(t->label[row]),label);
  }
}

rowwidget *rowtoggle_new(GtkBox *box, char *text,
                         void (*callback)(rowwidget *)){
  rowwidget *ret = calloc(1,sizeof(*ret));
  ret->rounding = 15;
  ret->radius = 12;
  ret->parent = box;
  ret->table = gtk_table_new(1,1,1);
  ret->button = gtk_toggle_button_new();
  ret->alignment = gtk_alignment_new(.5,.5,0,0);
  ret->vbox = gtk_vbox_new(0,2);
  ret->callback = callback;

  memset(ret->label,0,sizeof(ret->label));

  gtk_box_pack_start(box,ret->table,0,0,0);
  gtk_table_attach_defaults(GTK_TABLE(ret->table),ret->alignment,0,1,0,1);
  gtk_container_add(GTK_CONTAINER(ret->alignment),ret->vbox);
  gtk_table_attach_defaults(GTK_TABLE(ret->table),ret->button,0,1,0,1);

  g_signal_connect(G_OBJECT(ret->button), "expose-event",
                   G_CALLBACK(expose_rectarea), ret);
  g_signal_connect(G_OBJECT(ret->button), "pressed",
                   G_CALLBACK(press_rowtoggle), ret);
  g_signal_connect(G_OBJECT(ret->button), "released",
                   G_CALLBACK(release_rowtoggle), ret);
  g_signal_connect(G_OBJECT(ret->button), "toggled",
                   G_CALLBACK(toggle_rowtoggle), ret);
  g_signal_connect(G_OBJECT(ret->table), "expose-event",
                 G_CALLBACK(expose_rowwidget), ret);
  gtk_widget_set_size_request(ret->button,
                              GTK_WIDGET(box)->allocation.height*1.5,-1);

  if(text)
    rowwidget_add_label(ret, text, 0);

  gtk_widget_show_all(ret->table);
  return ret;
}

void rowtoggle_set_active(rowwidget *w, int state){
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w->button),state);
}

int rowtoggle_get_active(rowwidget *w){
  return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w->button));
}

rowwidget *rowreadout_new(GtkBox *box, char *text){
  rowwidget *ret = calloc(1,sizeof(*ret));
  ret->rounding = 0;
  ret->radius = 6;
  ret->parent = box;
  ret->table = gtk_table_new(1,1,1);
  ret->button = gtk_toggle_button_new();
  ret->alignment = gtk_alignment_new(.5,.5,0,0);
  ret->vbox = gtk_vbox_new(0,2);

  memset(ret->label,0,sizeof(ret->label));

  gtk_box_pack_start(box,ret->table,0,0,0);
  gtk_table_attach_defaults(GTK_TABLE(ret->table),ret->alignment,0,1,0,1);
  gtk_container_add(GTK_CONTAINER(ret->alignment),ret->vbox);
  gtk_table_attach_defaults(GTK_TABLE(ret->table),ret->button,0,1,0,1);
  gtk_widget_set_sensitive(ret->button,FALSE);

  g_signal_connect(G_OBJECT(ret->button), "expose-event",
                   G_CALLBACK(expose_rectarea), ret);
  g_signal_connect(G_OBJECT(ret->table), "expose-event",
                 G_CALLBACK(expose_rowwidget), ret);
  gtk_widget_set_size_request(ret->button,GTK_WIDGET(box)->allocation.height*1.8,-1);

  if(text)
    rowwidget_add_label(ret, text, 0);

  gtk_widget_show_all(ret->table);
  return ret;
}

void rowreadout_light(rowwidget *t,int state){
  GdkRectangle rect;
  t->lit=state;
  if(t->button->window){
    rect.x = t->button->allocation.x;
    rect.y = t->button->allocation.y;
    rect.width = t->button->allocation.width;
    rect.height = t->button->allocation.height;
    gdk_window_invalidate_rect(t->button->window,&rect,1);
  }
}

rowwidget *rowspacer_new(GtkBox *box,int pad){
  rowwidget *ret = calloc(1,sizeof(*ret));
  ret->parent = box;
  ret->alignment = gtk_alignment_new(.5,.5,0,0);

  gtk_box_pack_start(box,ret->alignment,0,0,0);

  gtk_widget_set_size_request(ret->alignment,pad,-1);

  gtk_widget_show_all(ret->alignment);
  return ret;
}

rowwidget *rowlabel_new(GtkBox *box,char *text){
  rowwidget *ret = calloc(1,sizeof(*ret));
  ret->vbox = gtk_vbox_new(1,2);
  ret->parent = box;

  memset(ret->label,0,sizeof(ret->label));
  gtk_box_pack_start(box,ret->vbox,0,0,0);
  gtk_widget_set_name(ret->vbox,"rowlabel");

  if(text)
    rowwidget_add_label(ret, text, 0);
  gtk_widget_show_all(ret->vbox);
  return ret;
}

static void slider_set(rowslider *t, int ex){

  t->delta = (ex - t->pipx)/t->pipw;

  if(t->delta<0)t->delta=0;
  if(t->delta>t->label_n-1)t->delta=t->label_n-1;

  int i=floor(t->delta);
  if(t->stops[i]){
    if(t->delta-i>.5){
      t->value=t->values[i+1];
      t->delta=i+1;
    }else{
      t->value=t->values[i];
      t->delta=i;
    }
  }else{
    if(i==t->label_n-1){
      t->value=t->values[t->label_n-1];
    }else{
      t->value = (i-t->delta+1)*t->values[i] + (t->delta-i)*t->values[i+1];
    }
  }

  if(t->button->window){
    GdkRectangle rect;
    rect.x = t->button->allocation.x;
    rect.y = t->button->allocation.y;
    rect.width = t->button->allocation.width;
    rect.height = t->button->allocation.height;
    gdk_window_invalidate_rect(t->button->window,&rect,1);
  }

  if(t->callback)t->callback(t);
}

static gboolean motion_slider(GtkWidget *widget,
                              GdkEvent *event,
                              gpointer userdata){
  rowslider *t = (rowslider *)userdata;
  GdkEventMotion *ev = (GdkEventMotion *)event;
  GtkStateType state = gtk_widget_get_state(widget);

  if(t->capwidth==0)
    rowslider_update_size(t);

  if(state==GTK_STATE_ACTIVE)
    slider_set(t,ev->x);

  t->x = ev->x;
  return TRUE;
}

static gboolean press_slider(GtkWidget *widget,
                             gpointer userdata){
  rowslider *t = (rowslider *)userdata;
  slider_set(t,t->x);

  return TRUE;
}

static gboolean press_slider_logx(GtkWidget *widget,
                                  GdkEvent *event,
                                  gpointer userdata){
  GdkEventButton *ev = (GdkEventButton *)event;
  rowslider *t = (rowslider *)userdata;
  t->x = ev->x;
  return FALSE;
}

rowslider *rowslider_new(GtkBox *box,
                         char *caption,
                         void (*callback)(rowslider *)){
  rowslider *ret = calloc(1,sizeof(*ret));
  ret->parent = box;
  ret->button = gtk_button_new();
  gtk_widget_set_name(ret->button,"rowslider");

  char buf[256];
  snprintf(buf,256,"<span weight=\"bold\">%s</span>",caption);
  ret->caption = gtk_widget_create_pango_layout(ret->button,NULL);
  pango_layout_set_markup(ret->caption,buf,-1);

  ret->callback = callback;
  ret->labels = calloc(1,sizeof(*ret->labels));
  ret->values = calloc(1,sizeof(*ret->values));
  ret->stops = calloc(1,sizeof(*ret->stops));

  gtk_box_pack_start(GTK_BOX(box),ret->button,1,1,0);

  gtk_widget_add_events (ret->button,
                         GDK_POINTER_MOTION_MASK);

  g_signal_connect(G_OBJECT(ret->button), "expose-event",
                   G_CALLBACK(expose_slider), ret);
  g_signal_connect(G_OBJECT(ret->button), "motion_notify_event",
                   G_CALLBACK(motion_slider), ret);
  g_signal_connect(G_OBJECT(ret->button), "button-press-event",
                   G_CALLBACK(press_slider_logx), ret);
  g_signal_connect(G_OBJECT(ret->button), "pressed",
                   G_CALLBACK(press_slider), ret);

  gtk_widget_show_all(ret->button);
  return ret;
}

void rowslider_add_stop(rowslider *t,
                        char *ltext,
                        float lvalue,
                        int snap){
  int n = t->label_n;

  t->labels = realloc(t->labels,sizeof(*t->labels)*(n+1));
  t->values = realloc(t->values,sizeof(*t->values)*(n+1));
  t->stops = realloc(t->stops,sizeof(*t->stops)*(n+1));
  t->labels[n]=gtk_widget_create_pango_layout(t->button,ltext);

  t->values[n]=lvalue;
  t->stops[n]=snap;
  t->label_n++;

  rowslider_update_size(t);
}

void rowslider_set(rowslider *t,float lvalue){
  int i;
  int flip=1.0;
  if(t->values[0] > t->values[t->label_n-1])
    flip=-1.0;

  lvalue*=flip;
  if(lvalue<=t->values[0]*flip){
    t->value = t->values[0];
    t->delta = 0;
  } else if(lvalue>=t->values[t->label_n-1]*flip){
    t->value = t->values[t->label_n-1];
    t->delta = t->label_n-1;
  }else{
    for(i=0;i<t->label_n-1;i++){
      if(lvalue>=t->values[i]*flip && lvalue<=t->values[i+1]*flip){
        float delta = (lvalue-t->values[i]*flip) /
          (t->values[i+1]-t->values[i])*flip;
        if(t->stops[i]){
          if(delta<.5){
            t->value = t->values[i];
            t->delta = i;
          }else{
            t->value = t->values[i+1];
            t->delta = i+1;
          }
        }else{
          t->value = lvalue*flip;
          t->delta = i+delta;
        }
      }
    }
  }

  if(t->button->window){
    GdkRectangle rect;
    rect.x = t->button->allocation.x;
    rect.y = t->button->allocation.y;
    rect.width = t->button->allocation.width;
    rect.height = t->button->allocation.height;
    gdk_window_invalidate_rect(t->button->window,&rect,1);
  }

  if(t->callback)t->callback(t);
}


/* housekeeping for the up and down buttons, along with the panel
   scrolling implementation */

static gboolean expose_upbutton(GtkWidget *widget,
                                GdkEventExpose *event,
                                gpointer userdata){
  cairo_t *cr = gdk_cairo_create(widget->window);
  int w=widget->allocation.width;
  int h=widget->allocation.height;
  GtkStateType state = gtk_widget_get_state(widget);

  if(state == GTK_STATE_INSENSITIVE) return TRUE;

  /* rounded arrow path */
  double
    radius    = 4.0,
    scale     = (w/2>h?h:w/2)-radius*2,
    x         = widget->allocation.x+w/2,        /* parameters like cairo_rectangle */
    y         = widget->allocation.y+radius+.5;
  double degrees = M_PI / 180.0;

  cairo_new_sub_path (cr);
  cairo_arc (cr, x, y, radius, 225*degrees, 315*degrees);
  cairo_arc (cr, x+scale, y+scale, radius, -45*degrees, 135*degrees);
  cairo_line_to (cr, x, y+sqrt(2)*radius);
  cairo_arc (cr, x-scale, y+scale, radius, 45*degrees, 225*degrees);
  cairo_close_path (cr);

  /* fill translucent light background */
  cairo_set_source_rgba (cr, .8, .8, .8, .2);
  cairo_fill_preserve (cr);

  /* stroke border */
  cairo_set_line_width(cr,1.0);
  if(state == GTK_STATE_ACTIVE)
    cairo_set_source_rgba (cr, .5, .6, .83, 1);
  else
    cairo_set_source_rgba (cr, .8, .8, .8, 1);
  cairo_stroke(cr);

  cairo_destroy(cr);

  return TRUE;
}

static gboolean expose_downbutton(GtkWidget *widget,
                                  GdkEventExpose *event,
                                  gpointer userdata){
  cairo_t *cr = gdk_cairo_create(widget->window);
  int w=widget->allocation.width;
  int h=widget->allocation.height;
  GtkStateType state = gtk_widget_get_state(widget);

  if(state == GTK_STATE_INSENSITIVE) return TRUE;

  /* rounded arrow path */
  double
    radius    = 4.0,
    scale     = (w/2>h?h:w/2)-radius*2,
    x         = widget->allocation.x+w/2,        /* parameters like cairo_rectangle */
    y         = widget->allocation.y+h-radius-1.5;
  double degrees = M_PI / 180.0;

  cairo_new_sub_path (cr);
  cairo_arc (cr, x, y, radius, -315*degrees, -225*degrees);
  cairo_arc (cr, x-scale, y-scale, radius, -225*degrees, -45*degrees);
  cairo_line_to (cr, x, y-sqrt(2)*radius);
  cairo_arc (cr, x+scale, y-scale, radius, -135*degrees, 45*degrees);
  cairo_close_path (cr);

  /* fill translucent light background */
  cairo_set_source_rgba (cr, .8, .8, .8, .2);
  cairo_fill_preserve (cr);

  /* stroke border */
  cairo_set_line_width(cr,1.0);
  if(state == GTK_STATE_ACTIVE)
    cairo_set_source_rgba (cr, .5, .6, .83, 1);
  else
    cairo_set_source_rgba (cr, .8, .8, .8, 1);
  cairo_stroke(cr);

  cairo_destroy(cr);

  return TRUE;
}

static void animate_panel(rowpanel *p){
  int h = p->panel_scrollfix->allocation.height;
  double new_now = filter_filter(p->current_panel,&p->display_filter);
  if(fabs(new_now - p->current_panel)*h<1.){
    p->display_animating=0;
    p->display_now=p->current_panel;
  }else{
    p->display_now=new_now;
    g_timeout_add(25,(GSourceFunc)animate_panel,p);
  }
  gtk_fixed_move(GTK_FIXED(p->panel_scrollfix),p->panel_lefttable,
                 0,rint(-h*p->display_now));
}

static void set_current_panel(rowpanel *p, int n){
  if(p->display_animating){
    p->current_panel=n;
  }else{
    p->display_animating = 1;
    filter_set(&p->display_filter,p->current_panel);
    p->current_panel=n;
    animate_panel(p);
  }

  if(p->current_panel==0){
    gtk_widget_set_sensitive(p->upbutton,FALSE);
  }else{
    gtk_widget_set_sensitive(p->upbutton,TRUE);
  }
  if(p->current_panel+1==p->num_panels){
    gtk_widget_set_sensitive(p->downbutton,FALSE);
  }else{
    gtk_widget_set_sensitive(p->downbutton,TRUE);
  }
  if(p->callback)p->callback(p);
}

static void upbutton_clicked(GtkWidget *widget, gpointer data){
  rowpanel *p=(rowpanel *)data;
  if(p->current_panel>0)
    set_current_panel(p,p->current_panel-1);
}

static void downbutton_clicked(GtkWidget *widget, gpointer data){
  rowpanel *p=(rowpanel *)data;
  if(p->current_panel+1<p->num_panels)
    set_current_panel(p,p->current_panel+1);
}

gboolean supports_alpha = FALSE;
static void screen_changed(GtkWidget *widget,
                    GdkScreen *old_screen, gpointer userdata){

  /* To check if the display supports alpha channels, get the colormap */
  GdkScreen *screen = gtk_widget_get_screen(widget);
  GdkColormap *colormap = gdk_screen_get_rgba_colormap(screen);

  if (!colormap){
    printf("Your screen does not support alpha channels!\n");
    colormap = gdk_screen_get_rgb_colormap(screen);
    supports_alpha = FALSE;
  }else{
    supports_alpha = TRUE;
  }

  gtk_widget_set_colormap(widget, colormap);
}

static gboolean expose_toplevel(GtkWidget *widget,
                                GdkEventExpose *event,
                                gpointer userdata){
  cairo_t *cr = gdk_cairo_create(widget->window);
  int w=widget->allocation.width;
  int h=widget->allocation.height;

  /* clear background to transparent */
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba (cr, 0, 0, 0, 0); /* transparent */
  cairo_paint (cr);

  /* rounded rectangle path */
  double
    x         = 1,        /* parameters like cairo_rectangle */
    y         = 1,
    width     = w-2,
    height    = h-2,
    radius    = 8.0;
  double degrees = M_PI / 180.0;

  cairo_new_sub_path (cr);
  cairo_arc (cr, x+width-radius, y+radius, radius, -90*degrees, 0);
  cairo_arc (cr, x+width-radius, y+height-radius, radius, 0, 90*degrees);
  cairo_arc (cr, x+radius, y+height-radius, radius, 90*degrees, 180*degrees);
  cairo_arc (cr, x+radius, y+radius, radius, 180*degrees, 270*degrees);
  cairo_close_path (cr);

  /* fill translucent dark background */
  cairo_set_source_rgba (cr, 0, 0, 0, .5);
  cairo_fill_preserve (cr);

  /* stroke border */
  cairo_set_line_width(cr,2.0);
  cairo_set_source_rgba (cr, .8, .8, .8, 1);
  cairo_stroke(cr);

  cairo_destroy(cr);

  return FALSE;
}

/* hide the X cursor upon entry -- this is a touch tablet app */
static gboolean hide_mouse(GtkWidget *widget,
                           GdkEvent *event,
                           gpointer userdata){
  GdkCursor *cursor=gdk_cursor_new(GDK_BLANK_CURSOR);
  gdk_window_set_cursor(widget->window, cursor);
  gdk_cursor_destroy(cursor);
  return TRUE;
}

rowpanel *rowpanel_new(int w, int h, void (*callback)(rowpanel *)){
  rowpanel *p = calloc(1,sizeof(*p));

  GtkWidget *topbox = gtk_hbox_new(0,0);
  GtkWidget *rightbox = gtk_vbox_new(1,0);
  GtkWidget *leftbox = gtk_hbox_new(1,0);

  p->toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  p->callback = callback;
  gtk_widget_set_app_paintable(p->toplevel, TRUE);
  g_signal_connect(G_OBJECT(p->toplevel), "screen-changed",
                   G_CALLBACK(screen_changed), p);
  g_signal_connect(G_OBJECT(p->toplevel), "expose-event",
                   G_CALLBACK(expose_toplevel), p);
  g_signal_connect(G_OBJECT(p->toplevel), "enter-notify-event",
                   G_CALLBACK(hide_mouse), p);

  /* toplevel is a fixed size, meant to be nailed to the screen in one
     spot */
  p->w=w;
  p->h=h;
  gtk_widget_set_size_request(GTK_WIDGET(p->toplevel),w,h);
  gtk_window_set_resizable(GTK_WINDOW(p->toplevel),FALSE);
  gtk_window_set_decorated(GTK_WINDOW(p->toplevel),FALSE);
  gtk_window_set_gravity(GTK_WINDOW(p->toplevel),GDK_GRAVITY_SOUTH_WEST);
  gtk_window_move (GTK_WINDOW(p->toplevel),0,gdk_screen_height()-1);

  p->panel_scrollfix = gtk_fixed_new();
  p->panel_lefttable = gtk_table_new(1,1,1);
  p->upbutton = gtk_button_new();
  p->downbutton = gtk_button_new();

  gtk_widget_set_size_request(GTK_WIDGET(p->upbutton),40,25);
  gtk_widget_set_size_request(GTK_WIDGET(p->downbutton),40,25);

  gtk_container_add(GTK_CONTAINER(p->toplevel),topbox);
  gtk_container_set_border_width(GTK_CONTAINER(topbox),2);
  gtk_box_pack_start(GTK_BOX(topbox),leftbox,1,1,0);
  gtk_box_pack_end(GTK_BOX(topbox),rightbox,0,0,0);
  gtk_box_pack_start(GTK_BOX(leftbox),p->panel_scrollfix,1,1,0);
  gtk_box_pack_start(GTK_BOX(rightbox),p->upbutton,0,1,3);
  gtk_box_pack_end(GTK_BOX(rightbox),p->downbutton,0,1,3);

  g_signal_connect(G_OBJECT(p->upbutton), "expose-event",
                   G_CALLBACK(expose_upbutton), p);
  g_signal_connect(G_OBJECT(p->downbutton), "expose-event",
                   G_CALLBACK(expose_downbutton), p);
  g_signal_connect(G_OBJECT(p->upbutton), "pressed",
                   G_CALLBACK(upbutton_clicked), p);
  g_signal_connect(G_OBJECT(p->downbutton), "pressed",
                   G_CALLBACK(downbutton_clicked), p);

  filter_make_critical(.07,1,&p->display_filter);
  gtk_widget_set_size_request(p->panel_lefttable,w-44,-1);
  gtk_fixed_put(GTK_FIXED(p->panel_scrollfix),p->panel_lefttable,0,0);

  screen_changed(p->toplevel, NULL, NULL);
  set_current_panel(p,0);
  gtk_widget_show_all(p->toplevel);

  return p;
}

GtkBox *rowpanel_new_row(rowpanel *p, int fill_p){
  int boxborder=8;
  int n = ++p->num_panels;
  GtkWidget *heightbox = gtk_hbox_new(0,0);
  GtkWidget *heightforce = gtk_vbox_new(1,0);
  GtkWidget *panelframe = gtk_frame_new(NULL);
  GtkWidget *centerbox = gtk_hbox_new(1,0);
  GtkWidget *buttonbox = gtk_hbox_new(0,6);

  gtk_table_resize(GTK_TABLE(p->panel_lefttable),n,1);

  gtk_table_attach(GTK_TABLE(p->panel_lefttable),heightbox,0,1,n-1,n,
                   GTK_EXPAND|GTK_FILL,0,0,0);
  gtk_box_pack_start(GTK_BOX(heightbox),heightforce,0,0,0);
  gtk_widget_set_size_request(heightforce,1,p->h-4);

  gtk_widget_set_name(panelframe,"topframe");
  gtk_frame_set_label_align(GTK_FRAME(panelframe),.5,.5);
  gtk_container_set_border_width(GTK_CONTAINER(panelframe),boxborder);
  gtk_frame_set_shadow_type(GTK_FRAME(panelframe),GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(heightbox),panelframe,1,1,0);
  gtk_container_add(GTK_CONTAINER(panelframe),centerbox);
  gtk_container_set_border_width(GTK_CONTAINER(buttonbox),boxborder);

  if(fill_p){
    /* expanding box will fill */
    gtk_box_pack_start(GTK_BOX(centerbox),buttonbox,1,1,0);
  }else{
    /* nonexpanding box will center */
    gtk_box_pack_start(GTK_BOX(centerbox),buttonbox,0,0,0);
  }
  gtk_widget_show_all(heightbox);

  while (gtk_events_pending())
    gtk_main_iteration();

  return GTK_BOX(buttonbox);
}
