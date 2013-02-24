#include "gtk-bounce.h"
#include "gtk-bounce-widget.h"
#include <pthread.h>

#define PANEL_WIDTH 1024
#define PANEL_HEIGHT 90

/* parameters that set the uncondiitonal behavior of the io thread */
sig_atomic_t exiting=0;

sig_atomic_t request_bits=16;
//sig_atomic_t request_ch=2;
sig_atomic_t request_rate=44100;

sig_atomic_t request_ch1_quant=0;
sig_atomic_t request_ch2_quant=0;
sig_atomic_t prime_1kHz_notch=0;
sig_atomic_t request_1kHz_notch=0;
sig_atomic_t request_1kHz_sine=0;
sig_atomic_t request_1kHz_sine2=0;
sig_atomic_t request_5kHz_sine=0;
sig_atomic_t request_1kHz_amplitude=0;
sig_atomic_t request_1kHz_modulate=0;
sig_atomic_t request_dither=0;
sig_atomic_t request_dither_shaped=0;
sig_atomic_t request_dither_amplitude=0;

sig_atomic_t request_lp_filter=0;
sig_atomic_t request_1kHz_square=0;
sig_atomic_t request_1kHz_square_offset=0;
sig_atomic_t request_1kHz_square_spread=0;

sig_atomic_t request_output_silence=0;
sig_atomic_t request_output_noise=0;
sig_atomic_t request_output_sweep=0;
sig_atomic_t request_output_logsweep=0;
sig_atomic_t request_output_tone=0;
sig_atomic_t request_output_duallisten=0;

/* these are panel settings remembered between panels but not
   necessarily active on a given panel. They're used to update the io
   settings when switching between panels */

int current_panel=0;

int panel_bits;
int panel_ch1_quant;
int panel_ch2_quant;
int panel_1kHz_notch;
int panel_1kHz_sine;
int panel_5kHz_sine;
int panel_1kHz_amplitude;
int panel_dither;
int panel_dither_shaped;
int panel_dither_amplitude;

int panel_lp_filter;
int panel_1kHz_square;
int panel_1kHz_square_offset;
int panel_1kHz_square_spread;


pthread_t io_thread_id;
int eventpipe[2];

/* panel 0 */
rowwidget *panel0_readout_hw;
rowwidget *panel0_button_16bit;
rowwidget *panel0_button_24bit;
rowwidget *panel0_button_44;
rowwidget *panel0_button_48;
rowwidget *panel0_button_96;
rowwidget *panel0_button_192;

/* panel 1 */
rowwidget *panel1_button_c1_8;
rowwidget *panel1_button_c1_16;
rowwidget *panel1_button_c2_8;
rowwidget *panel1_button_c2_16;

/* panel 2 */
rowslider *panel2_slider_bits;
rowwidget *panel2_button_notch;

/* panel 3 */
rowwidget *panel3_button_1k;
rowwidget *panel3_button_8bit;
rowwidget *panel3_button_16bit;
rowwidget *panel3_button_dither;

/* panel 4 */
rowslider *panel4_slider_amp;
rowwidget *panel4_button_dither;

/* panel 5 */
rowwidget *panel5_button_dither;
rowwidget *panel5_button_flat;
rowwidget *panel5_button_shaped;
rowwidget *panel5_button_notch;

/* panel 6 */
rowslider *panel6_slider_amp;
rowwidget *panel6_button_notch;
rowwidget *panel6_button_modulate;
rowwidget *panel6_button_flat;
rowwidget *panel6_button_shaped;

/* panel 7 */

/* panel 8 */
rowwidget *panel8_button_square;
rowslider *panel8_slider_offset;

/* panel 9 */
rowslider *panel9_slider_spread;

/* panel 10 */
rowwidget *panel10_readout_hw;

rowwidget *panel10_button_silence;
rowwidget *panel10_button_noise;
rowwidget *panel10_button_sweep;
rowwidget *panel10_button_logsweep;
rowwidget *panel10_button_tone1;
rowwidget *panel10_button_tone5;
rowwidget *panel10_button_duallisten;

