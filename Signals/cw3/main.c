#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

#define true 1
#define false 0

int L = 10;      // L number of USR1 signals
int mode = 3;
int PID = -1;
char confirmed = true;

int signalsSend = 0;
int sigReceivedByParent = 0;
int sigReceivedByChild = 0;
char SIGUSR2received = false;


void handleSIGUSR1(){

  if (PID == 0) {            // send to the parent

    printf("child signal recieved\n");
    sigReceivedByChild++;
    if (mode == 1 || mode == 2) kill(getppid(),SIGUSR1);
    else kill(getppid(),SIGRTMIN);
  }

  else {     // just 've got a signal from child

        sigReceivedByParent++;
        confirmed = true;
  }

}


void handleSIGUSR2(){

    if (PID == 0){                  // child can die now
        SIGUSR2received = true;
        printf("recived signal to die\n");
    }
}


void handleSIGINT() {

     if (PID == 0) return;

     if (PID > 0){        // parent's thread

          kill(PID, SIGKILL);
     }

     exit(-1);
}



void childLife(){

    confirmed = false;
    printf("childLives \n");
    sigset_t set;           // additional signals to block (child's thread)
    sigfillset(&set);

    if(mode == 1 || mode == 2) {

        sigdelset(&set,SIGUSR1);
        sigdelset(&set,SIGUSR2);
    }
    else {

        sigdelset(&set, SIGRTMIN);
        sigdelset(&set, SIGRTMIN+1);
    }

    sigprocmask(SIG_SETMASK,&set,NULL);

    while(!SIGUSR2received) {        // child loop
          ;
    }
    printf("sigReceivedByChild=%d\n",sigReceivedByChild);
    exit(0);

}


void sendSignals(){
    printf("Mother in send signals\n");

   for (int i=0; i<L; i++) {

        if (mode == 1) {
            kill(PID, SIGUSR1);
            signalsSend++;
        }

        else if (mode == 2) {
             if(confirmed){
                kill(PID, SIGUSR1);
                signalsSend++;
                confirmed=false;
             }
             else {
                i--;
             }
        }

        else {
              kill(PID, SIGRTMIN);
              signalsSend++;
        }
    }
    printf("sginal send\n");
}

// arguments L type
int main(int args, char* argv[])
{

    srand(time(NULL));

    if (args == 3) {                    // check arguments
    L = atoi(argv[1]);
    mode = atoi(argv[2]);
    }
    else
    printf("Nieprawidłowa ilość argumnetów! Korzystam z wartości domyślnych.\n");



    struct sigaction act1;    // structs for handling signals (parent case)
	struct sigaction act2;
    struct sigaction act3;

    act1.sa_flags=0;
    sigemptyset(&act1.sa_mask);
    act1.sa_handler = handleSIGUSR1;
    sigaction(SIGUSR1,&act1,NULL);
    sigaction(SIGRTMIN, &act1, NULL);

    act2.sa_flags=0;
    sigemptyset(&act2.sa_mask);
    act2.sa_handler = handleSIGUSR2;
    sigaction(SIGUSR2,&act2,NULL);
    sigaction(SIGRTMIN+1,&act2,NULL);

    act3.sa_flags=0;
    sigemptyset(&act3.sa_mask);
    act3.sa_handler = handleSIGINT;
    sigaction(SIGINT,&act3,NULL);


    PID= fork();

    if (PID == 0)    // child's thread
        childLife();


    if (PID!=0 && PID!= -1){     // parent's thread
        sleep(1);
        sendSignals();
    }

    // all signals were sent, kill child's thread
    if (mode == 1) kill(PID,SIGUSR2);

    if (mode == 2 && confirmed == true) {
        kill(PID,SIGUSR2);
        confirmed = false;
    }

    if(mode == 3) kill(PID,SIGRTMIN+1);
    printf("Mothere kill send\n");

    int status;
    wait (&status);     // wait for child to be killed

    printf("Sygnaly wysłane przez rodzica: %d\n", signalsSend);
    printf("Sygnały odebrane przez rodzica: %d\n", sigReceivedByParent);

}