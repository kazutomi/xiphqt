#include "gtk-bounce.h"
#include "gtk-bounce-filter.h"
#include "gtk-bounce-dither.h"
#include <time.h>
#include <alsa/asoundlib.h>

extern int header_out(FILE *out, int rate, int bits, int channels);

static snd_pcm_uframes_t pframes = 128;
static snd_pcm_uframes_t pframes_mark = 128*8;
static snd_pcm_uframes_t bframes = 4096;

/* only apply to ch0 and ch1 */

static shapestate shapestate_ch0;
static shapestate shapestate_ch1;

static notchfilter *notch[2]={0,0};
static convofilter *lowpass;
static convostate *lowpass_state[2]={0,0};
static int active_1kHz_notch;
static int active_lp_filter;
static int state_1kHz_modulation = 0;
static float amplitude_1kHz_modmix = 0;

static int param_setup_i(snd_pcm_t *devhandle, int dir, int rate,
                         int format, int channels){
  int ret;
  snd_pcm_hw_params_t *hw;
  snd_pcm_sw_params_t *sw;
  char *prompt = (!dir ? "capture" : "playback");

  /* allocate the parameter structures */
  snd_pcm_hw_params_alloca (&hw);
  snd_pcm_sw_params_alloca (&sw);

  /* fetch all possible hardware parameters */
  if ((ret = snd_pcm_hw_params_any (devhandle, hw)) < 0) {
    fprintf (stderr, "%s cannot fetch hardware parameters (%s)\n",
             prompt, snd_strerror (ret));
    return ret;
  }

  /* use file-like syscall interface as opposed to mmap */
  if ((ret = snd_pcm_hw_params_set_access (devhandle, hw, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fprintf (stderr, "%s cannot set access type (%s)\n",
             prompt, snd_strerror (ret));
    return ret;
  }

  /* set the sample bitformat */
  if ((ret = snd_pcm_hw_params_set_format (devhandle, hw, format)) < 0) {
    fprintf (stderr, "%s cannot set sample format (%s)\n",
             prompt, snd_strerror (ret));
    return ret;
  }

  /* set the sample rate */
  if ((ret = snd_pcm_hw_params_set_rate (devhandle, hw, rate, 0)) < 0) {
    fprintf (stderr, "%s cannot set sample rate (%s)\n",
             prompt, snd_strerror (ret));
    return ret;
  }

  /* set the number of channels */
  if ((ret = snd_pcm_hw_params_set_channels (devhandle, hw, channels)) < 0) {
    fprintf (stderr, "%s cannot set channel count (%s)\n",
             prompt, snd_strerror (ret));
    return ret;
  }

  /* set the time per hardware sample transfer */
  if ((ret = snd_pcm_hw_params_set_period_size_near(devhandle,
                                                      hw, &pframes, 0)) < 0) {

    fprintf (stderr, "%s cannot set period size (%s)\n",
             prompt, snd_strerror (ret));
    return ret;
  }

  /* set the length of the hardware sample buffer */
  if ((ret = snd_pcm_hw_params_set_buffer_size_near(devhandle,
                                                    hw, &bframes)) < 0) {

    fprintf (stderr, "%s cannot set buffer size (%s)\n",
             prompt, snd_strerror (ret));
    return ret;
  }

  if ((ret = snd_pcm_hw_params (devhandle, hw)) < 0) {
    fprintf (stderr, "%s cannot set hard parameters (%s)\n",
             prompt, snd_strerror (ret));
    return ret;
  }

  if((ret = snd_pcm_hw_params_get_period_size(hw, &pframes, 0)) < 0){
    fprintf (stderr, "%s cannot query period size (%s)\n",
             prompt, snd_strerror (ret));
    return ret;
  }

  if((ret = snd_pcm_hw_params_get_buffer_size(hw, &bframes)) < 0){
    fprintf (stderr, "%s cannot query buffer size (%s)\n",
             prompt, snd_strerror (ret));
    return ret;
  }

  /* fetch software parameter settings */
  if ((ret = snd_pcm_sw_params_current (devhandle, sw)) < 0) {
    fprintf (stderr, "%s cannot initialize hardware parameter structure (%s)\n",
             prompt, snd_strerror (ret));
    return ret;
  }

  /* start transfers at lowest possible buffer watermark */
  if(dir){
    if ((ret = snd_pcm_sw_params_set_start_threshold (devhandle, sw, pframes_mark)) < 0) {
      fprintf (stderr, "%s cannot set start threshold (%s)\n",
               prompt, snd_strerror (ret));
      return ret;
    }
  }else{
    if ((ret = snd_pcm_sw_params_set_start_threshold (devhandle, sw, pframes)) < 0) {
      fprintf (stderr, "%s cannot set start threshold (%s)\n",
               prompt, snd_strerror (ret));
      return ret;
    }
  }
  if ((ret = snd_pcm_sw_params_set_avail_min (devhandle, sw, pframes)) < 0) {
    fprintf (stderr, "%s cannot set minimum activity threshold (%s)\n",
             prompt,snd_strerror (ret));
    return ret;
  }

  if ((ret = snd_pcm_sw_params (devhandle, sw)) < 0) {
    fprintf (stderr, "%s cannot set soft parameters (%s)\n",
             prompt, snd_strerror (ret));
    return ret;
  }

  /* go time */
  if ((ret = snd_pcm_prepare (devhandle)) < 0) {
    fprintf (stderr, "%s cannot prepare audio interface for use (%s)\n",
             prompt,snd_strerror (ret));
    return ret;
  }

  return 0;
}

static int param_setup(snd_pcm_t *devhandle, int dir, int rate,
                       int bits, int *channels){

  /* The eMagic 2|6 only does 24 bit playback if also using 6
     channels.  The Focusrite Scarlett 2i2 can only do 32 bit
     output */
  int format_list[3] = {SND_PCM_FORMAT_S16_LE,
                        SND_PCM_FORMAT_S24_3LE,
                        SND_PCM_FORMAT_S32_LE};
  int channel_list[2] = {2,6};

  int fi = (bits==16?0:1);
  int ci;

  for(;fi<3;fi++){
    for(ci=0;ci<2;ci++){
      if(!param_setup_i(devhandle,dir,rate,format_list[fi],channel_list[ci])){
        *channels = channel_list[ci];
        return format_list[fi];
      }
    }
  }
  return -1;
}

static void alsa_to_dev_null(const char *file,
                               int line,
                               const char *function,
                               int err,
                               const char *fmt, ...){
}

/* capture open blocks as we can't do anything much without a capture
   clock */
static snd_pcm_t *open_capture(char *dev, int rate, int bits, int *channels, int *format){
  snd_pcm_t *handle;
  int ret,fail=0;
  struct timespec t = {0,250000000};
  while((ret=snd_pcm_open(&handle, dev, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
    if(ret!=-ENOENT){
      /* A device that's in the middle of initializing may throw a
         random error */
      fail++;
      if(fail==3)return NULL;
    }
    nanosleep(&t,NULL);
  }

  *format = param_setup(handle,0,rate,bits,channels);
  if(*format<0){
    snd_pcm_close(handle);
    return NULL;
  }

  memset(&shapestate_ch0,0,sizeof(shapestate_ch0));
  memset(&shapestate_ch1,0,sizeof(shapestate_ch1));

  return handle;
}

/* don't block; try it and if we fail, fail quickly */
static snd_pcm_t *open_playback(char *dev, int rate, int bits,
                                int *channels, int *format){
  snd_pcm_t *handle;
  int ret;

  if ((ret = snd_pcm_open (&handle, dev, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    return NULL;
  }

  *format = param_setup(handle,1,rate,bits,channels);
  if(*format<0){
    snd_pcm_close(handle);
    return NULL;
  }

  return handle;
}

static float *square_coeffs;
static float *square_phases;
static int square_coeffs_n;
static double square_coeffs_w;

/* brute-force the expansion of a synthetic aliased 'squarewave';
   choose a fundamental that evenly divides the rate, doesn't land a
   harmonic at nyquist, and has an integer period, so that each folded
   component lands directly on another. */
/* we don't adjust for phase through the complete aliased expansion
   during later generation because this isn't a real squarewave.  The
   additional mirrored components exist only to fill in the illusory
   appearance of it being a squarewave when drawn with a zero-hold at
   phase=0 */

void generate_squarewave_expansion(int rate){
  int period = rint(rate/4000.)*4;
  //double freq = rate/period;
  double w = 2./period*M_PI;
  int coeffs = period/4;
  int i,j;
  float *work = fftwf_malloc((period+2)*sizeof(*work));
  fftwf_plan plan = fftwf_plan_dft_r2c_1d(period,work,
                                          (fftwf_complex *)work,
                                          FFTW_ESTIMATE);

  for(i=0;i<period/2;i++)
    work[i]=1.;
  for(;i<period;i++)
    work[i]=-1.;
  fftwf_execute(plan);

  square_coeffs_n = coeffs;
  square_coeffs_w = w;
  if(square_coeffs)free(square_coeffs);
  if(square_phases)free(square_phases);
  square_coeffs = calloc(coeffs,sizeof(*square_coeffs));
  square_phases = calloc(coeffs,sizeof(*square_phases));

  for(i=1,j=0;j<square_coeffs_n;i+=2,j++){
    square_coeffs[j] = hypotf(work[i<<1],work[(i<<1)+1]) / period;
    square_phases[j] = atan2f(work[(i<<1)+1],work[i<<1]);
  }

}

void *io_thread(void *dummy){
  snd_pcm_t *in_devhandle=NULL;
  snd_pcm_t *out_devhandle=NULL;
  FILE *outpipe0=NULL;
  FILE *outpipe1=NULL;
  pole2 filter_1kHz_amplitude;
  pole2 filter_5kHz_amplitude;
  pole2 filter_1kHz_mix;
  pole2 filter_output_noise;
  pole2 filter_output_sweep;
  pole2 filter_modulation1;
  pole2 filter_modulation2;

  int channels = 2; //request_ch;
  int in_channels = 2;
  int out_channels = 2;
  int rate = request_rate;
  int bits = request_bits;
  int informat=-1;
  int outformat=-1;

  int i,j;

  unsigned char *iobuffer=NULL;
  float *crossbuffer;
  float *fbuffer[32];

  filter_make_critical(.0001,1,&filter_1kHz_amplitude);
  filter_make_critical(.0001,1,&filter_5kHz_amplitude);
  filter_make_critical(.0001,1,&filter_1kHz_mix);
  filter_make_critical(.0001,1,&filter_output_noise);
  filter_make_critical(.0001,1,&filter_output_sweep);
  filter_make_critical(.00005,2,&filter_modulation1);
  filter_make_critical(.00005,2,&filter_modulation2);

  /* libasound prints debugging output directly to stderr; tell it to
     shut up */
  snd_lib_error_set_handler(alsa_to_dev_null);

  while(!exiting){

    if(request_bits != bits || request_rate!=rate){
      /* the panel has requested a differnet bitdepth than we're running.
         Close any already-open devices, reopen below */
      bits=request_bits;
      //channels=request_ch;
      rate=request_rate;

      if(notch[0])filter_notch_destroy(notch[0]);
      notch[0]=NULL;
      if(notch[1])filter_notch_destroy(notch[1]);
      notch[1]=NULL;
      if(lowpass)convofilter_destroy(lowpass);
      lowpass=NULL;

      if(lowpass_state[0])convostate_destroy(lowpass_state[0]);
      lowpass_state[0]=NULL;
      if(lowpass_state[1])convostate_destroy(lowpass_state[1]);
      lowpass_state[1]=NULL;

      if(in_devhandle){
        snd_pcm_close(in_devhandle);
        in_devhandle=NULL;
        write(eventpipe[1],"\002",1);
      }
      if(out_devhandle){
        snd_pcm_close(out_devhandle);
        out_devhandle=NULL;
        write(eventpipe[1],"\002",1);
      }
      if(outpipe0) fclose(outpipe0);
      if(outpipe1) fclose(outpipe1);

      if(outpipe0 || outpipe1){
        /* solve a race with a sleep; we want the readers to realize
           this closed */
        struct timespec t = {0,250000000};
        nanosleep(&t,NULL);
        outpipe0=NULL;
        outpipe1=NULL;

        if(in_devhandle)
          snd_pcm_drop(in_devhandle);
        if(out_devhandle)
          snd_pcm_drop(out_devhandle);
        if(in_devhandle)
          snd_pcm_prepare(in_devhandle);
        if(out_devhandle)
          snd_pcm_prepare(out_devhandle);

      }
    }

    if(!outpipe0){
      int fd = open("pipe0", O_WRONLY|O_NONBLOCK);
      if(fd>=0){
        outpipe0 = fdopen(fd,"wb");
        if(outpipe0==NULL){
          close(fd);
          write(eventpipe[1],"\000",1);
        }
        header_out(outpipe0,rate,bits,channels);
        fprintf(stderr,"Opened outpipe0\n");
      }
    }

    if(!outpipe1){
      int fd = open("pipe1", O_WRONLY|O_NONBLOCK);
      if(fd>=0){
        outpipe1 = fdopen(fd,"wb");
        if(outpipe1==NULL){
          close(fd);
          write(eventpipe[1],"\000",1);
        }
        header_out(outpipe1,rate,bits,channels);
        fprintf(stderr,"Opened outpipe1\n");
      }
    }

    if(!in_devhandle || !out_devhandle){


      /* assume that hw:1 is always our capture handle, and if we don't
         have capture, we can't do anything */
      if(!in_devhandle){
        in_channels = channels;
        in_devhandle = open_capture("hw:1",rate,bits,&in_channels,&informat);

        if(in_devhandle){
          /* configure/reconfigure buffering for i/o */
          int maxchannels=8;
          if(iobuffer){
            iobuffer=realloc(iobuffer,pframes*maxchannels*4);
            for(i=0;i<maxchannels;i++)
              fbuffer[i]=realloc(fbuffer[i],pframes*sizeof(**fbuffer));
            crossbuffer=realloc(crossbuffer,pframes*sizeof(**fbuffer));
          }else{
            iobuffer=malloc(pframes*maxchannels*4);
            for(i=0;i<maxchannels;i++)
              fbuffer[i]=malloc(pframes*sizeof(**fbuffer));
            crossbuffer=malloc(pframes*sizeof(**fbuffer));
          }

          /* squarewave */
          generate_squarewave_expansion(rate);

          /* set-up our on-demand filtering that has
             expensive-to-construct persistent state */

          notch[0]=filter_notch_new(1000.,request_rate);
          lowpass=filter_lowpass_new(20500,request_rate,512);

          lowpass_state[0] = convostate_new(lowpass);
          if(channels>1){
            notch[1]=filter_notch_new(1000.,request_rate);
            lowpass_state[1] = convostate_new(lowpass);
          }
        }
      }

      if(!out_devhandle){
        out_channels = channels;
        out_devhandle = open_playback("hw:1",rate,bits,
                                      &out_channels,&outformat);
      }

      if(in_devhandle && out_devhandle){
        char *name;
        snd_card_get_name(1,&name);
        unsigned char len = strlen(name)+1;
        write(eventpipe[1],"\001",1);
        write(eventpipe[1],&len,1);
        write(eventpipe[1],name,(int)len);
      }
    }

    /* capture a block of input */
    if(in_devhandle){
      int frames = snd_pcm_readi (in_devhandle, iobuffer, pframes);
      if(frames<0){
        switch(frames){
        case -EAGAIN:
          break;
        case -EPIPE: case -ESTRPIPE:
          // underrun; set starve and reset soft device
          snd_pcm_drop(in_devhandle);
          if(out_devhandle)
            snd_pcm_drop(out_devhandle);
          snd_pcm_prepare(in_devhandle);
          if(out_devhandle)
            snd_pcm_prepare(out_devhandle);
          frames=0;
          fprintf(stderr,"\n CAPTURE XRUN @ (%ld)\n ", (long)time(NULL));
          break;
        case -EBADFD:
        default:
          /* shut down device */
          snd_pcm_close(in_devhandle);
          in_devhandle=NULL;
          write(eventpipe[1],"\002",1);
          frames=0;

          if(outpipe0) fclose(outpipe0);
          if(outpipe1) fclose(outpipe1);

          if(outpipe0 || outpipe1){
            /* solve a race with a sleep; we want the readers to realize
               this closed */
            struct timespec t = {0,250000000};
            nanosleep(&t,NULL);
            outpipe0=NULL;
            outpipe1=NULL;
          }

          break;
        }
      }else{

        /* convert our frames to 32-bit float intermediary */
        unsigned char *bufp = iobuffer;
        switch(informat){
        case SND_PCM_FORMAT_S32_LE:
          for(i=0;i<frames;i++){
            for(j=0;j<in_channels;j++){
              int32_t in = bufp[0]|(bufp[1]<<8)|(bufp[2]<<16)|(bufp[3]<<24);
              fbuffer[j][i]= in * (1./128./256./256./256.);
              bufp+=4;
            }
            for(;j<channels;j++) fbuffer[j][i]=0;
          }
          break;
        case SND_PCM_FORMAT_S32_BE:
          for(i=0;i<frames;i++){
            for(j=0;j<channels;j++){
              int32_t in = bufp[3]|(bufp[2]<<8)|(bufp[1]<<16)|(bufp[0]<<24);
              fbuffer[j][i]= in * (1./128./256./256./256.);
              bufp+=4;
            }
            for(;j<channels;j++) fbuffer[j][i]=0;
          }
          break;
        case SND_PCM_FORMAT_S24_LE:
          for(i=0;i<frames;i++){
            for(j=0;j<channels;j++){
              int32_t in = (bufp[1]<<8)|(bufp[2]<<16)|(bufp[3]<<24);
              fbuffer[j][i]= in * (1./128./256./256./256.);
              bufp+=4;
            }
            for(;j<channels;j++) fbuffer[j][i]=0;
          }
          break;
        case SND_PCM_FORMAT_S24_BE:
          for(i=0;i<frames;i++){
            for(j=0;j<channels;j++){
              int32_t in = (bufp[2]<<8)|(bufp[1]<<16)|(bufp[0]<<24);
              fbuffer[j][i]= in * (1./128./256./256./256.);
              bufp+=4;
            }
            for(;j<channels;j++) fbuffer[j][i]=0;
          }
          break;
        case SND_PCM_FORMAT_S24_3LE:
          for(i=0;i<frames;i++){
            for(j=0;j<channels;j++){
              int32_t in = (bufp[0]<<8)|(bufp[1]<<16)|(bufp[2]<<24);
              fbuffer[j][i]= in * (1./128./256./256./256.);
              bufp+=3;
            }
            for(;j<channels;j++) fbuffer[j][i]=0;
          }
          break;
        case SND_PCM_FORMAT_S24_3BE:
          for(i=0;i<frames;i++){
            for(j=0;j<channels;j++){
              int32_t in = (bufp[2]<<8)|(bufp[1]<<16)|(bufp[0]<<24);
              fbuffer[j][i]= in * (1./128./256./256./256.);
              bufp+=3;
            }
            for(;j<channels;j++) fbuffer[j][i]=0;
          }
          break;
        case SND_PCM_FORMAT_S16_LE:
          for(i=0;i<frames;i++){
            for(j=0;j<channels;j++){
              int32_t in = (bufp[0]<<16)|(bufp[1]<<24);
              fbuffer[j][i]= in * (1./128./256./256./256.);
              bufp+=2;
            }
            for(;j<channels;j++) fbuffer[j][i]=0;
          }
          break;
        case SND_PCM_FORMAT_S16_BE:
          for(i=0;i<frames;i++){
            for(j=0;j<channels;j++){
              int32_t in = (bufp[1]<<16)|(bufp[0]<<24);
              fbuffer[j][i]= in * (1./128./256./256./256.);
              bufp+=2;
            }
            for(;j<channels;j++) fbuffer[j][i]=0;
          }
          break;
        }

        //if(!(request_output_silence ||
        //   request_output_tone ||
        //   request_output_noise ||
        //   request_output_sweep ||
        //   request_output_logsweep)){
        ///* not in panel 10; mirror input 0 into input 1 */
        //memcpy(fbuffer[1],fbuffer[0],sizeof(**fbuffer)*frames);
        //}

        /* save the ch0 input in two-input mode */
        if(request_output_duallisten && channels>1){
          float *temp = crossbuffer;
          crossbuffer=fbuffer[0];
          fbuffer[0]=temp;
        }

        if(request_output_silence){
          for(i=0;i<frames;i++)
            fbuffer[0][i]=0;
        }

        /* input lowpass */
        if((request_lp_filter || active_lp_filter) && !request_1kHz_square){
          active_lp_filter =
            run_convolution_filter(lowpass,lowpass_state[0],
                                   fbuffer[0],
                                   frames,0,request_lp_filter,1.);

          if(channels>1)
            active_lp_filter |=
              run_convolution_filter(lowpass,lowpass_state[1],
                                     fbuffer[1],
                                     frames,0,request_lp_filter,1.);
        }

        /* panel 10 white noise burst */
        static int count_output_noise;
        static float mix_output_noise;
        if(request_output_noise || mix_output_noise>0.){
          float target = request_output_noise ? 1. : 0.;
          if(request_output_noise && count_output_noise==0)
            count_output_noise = 10 * rate;

          if(fabsf(mix_output_noise - target) > .0000001){
            float m;
            for(i=0;i<frames;i++){
              float v = .99*(drand48()-drand48());
              m = filter_filter(target,&filter_output_noise);
              fbuffer[0][i] =  v*m;
            }
            mix_output_noise = m;
          }else{
            mix_output_noise = target;
            if(request_output_noise){
              for(i=0;i<frames;i++){
                float v = .99*(drand48()-drand48());
                fbuffer[0][i] =  v;
              }
            }else
              count_output_noise=0;
          }
          count_output_noise-=frames;
          if(count_output_noise<=0){
            count_output_noise=0;
            request_output_noise=0;
            write(eventpipe[1],"\006",1);
          }
        }

        /* panel 10 linear/log sweep  */
        static float mix_output_sweep=0.;
        if(request_output_sweep ||
           request_output_logsweep ||
           mix_output_sweep){
          float target =
            (request_output_sweep||request_output_logsweep) ? 1. : 0.;

          double totalsamples = rate * 20.;
          float m;

          static int logp;
          static double theta = 0.;
          static double samples = 0.;
          static double d;
          double l0 = log(5./rate*2*M_PI);
          double l1 = log(M_PI);
          double dd = logp ? (l1-l0)/totalsamples : M_PI/totalsamples;

          if(request_output_logsweep)logp=1;
          if(request_output_sweep)logp=0;
          if(samples==0.){
            //filter_set(&filter_output_sweep,1.);
            d = logp ? l0 : 0;
          }

          for(i=0;i<frames;i++){
            double instf = logp ? exp(d) : d;
            float v = .99*sin(theta);
            m = filter_filter(target,&filter_output_sweep);

            d+=dd;
            theta+=instf;
            samples++;

            if(theta>2*M_PI)theta-=2.*M_PI;
            if(totalsamples-samples<2000){
              target=0;
              request_output_sweep=0;
              request_output_logsweep=0;
            }
            fbuffer[0][i] =  v*m;
          }

          mix_output_sweep=m;
          if(!request_output_sweep &&
             !request_output_logsweep &&
             m<.00000001){
            mix_output_sweep=0;
            samples=0;
            theta=0;
            write(eventpipe[1],"\006",1);
          }
        }

        /* synthetic ~ 1kHz 'squarewave' */
        /* generate it via previously computed Fourier expansion */
        if(request_1kHz_square){
          static double phase=0.;
          double offset = request_1kHz_square_offset/65535.*square_coeffs_w;
          double spread = offset + request_1kHz_square_spread/65535.*square_coeffs_w;

          for(i=0;i<frames;i++){
            float acc=0.;
            int k;
            for(j=0,k=1;j<square_coeffs_n;j++,k+=2)
              acc += square_coeffs[j] *
                cos( square_phases[j] + k*(phase+offset));
            fbuffer[0][i] = acc;

            if(channels>1 && spread!=offset){
              acc=0;
              for(j=0,k=1;j<square_coeffs_n;j++,k+=2)
                acc += square_coeffs[j] *
                  cos( square_phases[j] + k*(phase+spread));
              fbuffer[1][i] = acc;
            }else
              fbuffer[1][i] = fbuffer[0][i];

            phase+=square_coeffs_w;
            if(phase>=2*M_PI)phase-=2*M_PI;
          }
        }

        /* synthetic ~ 5kHz generation */
        static double amplitude_5kHz_sine=0;
        if(request_5kHz_sine || amplitude_5kHz_sine>0.){
          static double phase=0.;
          double w = 1./rate*5000.*2*M_PI;

          /* avoid clipping when dithering at 8 bits */
          float amp_target = request_5kHz_sine ?
            (1.-1.5f/pow(2,7)) *
            (pow(2,request_1kHz_amplitude/65536.f-1.f)/32768.):
            0.;

          if(fabsf(amplitude_5kHz_sine - amp_target) > .0000001){

            for(i=0;i<frames;i++){

              amplitude_5kHz_sine =
                filter_filter(amp_target, &filter_5kHz_amplitude);

              float v = sinf(phase)*amplitude_5kHz_sine;
              fbuffer[0][i] = v;
              phase+=w;
              if(phase>=2*M_PI)phase-=2*M_PI;
            }
          }else{
            amplitude_5kHz_sine = amp_target;
            if(request_5kHz_sine){
              for(i=0;i<frames;i++){
                float v = sinf(phase)*amplitude_5kHz_sine;
                fbuffer[0][i] =  v;
                phase+=w;
                if(phase>=2*M_PI)phase-=2*M_PI;
              }
            }
          }
        }

        /* synthetic ~ 1kHz generation; we choose a value that's an
           integer factor of the sample rate */
        static double amplitude_1kHz_sine=0;
        static double mix_1kHz_sine=0;
        if(request_1kHz_sine || mix_1kHz_sine>0.){
          static double phase=0.;
          int quadrant=rate/2;
          int period = ceil(rate/1000);
          double w = 1./period*2*M_PI;

          /* avoid clipping when dithering at 8 bits */
          float amp_target = (1.-1.5f/pow(2,7)) *
            (pow(2,request_1kHz_amplitude/65536.f-1.f)/32768.);
          float mix_target = request_1kHz_sine ? 1. : 0.;
          float modmix_target = request_1kHz_modulate ? 1. : 0.;

          if(amplitude_1kHz_modmix > 0. ||
             modmix_target > 0. ||
             fabsf(amplitude_1kHz_sine - amp_target) > .0000001 ||
             fabsf(mix_1kHz_sine - mix_target) > .0000001){

            for(i=0;i<frames;i++){

              float first =
                filter_filter(modmix_target, &filter_modulation1);
              amplitude_1kHz_modmix =
                filter_filter(first, &filter_modulation2);

              float mod = sinf(.5*M_PI*state_1kHz_modulation/quadrant)*
                amplitude_1kHz_modmix;

              state_1kHz_modulation++;
              if(state_1kHz_modulation>=2*quadrant)
                state_1kHz_modulation=0;
              mod *= mod;
              mod = fromdB(mod*-110);

              amplitude_1kHz_sine =
                filter_filter(amp_target, &filter_1kHz_amplitude)*mod;

              mix_1kHz_sine =
                filter_filter(mix_target, &filter_1kHz_mix);

              float v = sinf(phase)*amplitude_1kHz_sine;
              fbuffer[0][i] *= (1.-mix_1kHz_sine);
              fbuffer[0][i] +=  v*mix_1kHz_sine;
              if(channels>1 && request_1kHz_sine2){
                fbuffer[1][i] *= (1.-mix_1kHz_sine);
                fbuffer[1][i] +=  v*mix_1kHz_sine;
              }
              phase+=w;
              if(phase>=2*M_PI)phase-=2*M_PI;
            }
          }else{
            amplitude_1kHz_modmix = 0;
            amplitude_1kHz_sine = amp_target;
            mix_1kHz_sine = mix_target;
            state_1kHz_modulation = 0;

            if(request_1kHz_sine){
              for(i=0;i<frames;i++){
                float v = sinf(phase)*amplitude_1kHz_sine;
                fbuffer[0][i] =  v;
                if(channels>1 && request_1kHz_sine2)
                  fbuffer[1][i] =  v;
                phase+=w;
                if(phase>=2*M_PI)phase-=2*M_PI;
              }
            }
          }
        }

        /* conditionally subquantize and dither output */
        subquant_dither_to_X(&shapestate_ch0,fbuffer[0],frames,
                             request_ch1_quant);
        if(channels>1)
          subquant_dither_to_X(&shapestate_ch1,fbuffer[1],frames,
                               request_ch2_quant);

        /* notch filter has to come after dither as it's sort of the
           point...  The notch filter would normally break the dither
           (it would be ~equivalent to adding an out-of-phase since
           wave that is not itself dithered, and thus upon final quant
           it adds distortion), but because we add gain with the
           notch, it's OK */
        if(request_1kHz_notch ||
           active_1kHz_notch ||
           prime_1kHz_notch){
          float notch_gain_target = fromdB( 6*request_ch2_quant-30);

          active_1kHz_notch =
            run_notch_filter(notch[0],
                             fbuffer[0],
                             frames,
                             prime_1kHz_notch,
                             request_1kHz_notch,
                             notch_gain_target);
          if(channels>1)
            active_1kHz_notch |=
              run_notch_filter(notch[1],
                               fbuffer[1],
                               frames,
                               prime_1kHz_notch,
                               request_1kHz_notch,
                               notch_gain_target);
        }

        /* in dual listen mode, pipe sees original ch0 and ch1 */
        if(request_output_duallisten && channels>1){
          float *temp = crossbuffer;
          crossbuffer=fbuffer[0];
          fbuffer[0]=temp;
        }

        /* float buffer -> out pipe */
        {
          unsigned char *buf = iobuffer;
          switch(bits){
          case 16:
            for(i=0;i<frames;i++){
              for(j=0;j<channels;j++){
                int v = rint(fbuffer[j][i]*32768.f);
                if(v>32767)v=32767;
                if(v<-32768)v=-32768.;
                *buf++=v&0xff;
                *buf++=(v>>8)&0xff;
              }
            }
            break;
          case 24:
            for(i=0;i<frames;i++){
              for(j=0;j<channels;j++){
                int v = rint(fbuffer[j][i]*8388608.f);
                if(v>8388607)v=8388607;
                if(v<-8388608)v=-8388608;
                *buf++=v&0xff;
                *buf++=(v>>8)&0xff;
                *buf++=(v>>16)&0xff;
              }
            }
            break;
          }

          if(outpipe0 || outpipe1){
            if(outpipe0){
              fwrite(iobuffer,1,buf-iobuffer,outpipe0);
              fflush(outpipe0);
            }
            if(outpipe1){
              fwrite(iobuffer,1,buf-iobuffer,outpipe1);
              fflush(outpipe1);
            }
          }
        }

        /* output filtering for 'perfect squarewave' */
        if((request_lp_filter || active_lp_filter) && request_1kHz_square){
          active_lp_filter =
            run_convolution_filter(lowpass,lowpass_state[0],
                                   fbuffer[0],
                                   frames,0,request_lp_filter,1.);

          if(channels>1)
            active_lp_filter |=
              run_convolution_filter(lowpass,lowpass_state[1],
                                     fbuffer[1],
                                     frames,0,request_lp_filter,1.);

        }

        /* in dual listen mode, audio device sees silence on ch1,
           generated output on ch0 */
        if(request_output_duallisten && channels>1){
          float *temp=crossbuffer;
          crossbuffer=fbuffer[0];
          fbuffer[0]=temp;
        }

        /* panel 10 modes all require silence on ch1 */
        if(request_output_silence ||
           request_output_tone ||
           request_output_noise ||
           request_output_sweep ||
           request_output_logsweep){
          for(i=0;i<frames;i++)
            fbuffer[1][i]=0;
        }

        if(out_devhandle){
        /* float buffer -> PCM stream */

          /* the output pipe and the hardware device format may not
             match */
          unsigned char *buf = iobuffer;
          switch(outformat){
          case SND_PCM_FORMAT_S16_LE:
            for(i=0;i<frames;i++){
              for(j=0;j<channels;j++){
                int v = rint(fbuffer[j][i]*32768.f);
                if(v>32767)v=32767;
                if(v<-32768)v=-32768.;
                *buf++=v&0xff;
                *buf++=(v>>8)&0xff;
              }
              for(;j<out_channels;j++){
                *buf++=0;
                *buf++=0;
              }
            }
            break;
          case SND_PCM_FORMAT_S24_3LE:
            for(i=0;i<frames;i++){
              for(j=0;j<channels;j++){
                int v = rint(fbuffer[j][i]*8388608.f);
                if(v>8388607)v=8388607;
                if(v<-8388608)v=-8388608;
                *buf++=v&0xff;
                *buf++=(v>>8)&0xff;
                *buf++=(v>>16)&0xff;
              }
              for(;j<out_channels;j++){
                *buf++=0;
                *buf++=0;
                *buf++=0;
              }
            }
            break;
          case SND_PCM_FORMAT_S24_LE:
            for(i=0;i<frames;i++){
              for(j=0;j<channels;j++){
                int v = rint(fbuffer[j][i]*8388608.f);
                if(v>8388607)v=8388607;
                if(v<-8388608)v=-8388608;
                *buf++=0;
                *buf++=v&0xff;
                *buf++=(v>>8)&0xff;
                *buf++=(v>>16)&0xff;
              }
              for(;j<out_channels;j++){
                *buf++=0;
                *buf++=0;
                *buf++=0;
                *buf++=0;
              }
            }
            break;
          case SND_PCM_FORMAT_S32_LE:
            for(i=0;i<frames;i++){
              for(j=0;j<channels;j++){
                double vv = rint(fbuffer[j][i]*2147483648.f);

                if(vv>2147483647)vv=2147483647;
                if(vv<-2147483648)vv=-2147483648;
                int v = vv;
                *buf++=v&0xff;
                *buf++=(v>>8)&0xff;
                *buf++=(v>>16)&0xff;
                *buf++=(v>>24)&0xff;
              }
              for(;j<out_channels;j++){
                *buf++=0;
                *buf++=0;
                *buf++=0;
                *buf++=0;
              }
            }
            break;
          case SND_PCM_FORMAT_S16_BE:
            for(i=0;i<frames;i++){
              for(j=0;j<channels;j++){
                int v = rint(fbuffer[j][i]*32768.f);
                if(v>32767)v=32767;
                if(v<-32768)v=-32768.;
                *buf++=(v>>8)&0xff;
                *buf++=v&0xff;
              }
              for(;j<out_channels;j++){
                *buf++=0;
                *buf++=0;
              }
            }
            break;
          case SND_PCM_FORMAT_S24_3BE:
            for(i=0;i<frames;i++){
              for(j=0;j<channels;j++){
                int v = rint(fbuffer[j][i]*8388608.f);
                if(v>8388607)v=8388607;
                if(v<-8388608)v=-8388608;
                *buf++=(v>>16)&0xff;
                *buf++=(v>>8)&0xff;
                *buf++=v&0xff;
              }
              for(;j<out_channels;j++){
                *buf++=0;
                *buf++=0;
                *buf++=0;
              }
            }
            break;
          case SND_PCM_FORMAT_S24_BE:
            for(i=0;i<frames;i++){
              for(j=0;j<channels;j++){
                int v = rint(fbuffer[j][i]*8388608.f);
                if(v>8388607)v=8388607;
                if(v<-8388608)v=-8388608;
                *buf++=(v>>16)&0xff;
                *buf++=(v>>8)&0xff;
                *buf++=v&0xff;
                *buf++=0;
              }
              for(;j<out_channels;j++){
                *buf++=0;
                *buf++=0;
                *buf++=0;
                *buf++=0;
              }
            }
            break;
          case SND_PCM_FORMAT_S32_BE:
            for(i=0;i<frames;i++){
              for(j=0;j<channels;j++){
                double vv = rint(fbuffer[j][i]*2147483648.f);
                if(vv>2147483647)vv=2147483647;
                if(vv<-2147483648)vv=-2147483648;
                int v=vv;
                *buf++=(v>>24)&0xff;
                *buf++=(v>>16)&0xff;
                *buf++=(v>>8)&0xff;
                *buf++=v&0xff;
              }
              for(;j<out_channels;j++){
                *buf++=0;
                *buf++=0;
                *buf++=0;
                *buf++=0;
              }
            }
            break;
          }

          int togo = frames;
          unsigned char *outbuf=iobuffer;
          while(togo){
            int wret = snd_pcm_writei (out_devhandle,outbuf,togo);
            if(wret<0){
              switch(wret){
              case -EAGAIN:
                wret = 0;
                break;
              case -EPIPE: case -ESTRPIPE:
                // underrun; set starve and reset soft device
                snd_pcm_drop(in_devhandle);
                snd_pcm_drop(out_devhandle);
                snd_pcm_prepare(in_devhandle);
                snd_pcm_prepare(out_devhandle);
                wret=0;
                togo=0;
                fprintf(stderr,"\n PLAYBACK XRUN @ (%ld)\n ",
                        (long)time(NULL));
              break;
              case -EBADFD:
              default:
                /* shut down device */
                snd_pcm_close(out_devhandle);
                out_devhandle=NULL;
                wret=0;
                togo=0;
                break;
              }
            }else{
              outbuf += wret*channels*((bits+7)>>3);
              togo -= wret;
            }
          }
        }
      }
    }else
      sleep(1);
  }
  return NULL;
}
