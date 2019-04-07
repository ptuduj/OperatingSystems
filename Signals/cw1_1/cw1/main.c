#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <signal.h>

#define true 1
#define false 0


char SIGTSTOPreceived = false;


void displayTime()
{
    time_t crTime;
    time(&crTime);
    struct tm *tmTime;
    char out[100];
    setlocale (LC_ALL, "pl_PL");
    tmTime=localtime(&crTime);
    strftime(out,99,"%Y-%m-%d %H:%M:%S",localtime(&crTime));
    printf("%s\n",out);
}


void handleSIGINT()
{
    printf("\notrzymano sygnal SIGINT\n");
    exit(1);
}

void handleSIGTSTP()
{

    if(!SIGTSTOPreceived)
    {
        SIGTSTOPreceived=true;
        printf("Oczekuję na CTRL+Z - kontynuacja albo CTR+C - zakonczenie programu\n");
        return;
    }

    SIGTSTOPreceived=false;
    printf("\n");
}



int main() {

    struct sigaction act;
    act.sa_handler=handleSIGTSTP;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGTSTP,&act,NULL);
    signal(SIGINT,handleSIGINT);

    while(1)
    {
        if (SIGTSTOPreceived) {
        pause();

        }
        else displayTime();
        sleep(1);
    }

    return 0;
}
