#include <stdlib.h>
#include <string.h>
#include "decint.h"



/*Unpacks a series of octets from a given byte array into the pack buffer.
  No checking is done to ensure the buffer contains enough data.
  _opb: The pack buffer to read the octets from.
  _buf: The byte array to store the unpacked bytes in.
  _len: The number of octets to unpack.*/
static void oc_unpack_octets(oggpack_buffer *_opb,char *_buf,size_t _len){
  while(_len-->0){
    long val;
    theora_read(_opb,8,&val);
    *_buf++=(char)val;
  }
}

/*Unpacks a 32-bit integer encoded by octets in little-endian form.*/
static long oc_unpack_length(oggpack_buffer *_opb){
  long ret[4];
  int  i;
  for(i=0;i<4;i++)theora_read(_opb,8,ret+i);
  return ret[0]|ret[1]<<8|ret[2]<<16|ret[3]<<24;
}

static int oc_info_unpack(oggpack_buffer *_opb,theora_info *_info){
  long val;
  /*Check the codec bitstream version.*/
  theora_read(_opb,8,&val);
  _info->version_major=(unsigned char)val;
  theora_read(_opb,8,&val);
  _info->version_minor=(unsigned char)val;
  theora_read(_opb,8,&val);
  _info->version_subminor=(unsigned char)val;
  if(_info->version_major>OC_VERSION_MAJOR||
   _info->version_major==OC_VERSION_MAJOR&&
   (_info->version_minor>OC_VERSION_MINOR||
   _info->version_minor==OC_VERSION_MINOR&&
   _info->version_subminor>OC_VERSION_SUB)){
    return OC_VERSION;
  }
  /*Read the encoded frame description.*/
  theora_read(_opb,16,&val);
  _info->width=(ogg_uint32_t)val<<4;
  theora_read(_opb,16,&val);
  _info->height=(ogg_uint32_t)val<<4;
  theora_read(_opb,24,&val);
  _info->frame_width=(ogg_uint32_t)val;
  theora_read(_opb,24,&val);
  _info->frame_height=(ogg_uint32_t)val;
  theora_read(_opb,8,&val);
  _info->offset_x=(ogg_uint32_t)val;
  /*Note: The sense of offset_y is inverted in what we pass back to the
     application compared to how it is stored in the bitstream.*/
  theora_read(_opb,8,&val);
  _info->offset_y=_info->height-_info->frame_height-(ogg_uint32_t)val;
  theora_read32(_opb,&val);
  _info->fps_numerator=(ogg_uint32_t)val;
  theora_read32(_opb,&val);
  _info->fps_denominator=(ogg_uint32_t)val;
  if(_info->width<=0||_info->height<=0||
   _info->frame_width+_info->offset_x>_info->width||
   _info->frame_height+_info->offset_y>_info->height||
   _info->fps_numerator<=0||_info->fps_denominator<=0){
    return OC_BADHEADER;
  }
  theora_read(_opb,24,&val);
  _info->aspect_numerator=(ogg_uint32_t)val;
  theora_read(_opb,24,&val);
  _info->aspect_denominator=(ogg_uint32_t)val;
  theora_read(_opb,8,&val);
  _info->colorspace=(theora_colorspace)val;
  theora_read(_opb,24,&val);
  _info->target_bitrate=(int)val;
  theora_read(_opb,6,&val);
  _info->quality=(int)val;
  theora_read(_opb,5,&val);
  _info->keyframe_granule_shift=(int)val;
  theora_read(_opb,2,&val);
  _info->pixel_fmt=(theora_pixel_fmt)val;
  if(theora_read(_opb,3,&val)<0||val!=0)return OC_BADHEADER;
  return 0;
}

