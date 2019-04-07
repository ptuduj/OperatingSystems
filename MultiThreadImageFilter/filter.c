#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main(int args, char** argv) {

    if(args == 2){
    srand(time(NULL)); // randomize seed
      int c = atoi(argv[1]);
     //   int c = 30;
        double* filter = malloc(c*c*sizeof(double));
        double sum = 0.0;

        for (int i=0; i<c*c; i++){
            filter[i]= (double) (rand()%100);
            sum += filter[i];
        }

        for (int i=0; i<c*c; i++){
            filter[i]/= sum;
        }
        FILE* file = fopen("./filter.pgm", "w");
        if (file == NULL) return -1;


        fprintf(file, "%d", c);
        fprintf(file, "\n");
        for (int i=0; i<c*c; i++) {
            fprintf(file, "%f ", filter[i]);
        }
        fclose(file);

    }


    return 0;
}