static int ready=0;
void panelchange(rowpanel *p){
  if(!ready)return;
  current_panel = p->current_panel;

  if(current_panel == 10){
    /* we use 24 bit for the response testing */
    request_bits=24;
  }else{
    request_bits=panel_bits;
  }

  switch(current_panel){
  case 1:
    panel_ch1_quant=16;
    panel_ch2_quant=16;
    /* fall through */
  case 2:
  case 3:
    if(current_panel!=2){
      if(panel_ch1_quant<16)
        panel_ch1_quant=8;
      if(panel_ch2_quant<16)
        panel_ch2_quant=8;
    }

    request_ch1_quant=panel_ch1_quant;
    request_ch2_quant=panel_ch2_quant;

    rowtoggle_set_active(panel1_button_c1_8,panel_ch1_quant==8);
    rowtoggle_set_active(panel1_button_c2_8,panel_ch2_quant==8);
    rowtoggle_set_active(panel1_button_c1_16,panel_ch1_quant==16);
    rowtoggle_set_active(panel1_button_c2_16,panel_ch2_quant==16);

    rowslider_set(panel2_slider_bits,panel_ch2_quant);

    rowtoggle_set_active(panel3_button_8bit,panel_ch2_quant==8);
    rowtoggle_set_active(panel3_button_16bit,panel_ch2_quant==16);

    break;
  case 4:
  case 5:
  case 6:
    request_ch1_quant=16;
    request_ch2_quant=16;
    break;
  default:
    request_ch1_quant=0;
    request_ch2_quant=0;
    break;
  }

  if(current_panel==2 || current_panel==5 || current_panel==6){
    request_1kHz_notch=panel_1kHz_notch=0;
    prime_1kHz_notch=1;
    rowtoggle_set_active(panel2_button_notch,panel_1kHz_notch);
    rowtoggle_set_active(panel5_button_notch,panel_1kHz_notch);
    rowtoggle_set_active(panel6_button_notch,panel_1kHz_notch);
  }else{
    request_1kHz_notch=0;
    prime_1kHz_notch=0;
  }

  switch(current_panel){
  case 3:
    request_1kHz_sine = request_1kHz_sine2 = panel_1kHz_sine = 0;
    rowtoggle_set_active(panel3_button_1k,panel_1kHz_sine);
    break;
  case 4:
  case 5:
  case 6:
    request_1kHz_sine = 1;
    request_1kHz_sine2 = 1;
    break;
  default:
    request_1kHz_sine = 0;
    request_1kHz_sine2 = 0;
  }

  if(current_panel==4){
    request_1kHz_amplitude=panel_1kHz_amplitude;
  }else{
    request_1kHz_amplitude=65536*16.;
  }

  switch(current_panel){
  case 1:
  case 2:
  case 5:
  case 6:
    //case 8:
    //case 9:
    panel_dither=request_dither=1;
    rowtoggle_set_active(panel3_button_dither,panel_dither);
    rowtoggle_set_active(panel4_button_dither,panel_dither);
    rowtoggle_set_active(panel5_button_dither,panel_dither);

    break;
  case 3:
  case 4:
    request_dither=panel_dither;
    rowtoggle_set_active(panel3_button_dither,panel_dither);
    rowtoggle_set_active(panel4_button_dither,panel_dither);
    rowtoggle_set_active(panel5_button_dither,panel_dither);

    break;
  default:
    request_dither=0;
    break;
  }

  //if(current_panel==5){
  request_dither_shaped=panel_dither_shaped=0;
  request_1kHz_modulate=0;
  // }else{
  //if(current_panel != 6)request_dither_shaped=0;
  //}
  rowtoggle_set_active(panel5_button_flat,!panel_dither_shaped);
  rowtoggle_set_active(panel5_button_shaped,panel_dither_shaped);
  rowtoggle_set_active(panel6_button_flat,!panel_dither_shaped);
  rowtoggle_set_active(panel6_button_shaped,panel_dither_shaped);
  rowtoggle_set_active(panel6_button_modulate,request_1kHz_modulate);

  if(current_panel==6){
    request_dither_amplitude=panel_dither_amplitude;
  }else{
    request_dither_amplitude=65536;
  }

  if(current_panel>=7 && current_panel<=9){
    request_lp_filter=1;
  }else{
    request_lp_filter=0;
  }

  switch(current_panel){
  case 8:
    request_1kHz_square=panel_1kHz_square;
    request_1kHz_square_offset=panel_1kHz_square_offset;
    panel_1kHz_square_spread=request_1kHz_square_spread=0;
    break;
  case 9:
    request_1kHz_square=1;
    panel_1kHz_square_offset=request_1kHz_square_offset=0;
    request_1kHz_square_spread=panel_1kHz_square_spread;
    break;
  default:
    panel_1kHz_square=request_1kHz_square=0;
    panel_1kHz_square_offset=request_1kHz_square_offset=0;
    panel_1kHz_square_spread=request_1kHz_square_spread=0;
    break;
  }
  rowtoggle_set_active(panel8_button_square,panel_1kHz_square);
  rowslider_set(panel8_slider_offset,panel_1kHz_square_offset);
  rowslider_set(panel9_slider_spread,panel_1kHz_square_spread);

  if(current_panel==10){
    request_output_duallisten=0;
    request_output_silence=1;
    rowtoggle_set_active(panel10_button_duallisten,0);
    rowtoggle_set_active(panel10_button_silence,1);
  }else{
    request_output_silence=0;
  }
}