static int oc_comment_unpack(oggpack_buffer *_opb,theora_comment *_tc){
  long len;
  /*Read the vendor string.*/
  len=oc_unpack_length(_opb);
  if(len<0)return OC_BADHEADER;
  _tc->vendor=_ogg_malloc((size_t)len+1);
  oc_unpack_octets(_opb,_tc->vendor,len);
  _tc->vendor[len]='\0';
  /*Read the user comments.*/
  _tc->comments=oggpackB_read(_opb,32);
  if(_tc->comments>=0){
    int i;
    for(i=0;i<_tc->comments;i++){
      len=oc_unpack_length(_opb);
      if(len<0)return OC_BADHEADER;
      _tc->comment_lengths[i]=len;
      _tc->user_comments[i]=_ogg_malloc((size_t)len+1);
      oc_unpack_octets(_opb,_tc->user_comments[i],len);
      _tc->user_comments[i][len]='\0';
    }
  }
  return oggpackB_read(_opb,0)<0?OC_BADHEADER:0;
}

static int oc_setup_unpack(oggpack_buffer *_opb,theora_setup_info *_setup){
  /*Read the quantizer tables.*/
  oc_quant_params_unpack(_opb,&_setup->qinfo);
  /*Read the Huffman trees.*/
  return oc_huff_trees_unpack(_opb,_setup->huff_tables);
}

static void oc_setup_clear(theora_setup_info *_setup){
  oc_quant_params_clear(&_setup->qinfo);
  oc_huff_trees_clear(_setup->huff_tables);
}

static int oc_dec_headerin(oggpack_buffer *_opb,theora_info *_info,
 theora_comment *_tc,theora_setup_info **_setup,ogg_packet *_op){
  char buffer[6];
  int  packtype;
  int  ret;
  /*Check to make sure this is the info header.*/
  packtype=(int)oggpackB_read(_opb,8);
  /*Check the codec string.*/
  oc_unpack_octets(_opb,buffer,6);
  if(memcmp(buffer,"theora",6)!=0)return OC_NOTFORMAT;
  switch(packtype){
    /*Codec info header.*/
    case 0x80:{
      /*This should be the first packet, and we should not already be
         initialized.*/
      if(!_op->b_o_s||_info->width>0)return OC_BADHEADER;
      ret=oc_info_unpack(_opb,_info);
      if(ret<0)theora_info_clear(_info);
      else ret=2;
    }break;
    /*Comment header.*/
    case 0x81:{
      if(_tc==NULL)return OC_FAULT;
      /*We shoud have already decoded the info header, and should not yet have
         decoded the comment header.*/
      if(_info->width<=0||_tc->vendor!=NULL)return OC_BADHEADER;
      ret=oc_comment_unpack(_opb,_tc);
      if(ret<0)theora_comment_clear(_tc);
      else ret=1;
    }break;
    /*Codec setup header.*/
    case 0x82:{
      oc_setup_info *setup;
      if(_tc==NULL||_setup==NULL)return OC_FAULT;
      /*We should have already decoded the info header and the comment header,
         and should not yet have decoded the setup header.*/
      if(_info->width<=0||_tc->vendor==NULL||*_setup!=NULL){
        return OC_BADHEADER;
      }
      setup=(oc_setup_info *)_ogg_calloc(1,sizeof(*setup));
      ret=oc_setup_unpack(_opb,setup);
      if(ret<0){
        oc_setup_clear(setup);
        _ogg_free(setup);
      }
      else{
        *_setup=setup;
        ret=0;
      }
    }break;
    /*No more headers to read.*/
    default:ret=0;break;
  }
  return ret;
}


/*Decodes one header packet.
  This should be called repeatedly with the packets at the beginning of the
   stream until it returns 0.*/
int theora_decode_headerin(theora_info *_info,theora_comment *_tc,
 theora_setup_info **_setup,ogg_packet *_op){
  oggpack_buffer opb;
  int            ret;
  if(_op==NULL)return OC_BADHEADER;
  if(_info==NULL)return OC_FAULT;
  oggpackB_readinit(&opb,_op->packet,_op->bytes);
  ret=oc_dec_headerin(&opb,_info,_tc,_setup,_op);
  /*TODO: Clear opb in libogg2.*/
  return ret;
}

void theora_setup_free(theora_setup_info *_setup){
  if(_setup!=NULL){
    oc_setup_clear(_setup);
    _ogg_free(_setup);
  }
}