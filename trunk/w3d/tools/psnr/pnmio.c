#include <stdio.h>
#include <ctype.h>

int
eat_ws_comments(FILE *fp)
{
	int c;
	
	while( (c = fgetc(fp)) != EOF) {
		if( c == '#') {/* eat the comment */
			do {
				c = fgetc(fp);
				if( c == EOF) /* but bail if we hit EOF */
					return EOF;
			} while ( c != '\n');
		}
		
		if (!isspace(c)) /*we are done*/
			return ungetc(c,fp);
	}
	
	return EOF;  /* fell out of the loop*/
}


/* this is a kludge, but raw pnm's are not well defined... */
int
eat_ws_to_eol(FILE *fp)
{
	int c;
	do {
		c = fgetc(fp);
		if( c == '\n' || c == '\r') /* some dos/win pnms generated with just '\r' after maxval */
			return c;
	} while(isspace(c));

	return ungetc(c,fp);
}

int
write_pnm_header(FILE *fp, size_t type, size_t w, size_t h, size_t maxval, char *comment)
{
	if(comment)	   
		return fprintf(fp,"P%c\n%s\n%lu %lu\n%lu\n",type,comment,w,h,maxval);
	else
		return fprintf(fp,"P%c\n%lu %lu\n%lu\n",type,w,h,maxval);
}

int
read_pnm_header(FILE *fp, size_t *type, size_t *w, size_t *h, size_t *maxval)
{
	if(fgetc(fp) != 'P')
		return 0;
	*type = fgetc(fp);
	switch(*type){
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
		break;
	default: /*invalid type if not above*/
		return 0; 
	}

	eat_ws_comments(fp);
	fscanf(fp,"%d",w);
	eat_ws_comments(fp);
	fscanf(fp,"%d",h);
	eat_ws_comments(fp);
	fscanf(fp,"%d",maxval);	
	eat_ws_to_eol(fp);	

	return 1; /* valid file found */
}

int
read_ppm(FILE *fp, char *buf,size_t *w, size_t *h, size_t *m)
{
	size_t type,width,height,maxval;

	if (!read_pnm_header(fp,&width,&height,&maxval,&type) || type != '6')
		return 0;

	if ( fread(buf,3,width*height,fp) != width*height)
		return 0;

	*w=width; *h=height; *m=maxval;
	return 1;
}

int
read_pgm(FILE *fp, char *buf,size_t *w, size_t *h, size_t *m)
{
	size_t type,width,height,maxval;

	if (!read_pnm_header(fp,&width,&height,&maxval,&type) || type != '5')
		return 0;

	if ( fread(buf,1,width*height,fp) != width*height)
		return 0;

	*w=width; *h=height; *m=maxval;
	return 1;
}
