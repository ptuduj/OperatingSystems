#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <pthread.h>


int pthread_num = 2;
char* input_path = "./balloons.ascii.pgm";
int* input_image;
char* out_path = "./out.pgm";
int* out_image;
char* filter_path = "./filter.pgm";
int width;
int height;
int max_grey;
int filter_size;
double* filter_tab;

struct range{
    int start;
    int end;
};
struct range* ranges;


int max(int a, int b) {
    if (a > b) return a;
    return b;
}

int min(int a, int b){
    if (a < b) return a;
    return b;
}


int ceiling(double a) {
    return (int) (a + 1.0);
}

int my_round(double a) {
    return (int) (a + 0.5);
}


int parse_input (const char* input_path) {

    FILE *input_file = fopen(input_path, "r");
    if (input_file == NULL) return -1;
    char buffer[10];

    fscanf(input_file, "%s", buffer);
    fscanf(input_file, "%d", &width);
    fscanf(input_file, "%d", &height);
    fscanf(input_file, "%d", &max_grey);
    printf("%d %d\n", width, height);
    printf("%d\n", max_grey);

    input_image = malloc(width * height * sizeof(int));
    for (int i=0; i < width * height; i++){
        fscanf(input_file, "%d", &input_image[i]);
    }

    fclose(input_file);
    return 0;
}

int parse_filter (const char* filter_path){
    FILE *filter = fopen(filter_path, "r");
    if (filter == NULL) return -1;

    fscanf(filter, "%d", &filter_size);
   //printf("%d\n", filter_size);
    filter_tab = malloc (filter_size * filter_size * sizeof(double));
    for (int i=0; i< filter_size * filter_size; i++) {
        fscanf(filter, "%lf", &filter_tab[i]);
       // printf("%lf ", filter_tab[i]);
    }
    fclose(filter);
    return 0;
}

void alloc_out_tab(int width, int height){

    out_image = malloc(width * height * sizeof(int));
}


void write_to_output (const char* out_path) {

    FILE *out_file = fopen(out_path, "w");

    fprintf(out_file, "P2\n%d %d\n%d\n", width, height, max_grey);

    for (int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++)
            fprintf(out_file, "%d ", out_image[i*width + j]);
        fprintf(out_file, "\n");
    }

    fclose(out_file);
}


void calculate_pixel(int row, int column) {

    double result = 0.0;
    int tmp = ceiling(filter_size / 2.0 );

    for (int k=0; k< filter_size * filter_size; k++){
        int i = k/filter_size;
        int j = k % filter_size;

        int tmp_row = min(height - 1, max(0, row - tmp + i));
        int tmp_column = min(width - 1, max(0, column - tmp + j));
        result += ((double) input_image[tmp_row * width + tmp_column]) * filter_tab[i * filter_size + j];
    }

    out_image[row * width + column] = min(max_grey, max(0, my_round(result)));
}




void split_image (void) {

    int range_size = width / pthread_num;
    int excess = width % pthread_num;
    int range_start = 0;

    for(int i = 0; i < pthread_num; i++) {

        int range_end = range_start + range_size;
        if(excess > 0) {
            range_end++;
            excess--;
        }

        ranges[i].start = range_start;
        ranges[i].end = range_end;
     //   printf("range_start: %d  range_end: %d\n", range_start, range_end);
        range_start = range_end;

    }
}

void* thread_process (void *arg) {

    struct range *range = (struct range*) arg;

    for (int i=0; i< height; i++){
        for (int j = range->start; j<range->end; j++) {
            calculate_pixel(i, j);
        }
    }
    return NULL;
}

double subtract_time(struct timeval a, struct timeval b)
{
    double tmp_a = ((double) a.tv_sec)  + (((double) a.tv_usec) / 1000000);
    double tmp_b = ((double) b.tv_sec)  + (((double) b.tv_usec) / 1000000);
    return tmp_a - tmp_b;
}


int main(int argc, char **argv) {

    if (argc == 5){
        pthread_num = atoi(argv[1]);
        input_path = argv[2];
        filter_path = argv[3];
        out_path = argv[4];
    }

    // save start time
    struct rusage ru_start, ru_end;
    struct timeval sys_start, sys_end, user_start, user_end, real_start, real_end;

    struct timespec real_start_ts, real_end_ts;
    clock_gettime(CLOCK_REALTIME, &real_start_ts);
    getrusage(RUSAGE_SELF, &ru_start);


    parse_input(input_path);
    alloc_out_tab(width, height);
    parse_filter(filter_path);

    ranges = malloc(pthread_num * sizeof(struct range));


    split_image();

    pthread_t threads[pthread_num];

    for(int i = 0; i < pthread_num; i++)
        pthread_create(threads + i, NULL, thread_process, ranges + i);

    for(int i = 0; i < pthread_num; i++)
        pthread_join (threads[i], NULL);

    // Saves end time
    clock_gettime(CLOCK_REALTIME, &real_end_ts);
    getrusage(RUSAGE_SELF, &ru_end);

    real_start.tv_sec = real_start_ts.tv_sec;
    real_start.tv_usec = real_start_ts.tv_nsec / 1000;
    real_end.tv_sec = real_end_ts.tv_sec;
    real_end.tv_usec = real_end_ts.tv_nsec / 1000;

    sys_start = ru_start.ru_stime;
    user_start = ru_start.ru_utime;
    sys_end = ru_end.ru_stime;
    user_end = ru_end.ru_utime;

    printf("image filtering time by %d pthreads:\n", pthread_num);
    printf("real time:\t");
    printf("%lf s\n", subtract_time(real_end, real_start));

    printf("user time:\t");
    printf("%lf s\n", subtract_time(user_end, user_start));

    printf("sys time:\t");
    printf("%lf s\n", subtract_time(sys_end, sys_start));


    write_to_output(out_path);
    return 0;
}
