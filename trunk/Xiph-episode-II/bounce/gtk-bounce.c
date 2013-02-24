#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#define PANEL_WIDTH 1024
#define PANEL_HEIGHT 150

int current_panel=0;
int num_panels=4;

gboolean supports_alpha = FALSE;
static void screen_changed(GtkWidget *widget, GdkScreen *old_screen, gpointer userdata)
{
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

void expose_a_widget(gpointer a, gpointer b){
  GtkWidget *widget = (GtkWidget *)a;
  GdkEventExpose *event = (GdkEventExpose *)b;
  GTK_WIDGET_CLASS(GTK_WIDGET_GET_CLASS(widget))->
    expose_event (widget, event);
}

typedef struct {
  GtkBox *parent;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *vbox;
  GtkWidget *label[5];
  GtkWidget *alignment;
  int pressed;
  int lit;
} rowwidget;

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
  double radius = 12;
  double height = h-2;
  double rxd = radius-cos(15*degrees)*radius;
  double ryd = sin(15*degrees)*radius;
  double R = (height/2-radius+ryd)/sin(15*degrees);
  double RxD = R-cos(15*degrees)*R;

  double x = widget->allocation.x+RxD-rxd+1;
  double width = w - RxD*2 + rxd*2 - 2;
  double y = widget->allocation.y+1;

  double Rx1 = widget->allocation.x+R+1;
  double Rx2 = widget->allocation.x+w-R-1;

  clip_to_ancestors(widget,cr);

  cairo_new_sub_path (cr);
  cairo_arc (cr, x+width-radius, y+radius, radius, -90*degrees, -15*degrees);
  cairo_arc (cr, Rx2, y+height/2, R, -15*degrees, 15*degrees);
  cairo_arc (cr, x+width-radius, y+height-radius, radius, 15*degrees, 90*degrees);

  cairo_arc (cr, x+radius, y+height-radius, radius, 90*degrees, 165*degrees);
  cairo_arc (cr, Rx1, y+height/2, R, 165*degrees, 195*degrees);
  cairo_arc (cr, x+radius, y+radius, radius, 195*degrees, 270*degrees);
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
      cairo_set_source_rgba (cr, 0, .5, 1, .4);
    else
      cairo_set_source_rgba (cr, 0, 0, 0, .5);
  }
  cairo_fill_preserve (cr);

  /* gradient */
  if(state != GTK_STATE_INSENSITIVE){
    cairo_pattern_t *pat = cairo_pattern_create_linear(x, y, x, y+h);
    if(!t->pressed){
      cairo_pattern_add_color_stop_rgba(pat, 1.0, 0, 0, 0, .2);
      cairo_pattern_add_color_stop_rgba(pat, 0.8, 0, 0, 0, 0);
      cairo_pattern_add_color_stop_rgba(pat, 0.3, 1, 1, 1, 0);
      cairo_pattern_add_color_stop_rgba(pat, 0, 1, 1, 1, .15);
    }
    cairo_set_source(cr, pat);
    cairo_fill_preserve (cr);
    cairo_pattern_destroy(pat);
  }

  /* stroke border */
  if(state != GTK_STATE_INSENSITIVE){
    switch(state){
    case GTK_STATE_ACTIVE:
    case GTK_STATE_NORMAL:
      cairo_set_source_rgba (cr, .8, .8, .8, 1);
      break;
    case GTK_STATE_PRELIGHT:
    case GTK_STATE_SELECTED:
      cairo_set_source_rgba (cr, 1, 1, 1, 1);
      break;
    }

    cairo_set_line_width(cr,2.0);
    cairo_stroke(cr);
  }
  cairo_destroy(cr);

  /* have to do this by hand as returning TRUE below short-circuits
     the whole expose chain */
  //GList *children=gtk_container_get_children(GTK_CONTAINER(widget));
  //g_list_foreach(children,expose_a_widget,event);

  return TRUE;
}

/* hide the X cursor upon entry -- this is a touch tablet app */
static gboolean hide_mouse(GtkWidget *widget,
                           GdkEvent *event,
                           gpointer userdata){
  GdkCursor *cursor=gdk_cursor_new(GDK_BLANK_CURSOR);
  gdk_window_set_cursor(widget->window, cursor);
  gdk_cursor_destroy(cursor);
}