/* action handlers for panel buttons */
static int clicked_enter=0;
void panel0_clicked_bits(rowwidget *t){
  if(!clicked_enter){
    clicked_enter=1;
    if(rowtoggle_get_active(t)){

      if(t==panel0_button_16bit)
        panel_bits=request_bits=16;
      else
        rowtoggle_set_active(panel0_button_16bit,0);

      if(t==panel0_button_24bit)
        panel_bits=request_bits=24;
      else
        rowtoggle_set_active(panel0_button_24bit,0);

    }else
      rowtoggle_set_active(t,1);
    clicked_enter=0;
  }
}

void panel0_clicked_rate(rowwidget *t){
  if(!clicked_enter){
    clicked_enter=1;
    if(rowtoggle_get_active(t)){

      if(t==panel0_button_44)
        request_rate=44100;
      else
        rowtoggle_set_active(panel0_button_44,0);

      if(t==panel0_button_48)
        request_rate=48000;
      else
        rowtoggle_set_active(panel0_button_48,0);

      if(t==panel0_button_96)
        request_rate=96000;
      else
        rowtoggle_set_active(panel0_button_96,0);

      if(t==panel0_button_192)
        request_rate=192000;
      else
        rowtoggle_set_active(panel0_button_192,0);
    }else
      rowtoggle_set_active(t,1);
    clicked_enter=0;
  }
}

void panel1_clicked_c1(rowwidget *t){
  if(!clicked_enter){
    clicked_enter=1;
    if(rowtoggle_get_active(t)){

      if(t==panel1_button_c1_8)
        panel_ch1_quant=request_ch1_quant=8;
      else
        rowtoggle_set_active(panel1_button_c1_8,0);

      if(t==panel1_button_c1_16)
        panel_ch1_quant=request_ch1_quant=16;
      else
        rowtoggle_set_active(panel1_button_c1_16,0);
    }else
      rowtoggle_set_active(t,1);
    clicked_enter=0;
  }
}

void panel1_clicked_c2(rowwidget *t){
  if(!clicked_enter){
    clicked_enter=1;
    if(rowtoggle_get_active(t)){

      if(t==panel1_button_c2_8)
        panel_ch2_quant=request_ch2_quant=8;
      else
        rowtoggle_set_active(panel1_button_c2_8,0);

      if(t==panel1_button_c2_16)
        panel_ch2_quant=request_ch2_quant=16;
      else
        rowtoggle_set_active(panel1_button_c2_16,0);
    }else
      rowtoggle_set_active(t,1);
    clicked_enter=0;
  }
}

