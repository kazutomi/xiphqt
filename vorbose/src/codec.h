typedef struct vorbis_info vorbis_info;

typedef struct{
  int  order;
  long rate;
  long barkmap;

  int  ampbits;
  int  ampdB;

  int  numbooks; /* <= 16 */
  int  books[16];

} vorbis_info_floor0;

#define VIF_POSIT 63
#define VIF_CLASS 16
#define VIF_PARTS 31

typedef struct{
  int  class_dim;        /* 1 to 8 */
  int class_subs;       /* 0,1,2,3 (bits: 1<<n poss) */
  int class_book;       /* subs ^ dim entries */
  int class_subbook[8]; /* [VIF_CLASS][subs] */
} floor1class;  

typedef struct{
  floor1class  class[VIF_CLASS];         
  ogg_int16_t  partitionclass[VIF_PARTS];
  ogg_uint16_t postlist[VIF_POSIT+2];
  ogg_int16_t  forward_index[VIF_POSIT+2];
  ogg_int16_t  hineighbor[VIF_POSIT];
  ogg_int16_t  loneighbor[VIF_POSIT];

  ogg_int16_t   partitions;     /* 0 to 31 */
  ogg_int16_t   posts;
  ogg_int16_t   mult;           /* 1 2 3 or 4 */ 

} vorbis_info_floor1;

typedef struct vorbis_info_floor{
  int type;
  union {
    vorbis_info_floor0 floor0;
    vorbis_info_floor1 floor1;
  } floor;
} vorbis_info_floor;

typedef struct vorbis_info_residue{
  int type;
  char stagemasks[64];
  char stagebooks[64*8];

/* block-partitioned VQ coded straight residue */
  long begin;
  long end;

  int  grouping; 
  int  partitions;
  int groupbook;  
  int  stages;
} vorbis_info_residue;

typedef struct {
  int blockflag;
  int mapping;
} vorbis_info_mode;

typedef struct coupling_step{
  ogg_uint16_t mag;
  ogg_uint16_t ang;
} coupling_step;

typedef struct submap{
  ogg_int16_t floor;
  ogg_int16_t residue;
} submap;

typedef struct vorbis_info_mapping{
  ogg_int16_t    submaps; 
  
  int           chmuxlist[256];
  submap        submaplist[16];

  int            coupling_steps;
  coupling_step  coupling[256];
} vorbis_info_mapping;

typedef struct codebook{
  int          dim;
  int          entries;
  int          used_entries;
  ogg_int32_t *dec_table;
} codebook;

struct vorbis_info{
  int channels;
  int blocksizes[2];
  
  int modes;
  int maps;
  int floors;
  int residues;
  int books;

  vorbis_info_mode     mode_param[64];
  vorbis_info_mapping  map_param[64];
  vorbis_info_floor    floor_param[64];
  vorbis_info_residue  residue_param[64];
  codebook             book_param[256];
  
};

extern int  vorbis_info_blocksize(vorbis_info *vi,ogg_int16_t zo);
extern int  vorbis_info_headerin(vorbis_info *vi,ogg2_packet *op);
extern int  vorbis_info_clear(vorbis_info *vi);
extern int  vorbis_book_clear(codebook *b);
extern int  vorbis_book_unpack(ogg2pack_buffer *b,codebook *c);
extern long vorbis_book_decode(codebook *book, ogg2pack_buffer *b);


extern int floor_info_unpack(vorbis_info *vi,ogg2pack_buffer *opb,
			     vorbis_info_floor *fi);
extern int res_unpack(vorbis_info_residue *info,
		      vorbis_info *vi,ogg2pack_buffer *opb);

extern int res_inverse(vorbis_info *vi,
		       vorbis_info_residue *info,
		       int *nonzero,int ch,
		       ogg2pack_buffer *opb);

extern int mapping_info_unpack(vorbis_info_mapping *info,vorbis_info *vi,
			       ogg2pack_buffer *opb);

extern int floor0_inverse(vorbis_info *vi,vorbis_info_floor0 *info,
			  ogg2pack_buffer *opb);
extern int floor1_inverse(vorbis_info *vi,vorbis_info_floor1 *info,
			  ogg2pack_buffer *opb);
extern int mapping_inverse(vorbis_info *vi,vorbis_info_mapping *info,
			   ogg2pack_buffer *opb);
extern int vorbis_decode(vorbis_info *vi,ogg2_packet *op);