static gboolean expose_rowwidget(GtkWidget *widget,
                                 GdkEventExpose *event,
                                 gpointer userdata){
  rowwidget *t = (rowwidget *)userdata;
  int i;

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

static rowwidget *rowtoggle_new(GtkBox *box, GCallback callback){
  rowwidget *ret = calloc(1,sizeof(*ret));
  ret->parent = box;
  ret->table = gtk_table_new(1,1,1);
  ret->button = gtk_toggle_button_new();
  ret->alignment = gtk_alignment_new(.5,.5,0,0);
  ret->vbox = gtk_vbox_new(0,2);

  memset(ret->label,0,sizeof(ret->label));

  gtk_box_pack_start(box,ret->table,0,0,10);
  gtk_table_attach_defaults(GTK_TABLE(ret->table),ret->alignment,0,1,0,1);
  gtk_container_add(GTK_CONTAINER(ret->alignment),ret->vbox);
  gtk_table_attach_defaults(GTK_TABLE(ret->table),ret->button,0,1,0,1);

  g_signal_connect(G_OBJECT(ret->button), "expose-event",
                   G_CALLBACK(expose_rectarea), ret);
  g_signal_connect(G_OBJECT(ret->button), "pressed",
                   G_CALLBACK(press_rowtoggle), ret);
  g_signal_connect(G_OBJECT(ret->button), "released",
                   G_CALLBACK(release_rowtoggle), ret);
  g_signal_connect(G_OBJECT(ret->table), "expose-event",
                 G_CALLBACK(expose_rowwidget), ret);
  gtk_widget_set_size_request(ret->button,PANEL_HEIGHT,-1);

  return ret;
}

static rowwidget *rowlabel_new(GtkBox *box){
  rowwidget *ret = calloc(1,sizeof(*ret));
  ret->parent = box;
  ret->table = gtk_table_new(1,1,1);
  ret->button = gtk_toggle_button_new();
  ret->alignment = gtk_alignment_new(.5,.5,0,0);
  ret->vbox = gtk_vbox_new(0,2);

  memset(ret->label,0,sizeof(ret->label));

  gtk_box_pack_start(box,ret->table,0,0,10);
  gtk_table_attach_defaults(GTK_TABLE(ret->table),ret->alignment,0,1,0,1);
  gtk_container_add(GTK_CONTAINER(ret->alignment),ret->vbox);
  gtk_table_attach_defaults(GTK_TABLE(ret->table),ret->button,0,1,0,1);
  gtk_widget_set_sensitive(ret->button,FALSE);

  g_signal_connect(G_OBJECT(ret->button), "expose-event",
                   G_CALLBACK(expose_rectarea), ret);
  g_signal_connect(G_OBJECT(ret->table), "expose-event",
                 G_CALLBACK(expose_rowwidget), ret);
  gtk_widget_set_size_request(ret->button,PANEL_HEIGHT,-1);

  return ret;
}

static rowwidget *rowlabel_light(rowwidget *t,int state){
  t->lit=state;
  expose_rectarea(t->button,NULL,t);
}

static void rowwidget_add_label(rowwidget *t, char *label, int row){
  if(!t)return;
  if(row<0 || row>4)return;
  if(t->label[row] && label==NULL){
    gtk_widget_destroy(t->label[row]);
    t->label[row]=NULL;
  }else{
    if(!t->label[row]){
      t->label[row]=gtk_label_new(NULL);
      gtk_box_pack_start(GTK_BOX(t->vbox),t->label[row],0,0,0);
    }
    gtk_label_set_markup(GTK_LABEL(t->label[row]),label);
  }
}

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
    y         = widget->allocation.y+radius;
  double degrees = M_PI / 180.0;

  cairo_new_sub_path (cr);
  cairo_arc (cr, x, y, radius, 225*degrees, 315*degrees);
  cairo_arc (cr, x+scale, y+scale, radius, -45*degrees, 135*degrees);
  cairo_line_to (cr, x, y+sqrt(2)*radius);
  cairo_arc (cr, x-scale, y+scale, radius, 45*degrees, 225*degrees);
  cairo_close_path (cr);

  /* fill translucent light background */
  if(state == GTK_STATE_PRELIGHT)
    cairo_set_source_rgba (cr, 1, 1, 1, .5);
  else
    cairo_set_source_rgba (cr, .8, .8, .8, .5);
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
    y         = widget->allocation.y+h-radius-1;
  double degrees = M_PI / 180.0;

  cairo_new_sub_path (cr);
  cairo_arc (cr, x, y, radius, -315*degrees, -225*degrees);
  cairo_arc (cr, x-scale, y-scale, radius, -225*degrees, -45*degrees);
  cairo_line_to (cr, x, y-sqrt(2)*radius);
  cairo_arc (cr, x+scale, y-scale, radius, -135*degrees, 45*degrees);
  cairo_close_path (cr);

  /* fill translucent light background */
  if(state == GTK_STATE_PRELIGHT)
    cairo_set_source_rgba (cr, 1, 1, 1, .5);
  else
    cairo_set_source_rgba (cr, .8, .8, .8, .5);
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

typedef struct {
  double a;
  double b1;
  double b2;
  double x[2];
  double y[2];
} pole2;

static void filter_reset(pole2 *p, double val){
  p->x[0]=p->x[1]=val;
  p->y[0]=p->y[1]=val;
}

static void filter_make_critical(double w, pole2 *f){
  double w0 = tan(M_PI*w*pow(pow(2,.5)-1,-.5));
  f->a  = w0*w0/(1+(2*w0)+w0*w0);
  f->b1 = 2*f->a*(1/(w0*w0)-1);
  f->b2 = 1-(4*f->a+f->b1);
  filter_reset(f,0);
}

static double filter_filter(double x, pole2 *p){
  double y =
    p->a*x + 2*p->a*p->x[0] + p->a*p->x[1] +
    p->b1*p->y[0] + p->b2*p->y[1];
  p->y[1] = p->y[0]; p->y[0] = y;
  p->x[1] = p->x[0]; p->x[0] = x;
  return y;
}

static pole2 display_filter;
static double display_now=0;
static int display_animating=0;
GtkWidget *panel_scrollfix=NULL;
GtkWidget *panel_lefttable=NULL;
GtkWidget *upbutton=NULL;
GtkWidget *downbutton=NULL;

static void animate_panel(){
  int h = panel_scrollfix->allocation.height;
  double new_now = filter_filter(current_panel,&display_filter);
  if(fabs(new_now - current_panel)*h<1.){
    display_animating=0;
    display_now=current_panel;
  }else{
    display_now=new_now;
    g_timeout_add(25,(GSourceFunc)animate_panel,NULL);
  }
  gtk_fixed_move(GTK_FIXED(panel_scrollfix),panel_lefttable,
                 0,rint(-h*display_now));
}

static void set_current_panel(int n){
  if(display_animating){
    current_panel=n;
  }else{
    display_animating = 1;
    filter_reset(&display_filter,current_panel);
    current_panel=n;
    animate_panel();
  }

  if(current_panel==0){
    gtk_widget_set_sensitive(upbutton,FALSE);
  }else{
    gtk_widget_set_sensitive(upbutton,TRUE);
  }
  if(current_panel+1==num_panels){
    gtk_widget_set_sensitive(downbutton,FALSE);
  }else{
    gtk_widget_set_sensitive(downbutton,TRUE);
  }
}

static void upbutton_clicked(GtkWidget *widget, gpointer data){
  if(current_panel>0)
    set_current_panel(current_panel-1);
}

static void downbutton_clicked(GtkWidget *widget, gpointer data){
  if(current_panel+1<num_panels)
    set_current_panel(current_panel+1);
}

static void make_panel(void){
  int w=PANEL_WIDTH;
  int h=PANEL_HEIGHT;

  GtkWidget *toplevel=NULL;
  GtkWidget *topbox=NULL;
  GtkWidget *rightbox=NULL;
  GtkWidget *leftbox=NULL;
  GtkWidget *panels[num_panels];

  int i;

  toplevel = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_app_paintable(toplevel, TRUE);
  g_signal_connect(G_OBJECT(toplevel), "screen-changed", G_CALLBACK(screen_changed), NULL);
  g_signal_connect(G_OBJECT(toplevel), "expose-event", G_CALLBACK(expose_toplevel), NULL);
  g_signal_connect(G_OBJECT(toplevel), "enter-notify-event", G_CALLBACK(hide_mouse), NULL);

  /* toplevel is a fixed size, meant to be nailed to the screen in one
     spot */
  gtk_widget_set_size_request(GTK_WIDGET(toplevel),w,h);
  gtk_window_set_resizable(GTK_WINDOW(toplevel),FALSE);
  gtk_window_set_decorated(GTK_WINDOW(toplevel),FALSE);
  gtk_window_set_gravity(GTK_WINDOW(toplevel),GDK_GRAVITY_SOUTH_WEST);
  gtk_window_move (GTK_WINDOW(toplevel),0,gdk_screen_height()-1);

  /* multiple sliding panes within */
  topbox = gtk_hbox_new(0,0);
  rightbox = gtk_vbox_new(1,0);
  leftbox = gtk_hbox_new(1,0);
  panel_scrollfix = gtk_fixed_new();
  panel_lefttable = gtk_table_new(num_panels,1,1);
  upbutton = gtk_button_new();
  downbutton = gtk_button_new();

  gtk_widget_set_size_request(GTK_WIDGET(upbutton),60,25);
  gtk_widget_set_size_request(GTK_WIDGET(downbutton),60,25);

  gtk_container_add(GTK_CONTAINER(toplevel),topbox);
  gtk_container_set_border_width(GTK_CONTAINER(topbox),2);
  gtk_box_pack_start(GTK_BOX(topbox),leftbox,1,1,0);
  gtk_box_pack_end(GTK_BOX(topbox),rightbox,0,0,0);
  gtk_box_pack_start(GTK_BOX(leftbox),panel_scrollfix,1,1,0);
  gtk_box_pack_start(GTK_BOX(rightbox),upbutton,0,1,3);
  gtk_box_pack_end(GTK_BOX(rightbox),downbutton,0,1,3);

  g_signal_connect(G_OBJECT(upbutton), "expose-event", G_CALLBACK(expose_upbutton), NULL);
  g_signal_connect(G_OBJECT(downbutton), "expose-event", G_CALLBACK(expose_downbutton), NULL);
  g_signal_connect(G_OBJECT(upbutton), "pressed", G_CALLBACK(upbutton_clicked), NULL);
  g_signal_connect(G_OBJECT(downbutton), "pressed", G_CALLBACK(downbutton_clicked), NULL);

  filter_make_critical(.05,&display_filter);
  gtk_widget_set_size_request(panel_lefttable,w-4,-1);
  gtk_fixed_put(GTK_FIXED(panel_scrollfix),panel_lefttable,0,0);


  /* build the sliding frame table */
  int boxborder=10;
  for(i=0;i<num_panels;i++){
    GtkWidget *heightbox = gtk_hbox_new(0,0);
    GtkWidget *heightforce = gtk_vbox_new(1,0);
    GtkWidget *panelframe = gtk_frame_new("foo label");
    GtkWidget *buttonbox = gtk_hbox_new(0,0);

    gtk_table_attach(GTK_TABLE(panel_lefttable),heightbox,0,1,i,i+1,
                     GTK_EXPAND|GTK_FILL,0,0,0);
    gtk_box_pack_start(GTK_BOX(heightbox),heightforce,0,0,0);
    gtk_widget_set_size_request(heightforce,1,h-4);

    gtk_widget_set_name(panelframe,"topframe");
    gtk_frame_set_label_align(GTK_FRAME(panelframe),.5,.5);
    gtk_container_set_border_width(GTK_CONTAINER(panelframe),boxborder);
    gtk_frame_set_shadow_type(GTK_FRAME(panelframe),GTK_SHADOW_NONE);
    gtk_box_pack_start(GTK_BOX(heightbox),panelframe,1,1,0);
    gtk_container_add(GTK_CONTAINER(panelframe),buttonbox);
    gtk_container_set_border_width(GTK_CONTAINER(buttonbox),boxborder);

    switch(i){
    default:
      {
        rowwidget *t = rowtoggle_new(GTK_BOX(buttonbox), NULL);
        rowwidget_add_label(t,"foo",0);
      }
      break;
    case 1:
      {
        rowwidget *t = rowlabel_new(GTK_BOX(buttonbox));
        rowwidget_add_label(t,"foo",0);
      }
      break;
    case 2:
      {
        rowwidget *t = rowlabel_new(GTK_BOX(buttonbox));
        rowwidget_add_label(t,"foo",0);
        rowlabel_light(t,1);
      }
      break;
    }
  }


  screen_changed(toplevel, NULL, NULL);
  set_current_panel(0);

  gtk_widget_show_all(toplevel);
}

int main(int argc, char **argv){
  gtk_init (&argc, &argv);

  gtk_rc_parse_string
    ("style \"panel\" {"
     "  fg[NORMAL]=\"#ffffff\""
     "}"
     "style \"topframe\" {"
     "  font_name = \"sans 10 bold\""
     "  fg[NORMAL]=\"#cccccc\""
     "}"
     "class \"*\" style \"panel\""
     "widget \"*.topframe.GtkLabel\" style \"topframe\""
     );

  make_panel();
  gtk_main();

  return 0;
}