void panel2_clicked_bits(rowslider *t){
  panel_ch1_quant = request_ch1_quant = t->value;
  panel_ch2_quant = request_ch2_quant = t->value;
}

void panel2_clicked_notch(rowwidget *t){
  panel_1kHz_notch = request_1kHz_notch = rowtoggle_get_active(t);
}

void panel3_clicked_1kHz(rowwidget *t){
  panel_1kHz_sine = request_1kHz_sine = request_1kHz_sine2 =
    rowtoggle_get_active(t);
}

void panel3_clicked_dither(rowwidget *t){
  panel_dither = request_dither = rowtoggle_get_active(t);
}

void panel3_clicked_bits(rowwidget *t){
  if(!clicked_enter){
    clicked_enter=1;
    if(rowtoggle_get_active(t)){

      if(t==panel3_button_8bit){
        panel_ch1_quant=request_ch1_quant=8;
        panel_ch2_quant=request_ch2_quant=8;
      }else
        rowtoggle_set_active(panel3_button_8bit,0);

      if(t==panel3_button_16bit){
        panel_ch1_quant=request_ch1_quant=16;
        panel_ch2_quant=request_ch2_quant=16;
      }else
        rowtoggle_set_active(panel3_button_16bit,0);
    }else
      rowtoggle_set_active(t,1);
    clicked_enter=0;
  }
}

void panel4_clicked_amp(rowslider *t){
  panel_1kHz_amplitude = request_1kHz_amplitude = rint(t->value*65536);
}

void panel4_clicked_dither(rowwidget *t){
  panel_dither = request_dither = rowtoggle_get_active(t);
}

void panel5_clicked_dither(rowwidget *t){
  panel_dither = request_dither = rowtoggle_get_active(t);
}

void panel5_clicked_notch(rowwidget *t){
  panel_1kHz_notch = request_1kHz_notch = rowtoggle_get_active(t);
}

void panel5_clicked_shape(rowwidget *t){
  if(!clicked_enter){
    clicked_enter=1;
    if(rowtoggle_get_active(t)){

      if(t==panel5_button_flat){
        panel_dither_shaped=request_dither_shaped=0;
      }else
        rowtoggle_set_active(panel5_button_flat,0);

      if(t==panel5_button_shaped){
        panel_dither_shaped=request_dither_shaped=1;
      }else
        rowtoggle_set_active(panel5_button_shaped,0);
    }else
      rowtoggle_set_active(t,1);
    clicked_enter=0;
  }
}

void panel6_clicked_notch(rowwidget *t){
  panel_1kHz_notch = request_1kHz_notch = rowtoggle_get_active(t);
}

void panel6_clicked_modulate(rowwidget *t){
  request_1kHz_modulate = rowtoggle_get_active(t);
}

void panel6_clicked_dithamp(rowslider *t){
  panel_dither_amplitude = request_dither_amplitude = rint(t->value*65536);
}

void panel6_clicked_shape(rowwidget *t){
  if(!clicked_enter){
    clicked_enter=1;
    if(rowtoggle_get_active(t)){

      if(t==panel6_button_flat){
        panel_dither_shaped=request_dither_shaped=0;
      }else
        rowtoggle_set_active(panel6_button_flat,0);

      if(t==panel6_button_shaped){
        panel_dither_shaped=request_dither_shaped=1;
      }else
        rowtoggle_set_active(panel6_button_shaped,0);
    }else
      rowtoggle_set_active(t,1);
    clicked_enter=0;
  }
}

void panel8_clicked_square(rowwidget *t){
  panel_1kHz_square = request_1kHz_square = rowtoggle_get_active(t);
}

void panel8_clicked_offset(rowslider *t){
  panel_1kHz_square_offset = request_1kHz_square_offset = rint(t->value*65536.);
}

void panel9_clicked_spread(rowslider *t){
  panel_1kHz_square_spread = request_1kHz_square_spread = rint(t->value*65536.);
}

