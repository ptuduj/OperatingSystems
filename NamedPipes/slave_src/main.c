#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#define true 1
#define false 0


char slaveLife(char* path, int n) {

    srand(time(NULL));
    int PID = getpid();
    printf("Slave PID: %d\n",PID);


    int fd = open(path,O_WRONLY);

    if(fd<0) {
        printf("Error while opening file!");
        return false;
    }

    FILE* fHandler;
    char buff[100];
    char outBuff[100];

    int sleepTime;

    for(int i=0;i<n;i++) {

        fHandler = popen("date","r");
        if(fHandler == NULL) return false;

        fread(buff,1,50, fHandler);

        for(int i=0;i<100;i++) {
            if(buff[i]=='\n')  buff[i]='\0';
        }

        sprintf(outBuff,"PID:%d    %s\n",PID,buff);
        printf("%s",outBuff);
        int tmp = write(fd, outBuff, strlen(outBuff)+1);       // write to pipe

        printf("Just written %d characters\n",tmp);

        sleepTime=(rand()%4+2);
        sleep(sleepTime);

        fclose(fHandler);
    }

    close(fd);
    return true;
}

int main(int args,char* argv[]){

    int n=10;
    char pathBuff[]="/home/paulinat/Desktop/zestaw5/PaulinaTuduj/zad2/testPipe";

    char* path=pathBuff;

    if(args == 3){
        path = argv[1];
        n = atoi(argv[2]);
    }

    if(!slaveLife(path,n)) printf("Fatal error occured!\n");

    return 0;
}
