#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define true 1
#define false 0

char masterLife(char* path){

    if(!mkfifo(path, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH)) return false;

    char buff[100];

    int fd = open(path,O_RDONLY);

    if (fd < 0) {
        printf("Error while opening FIFO!\n");
        return false;
    }


    int bytesRead;

    while(true) {

        bytesRead = read(fd, buff, 50);
        if(bytesRead>0) {

            printf("%s",buff);
            fflush(stdout);
        }
        else break;
    }
}


int main(int args, char* argv[]) {

    char pathBuff[]="testPipe";
    char* path = pathBuff;

    if (args == 2) path=argv[1];

    if(masterLife(path));

    return 0;
}