void panel10_clicked_output(rowwidget *t){
  if(!clicked_enter){
    clicked_enter=1;
    if(rowtoggle_get_active(t)){

      if(t==panel10_button_silence){
        request_output_silence=1;
      }else{
        rowtoggle_set_active(panel10_button_silence,0);
        request_output_silence=0;
      }

      if(t==panel10_button_tone1){
        request_1kHz_sine=1;
      }else{
        rowtoggle_set_active(panel10_button_tone1,0);
        request_1kHz_sine=0;
      }

      if(t==panel10_button_tone5){
        request_5kHz_sine=1;
      }else{
        rowtoggle_set_active(panel10_button_tone5,0);
        request_5kHz_sine=0;
      }

      if(t!=panel10_button_tone1 &&
         t!=panel10_button_tone5)
        request_output_tone=0;
      else
        request_output_tone=1;

      if(t==panel10_button_noise){
        request_output_noise=1;
      }else{
        rowtoggle_set_active(panel10_button_noise,0);
        request_output_noise=0;
      }

      if(t==panel10_button_sweep){
        request_output_sweep=1;
      }else{
        rowtoggle_set_active(panel10_button_sweep,0);
        request_output_sweep=0;
      }

      if(t==panel10_button_logsweep){
        request_output_logsweep=1;
      }else{
        rowtoggle_set_active(panel10_button_logsweep,0);
        request_output_logsweep=0;
      }
    }else
      rowtoggle_set_active(t,1);
    clicked_enter=0;
  }
}

void panel10_clicked_duallisten(rowwidget *t){
  request_output_duallisten = rowtoggle_get_active(t);
}

/* panel communication */

static void blocking_read(int fd,char *buffer, int len){
  while(len){
    int bytes = read(fd,buffer,len);
    if(bytes>0){
      len-=bytes;
      buffer+=bytes;
    }
  }
}

static gboolean async_event_handle(GIOChannel *channel,
                                   GIOCondition condition,
                                   gpointer data){
  unsigned char buf[1];

  while(read(eventpipe[0],buf,1)>0){
    switch((int)buf[0]){
    case 0: /* exit */
      gtk_main_quit();
      break;
    case 1: /* hw1 opened */
      {
        char name[255];
        char len;
        blocking_read(eventpipe[0],&len,1);
        blocking_read(eventpipe[0],name,(int)len);

        rowreadout_light(panel0_readout_hw,1);
        rowwidget_add_label(panel0_readout_hw,name,1);
        rowreadout_light(panel10_readout_hw,1);
        rowwidget_add_label(panel10_readout_hw,name,1);
      }
      break;
    case 2: /* hw1 closed */
      rowreadout_light(panel0_readout_hw,0);
      rowwidget_add_label(panel0_readout_hw,NULL,1);
      rowreadout_light(panel10_readout_hw,0);
      rowwidget_add_label(panel10_readout_hw,NULL,1);
      break;
    case 5: /* update shaped dither setting */
      panel_dither_shaped = request_dither_shaped;
      rowtoggle_set_active(panel5_button_shaped,panel_dither_shaped);
      break;
    case 6: /* done with an output burst; back to silence */
      rowtoggle_set_active(panel10_button_silence,1);
    }
  }
  return TRUE;
}

/* toplevel panel itself */

