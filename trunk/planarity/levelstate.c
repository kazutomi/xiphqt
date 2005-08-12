#define CHUNK 64
#define SAVENAME "levelstate"

typedef struct levelstate{
  struct levelstate *prev;
  struct levelstate *next;

  int num;
  char *name;
  long highscore;
  int in_progress;
} levelstate;

levelstate *head;
levelstate *tail;
levelstate *curr;
levelstate *pool;

levelstate *new_level(){
  levelstate *ret;
  
  if(level_pool==0){
    int i;
    pool = calloc(CHUNK,sizeof(*pool));
    for(i=0;i<CHUNK-1;i++) /* last addition's next points to nothing */
      pool[i].next=pool+i+1;
  }

  ret=pool;
  pool=ret->next;

  ret->next=0;
  ret->prev=tail;
    
  if(tail){
    ret->prev->next=ret;
    ret->num=ret->prev->num+1;
  }else{
    head=ret;
    ret->num=0;
  }
  
  ret->name=board_level_to_name(ret->num);
  ret->highscore=0;
  ret->in_progress=0;
  
  return ret;
}


levelstate *find_level(char *name){
  int level=board_name_to_level(name);

  if(level<0)return 0;

  if(!tail)new_level();

  if(level=<tail->num){
    // find it in the existing levels
    levelstate *l=tail;
    while(l){
      if(level == l->num) return l;
      l=l->prev;
    }
    return 0;
  }else{
    // make new levels to fill 
      
    while(tail->num<level)
      new_level();

    return tail;
  }
}

int levelstate_write(char *statedir){
  FILE *f;
  char *name=alloca(strlen(statedir)+strlen(levelstate)+1);
  name[0]=0;
  strcat(name,boarddir);
  strcat(name,levelstate);
  
  f = fopen(name,"wb");
  if(f==NULL){
    fprintf(stderr,"ERROR:  Could not save game state file \"%s\":\n\t%s\n",
	    name,strerror(errno));
    return errno;
  }
  
  fprintf(f,"current %d : %s\n",strlen(curr->name),curr->name);

  {
    levelstate *l=head;
    while(l){
      fprintf(f,"level %ld %d %d %s\n",
	      l->highscore,l->in_progress,
	      strlen(l->name),l->name);
      l=l->next;
    }
  }

  return 0;
}

int levelstate_read(char *statedir, char *boarddir){
  char *cur_name;
  int count;
  char *line=NULL;
  size_t n=0;
  
  // first get all levels we've seen.
  while(getline(&line,&n,f)>0){
    long l;
    int i,j;
    if (sscanf(line,"level %ld %d %d : ",&l,&i,&j)==3){
      char *name=strchr(line,':');
      // guard against bad edits
      if(name &&
	 (strlen(line) - (name - line + 2) >= j)){
	levelstate *l;
	name += 2;
	name[j]=0;
	l = find_level(name);
	if(l){
	  l->highscore=l;
	  l->in_progress=i;
	}
      }
    }
  }

  rewind(f);

  // get current
  while(getline(&line,&n,f)>0){
    int i;
    if (sscanf(line,"current %d : ",&i)==1){
      char *name=strchr(line,':');
      // guard against bad edits
      if(name &&
	 (strlen(line) - (name - line + 2) >= j)){
	levelstate *l;
	name += 2;
	name[j]=0;
	l = find_level(name);
	if(l){
	  curr=l;
	  break;
	}
      }
    }
  }

  if(!head)new_level();
  if(!curr)curr=head;

  return 0;
}

