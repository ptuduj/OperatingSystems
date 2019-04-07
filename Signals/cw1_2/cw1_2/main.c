#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <signal.h>

#define true 1
#define false 0


char SIGTSTOPreceived=false;
int PID = -1;


void displayTime()
{
    PID=fork();

    if(PID==0)      // child's thread
    {
        execl("./script",NULL);
        exit(1);
    }
}


void handleSIGINT()
{
    printf("\nOtrzymano sygnal SIGINT\n");


    if(PID!=-1)  kill(PID,SIGKILL);
    exit(1);
}

void handleSIGTSTOP()
{

    if(!SIGTSTOPreceived)
    {
        SIGTSTOPreceived = true;
        printf("\nOczekuję na CTRL+Z - kontynuacja albo CTR+C - zakonczenie programu\n");
        kill(PID,SIGSTOP);

        return;
    }
    kill(PID,SIGCONT);
    SIGTSTOPreceived = false;

}


int main()
{

    struct sigaction act;
    act.sa_handler=handleSIGTSTOP;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGTSTP,&act,NULL);

    signal(SIGINT,handleSIGINT);

    displayTime();


    while(1)
    {
        sleep(1);
    }

    return 0;
}