static void make_panel(void){
  rowpanel *p=rowpanel_new(PANEL_WIDTH,PANEL_HEIGHT,panelchange);
  GtkBox *b;

  /* panel 0 */
  b = rowpanel_new_row(p,0);
  panel0_readout_hw = rowreadout_new
    (b, "<span foreground=\"#c0c0c0\">hw:1</span>");

  rowspacer_new(b,10);

  panel0_button_16bit = rowtoggle_new(b,"16 bit",panel0_clicked_bits);
  panel0_button_24bit = rowtoggle_new(b,"24 bit",panel0_clicked_bits);

  rowspacer_new(b,10);

  panel0_button_44 = rowtoggle_new(b,"44.1kHz",panel0_clicked_rate);
  panel0_button_48 = rowtoggle_new(b,"48kHz",panel0_clicked_rate);
  panel0_button_96 = rowtoggle_new(b,"96kHz",panel0_clicked_rate);
  panel0_button_192 = rowtoggle_new(b,"192kHz",panel0_clicked_rate);

  rowtoggle_set_active(panel0_button_16bit,1);
  rowtoggle_set_active(panel0_button_44,1);

  /* panel 1 */
  b = rowpanel_new_row(p,0);
  rowlabel_new(b, "channel 1:");
  panel1_button_c1_8 = rowtoggle_new(b,"8 bit",panel1_clicked_c1);
  panel1_button_c1_16 = rowtoggle_new(b,"16 bit",panel1_clicked_c1);

  rowspacer_new(b,10);
  rowspacer_new(b,10);

  rowlabel_new(b, "channel 2:");
  panel1_button_c2_8 = rowtoggle_new(b,"8 bit",panel1_clicked_c2);
  panel1_button_c2_16 = rowtoggle_new(b,"16 bit",panel1_clicked_c2);

  rowtoggle_set_active(panel1_button_c1_16,1);
  rowtoggle_set_active(panel1_button_c2_16,1);

  /* panel 2 */
  b = rowpanel_new_row(p,1);
  rowspacer_new(b,10);
  rowslider *t = panel2_slider_bits =
    rowslider_new(b,"bits",panel2_clicked_bits);
  rowslider_add_stop(t,"8",8,1);
  rowslider_add_stop(t,"9",9,1);
  rowslider_add_stop(t,"10",10,1);
  rowslider_add_stop(t,"11",11,1);
  rowslider_add_stop(t,"12",12,1);
  rowslider_add_stop(t,"13",13,1);
  rowslider_add_stop(t,"14",14,1);
  rowslider_add_stop(t,"15",15,1);
  rowslider_add_stop(t,"16",16,1);
  rowspacer_new(b,20);
  panel2_button_notch = rowtoggle_new(b,"notch and",panel2_clicked_notch);
  rowwidget_add_label(panel2_button_notch,"gain",1);
  rowspacer_new(b,10);

  /* panel 3 */
  b = rowpanel_new_row(p,0);
  panel3_button_1k = rowtoggle_new(b,"generate",panel3_clicked_1kHz);
  rowwidget_add_label(panel3_button_1k,"sine wave",1);
  rowspacer_new(b,40);
  panel3_button_8bit = rowtoggle_new(b,"8 bit",panel3_clicked_bits);
  panel3_button_16bit = rowtoggle_new(b,"16 bit",panel3_clicked_bits);
  rowspacer_new(b,40);
  panel3_button_dither = rowtoggle_new(b,"dither",panel3_clicked_dither);

  rowtoggle_set_active(panel3_button_1k,1);
  rowtoggle_set_active(panel3_button_8bit,1);
  rowtoggle_set_active(panel3_button_dither,1);

  /* panel 4 */
  b = rowpanel_new_row(p,1);
  rowspacer_new(b,20);
  t = panel4_slider_amp =
    rowslider_new(b,"amplitude (bits)",panel4_clicked_amp);
  rowslider_add_stop(t,"1/4",-1,0);
  rowslider_add_stop(t,"1/2",0,0);
  rowslider_add_stop(t,"1",1,0);
  rowslider_add_stop(t,"2",2,0);
  rowslider_add_stop(t,"4",4,0);
  rowslider_add_stop(t,"8",8,0);
  rowslider_add_stop(t,"16",16,0);
  rowspacer_new(b,20);

  panel4_button_dither = rowtoggle_new(b,"dither",panel4_clicked_dither);
  rowspacer_new(b,10);
  rowslider_set(t,16);

  /* panel 5 */
  b = rowpanel_new_row(p,0);

  panel5_button_notch = rowtoggle_new(b,"notch and",panel5_clicked_notch);
  rowwidget_add_label(panel5_button_notch,"gain",1);

  rowspacer_new(b,40);

  panel5_button_flat = rowtoggle_new(b,"flat",panel5_clicked_shape);
  panel5_button_shaped = rowtoggle_new(b,"shaped",panel5_clicked_shape);
  rowtoggle_set_active(panel5_button_flat,1);

  rowspacer_new(b,40);

  panel5_button_dither = rowtoggle_new(b,"dither",panel5_clicked_dither);

  /* panel 6 */
  b = rowpanel_new_row(p,1);

  rowspacer_new(b,0);
  panel6_button_notch = rowtoggle_new(b,"notch and",panel6_clicked_notch);
  rowwidget_add_label(panel6_button_notch,"gain",1);
  panel6_button_modulate = rowtoggle_new(b,"modulate",panel6_clicked_modulate);
  rowwidget_add_label(panel6_button_modulate,"input",1);

  rowspacer_new(b,10);
  t = panel6_slider_amp = rowslider_new(b,"dither  ",panel6_clicked_dithamp);
  rowslider_add_stop(t,"none",0,0);
  rowslider_add_stop(t,"",.1,0);
  rowslider_add_stop(t,"",.2,0);
  rowslider_add_stop(t,"",.3,0);
  rowslider_add_stop(t,"",.4,0);
  rowslider_add_stop(t,"",.5,0);
  rowslider_add_stop(t,"",.6,0);
  rowslider_add_stop(t,"",.7,0);
  rowslider_add_stop(t,"",.8,0);
  rowslider_add_stop(t,"",.9,0);
  rowslider_add_stop(t,"full",1,0);

  rowspacer_new(b,10);

  panel6_button_flat = rowtoggle_new(b,"flat",panel6_clicked_shape);
  panel6_button_shaped = rowtoggle_new(b,"shaped",panel6_clicked_shape);

  rowspacer_new(b,0);
  rowslider_set(t,1.0);

  /* panel 7 */
  b = rowpanel_new_row(p,0);
  rowlabel_new(b, "filter: "
               "<span color=\"#a0c0ff\">"
               "cutoff=</span>"
               "20.5kHz "
               "<span color=\"#a0c0ff\">"
               "rolloff="
               "</span>"
               "-100dB @ 21kHz");

  /* panel 8 */
  b = rowpanel_new_row(p,1);

  rowspacer_new(b,25);

  panel8_button_square = rowtoggle_new(b,"generate",panel8_clicked_square);
  rowwidget_add_label(panel8_button_square,"square wave",1);

  rowspacer_new(b,30);

  t = panel8_slider_offset =
    rowslider_new(b,"sample offset",panel8_clicked_offset);
  rowslider_add_stop(t,"-1.0",-1,0);
  rowslider_add_stop(t,"",-.8,0);
  rowslider_add_stop(t,"",-.6,0);
  rowslider_add_stop(t,"",-.4,0);
  rowslider_add_stop(t,"",-.2,0);
  rowslider_add_stop(t,"0",0,0);
  rowslider_add_stop(t,"",.2,0);
  rowslider_add_stop(t,"",.4,0);
  rowslider_add_stop(t,"",.6,0);
  rowslider_add_stop(t,"",.8,0);
  rowslider_add_stop(t,"1.0",1,0);
  rowspacer_new(b,20);
  rowslider_set(t,0);

  /* panel 9 */
  b = rowpanel_new_row(p,1);
  rowspacer_new(b,60);
  t = panel9_slider_spread =
    rowslider_new(b,"sample spread",panel9_clicked_spread);
  rowslider_add_stop(t,"-1.0",-1,0);
  rowslider_add_stop(t,"",-.8,0);
  rowslider_add_stop(t,"",-.6,0);
  rowslider_add_stop(t,"",-.4,0);
  rowslider_add_stop(t,"",-.2,0);
  rowslider_add_stop(t,"0",0,0);
  rowslider_add_stop(t,"",.2,0);
  rowslider_add_stop(t,"",.4,0);
  rowslider_add_stop(t,"",.6,0);
  rowslider_add_stop(t,"",.8,0);
  rowslider_add_stop(t,"1.0",1,0);
  rowspacer_new(b,50);
  rowslider_set(t,0);

  /* panel 10 */
  b = rowpanel_new_row(p,0);
  panel10_readout_hw = rowreadout_new
    (b, "<span foreground=\"#c0c0c0\">hw:1</span>");

  rowspacer_new(b,20);

  panel10_button_silence =
    rowtoggle_new(b,"silence",panel10_clicked_output);
  panel10_button_tone1 =
    rowtoggle_new(b,"1kHz tone",panel10_clicked_output);
  panel10_button_tone5 =
    rowtoggle_new(b,"5kHz tone",panel10_clicked_output);
  panel10_button_noise =
    rowtoggle_new(b,"white noise",panel10_clicked_output);
  panel10_button_sweep =
    rowtoggle_new(b,"linear sweep",panel10_clicked_output);
  panel10_button_logsweep =
    rowtoggle_new(b,"log sweep",panel10_clicked_output);

  rowspacer_new(b,20);

  panel10_button_duallisten =
    rowtoggle_new(b,"two-input",panel10_clicked_duallisten);
  rowwidget_add_label(panel10_button_duallisten,"mode",1);

  rowtoggle_set_active(panel10_button_silence,1);
  if(current_panel!=10)request_output_silence=0;
  ready=1;
  panelchange(p); /* force setup to be consistent with current panel */
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
     "style \"rowlabel\" {"
     "  font_name = \"sans 13 \""
     "  fg[NORMAL]=\"#cccccc\""
     "}"
     "class \"*\" style \"panel\""
     "widget \"*.topframe.GtkLabel\" style \"topframe\""
     "widget \"*.topframe*.rowlabel*\" style \"rowlabel\""
     );

  /* easiest way to inform gtk of changes and not deal with locking
     issues around the UI */
  if(pipe(eventpipe)){
    fprintf(stderr,"Unable to open event pipe:\n"
            "  %s\n",strerror(errno));
    return 1;
  }

  /* Allows event compression on the read side */
  if(fcntl(eventpipe[0], F_SETFL, O_NONBLOCK)){
    fprintf(stderr,"Unable to set O_NONBLOCK on event pipe:\n"
            "  %s\n",strerror(errno));
    return 1;
  }

  /* Tell glib to watch the notificaiton pipe in gtk_main() */
  GIOChannel *channel = g_io_channel_unix_new (eventpipe[0]);
  g_io_channel_set_encoding (channel, NULL, NULL);
  g_io_channel_set_buffered (channel, FALSE);
  g_io_channel_set_close_on_unref (channel, TRUE);
  g_io_add_watch (channel, G_IO_IN, async_event_handle, NULL);
  g_io_channel_unref (channel);

  /* go */
  make_panel();

  struct sched_param sched;
  int policy,s;
  sched.sched_priority = 99;
  pthread_create(&io_thread_id,NULL,&io_thread,NULL);

  if(pthread_setschedparam(io_thread_id, SCHED_FIFO, &sched)){
    fprintf(stderr,"Unable to set realtime scheduling on io thread\n");
  }

  if(pthread_getschedparam(io_thread_id, &policy, &sched)){
    fprintf(stderr,"Unable to check realtime scheduling on io thread\n");
  }

  printf("   io thread policy=%s, priority=%d\n",
         (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
         (policy == SCHED_RR)    ? "SCHED_RR" :
         (policy == SCHED_OTHER) ? "SCHED_OTHER" :
         "???",
         sched.sched_priority);


  sched.sched_priority = 90;
  if(pthread_setschedparam(pthread_self(), SCHED_FIFO, &sched)){
    fprintf(stderr,"Unable to set realtime scheduling on main thread\n");
  }

  if(pthread_getschedparam(pthread_self(), &policy, &sched)){
    fprintf(stderr,"Unable to check realtime scheduling on main thread\n");
  }

  printf("   main thread policy=%s, priority=%d\n",
         (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
         (policy == SCHED_RR)    ? "SCHED_RR" :
         (policy == SCHED_OTHER) ? "SCHED_OTHER" :
         "???",
         sched.sched_priority);

  gtk_main();

  return 0;
}
