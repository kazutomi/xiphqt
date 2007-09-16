#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	FILE *fp_input, *fp_output;
	int result, dado, count = 0;
	int aux, a;
	if (argc != 3) {
		printf("Uso:\n file_to_mem <input_file> <output_header_file>");
		return -1;
	} else {
		fp_input = fopen(argv[1], "rb");
		
		scanf("%d", &a);
		if (fp_input != NULL) {
			fp_output = fopen(argv[2], "w");
			
			fprintf(fp_output, "static const unsigned int entrada_%d[] = {\n", a);
			
			result = fread (&dado, sizeof(int), 1, fp_input);
			if (!result) {
				fprintf(stderr, "ftell = %ld\n", ftell(fp_input));
				fputs ("Erro de leitura\n", stderr);
				
				exit (3);
			} else {
				count++;
				aux = 0;
				aux = ((dado & 0xFF000000) >> 24) + ((dado & 0x00FF0000) >> 8);
				aux+= ((dado & 0x0000FF00) << 8) + ((dado & 0x000000FF) << 24);
				fprintf(fp_output, "%d", aux);
			}	
			
			while ((result = fread (&dado, sizeof(int), 1, fp_input)) > 0) {
				count++;
				aux = 0;
				aux = ((dado & 0xFF000000) >> 24) + ((dado & 0x00FF0000) >> 8);
				aux+= ((dado & 0x0000FF00) << 8) + ((dado & 0x000000FF) << 24);
				fprintf(fp_output, ",\n%d", aux);
			}
			if (!feof(fp_input)) {
				fputs ("Erro de leitura\n", stderr); 
				exit (3);
			} else {
				fprintf(fp_output, "};\n");
				fprintf(fp_output, "\n\n#define ENTRADA_SIZE_%d %d\n", a, count);
			}
			fclose(fp_input);
			fclose(fp_output);
		}
		return 0;
	}
}
