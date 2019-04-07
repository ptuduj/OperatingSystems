#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>


#define false 0
#define true 1


#define SHOW_CREATED 1
#define SHOW_ASKED 1
#define SHOW_SEND 1
#define SHOW_GOT_RT 1
#define SHOW_DEATH 1


struct Child{

 int PID;
 char ifEnded;
};


char ifAccepted = false;
int  childfIndex=0;
int asked =0;
int childrenNum = 5;
int requestNum = 3;   // max number of requests
int motherPID;
struct Child* childArray;
int * childArrayCalled;
int childArrayCalledId=0;


void handleSIGUSR1(int sig,siginfo_t *siginfo, void* context) {

    asked++;
  if (getpid() == motherPID) {
    #ifdef SHOW_ASKED

        printf("Poproszono o dostep przed proces %d\n",siginfo->si_pid);
    #endif //SHOW_ASKED

    if (asked >= requestNum){
    #ifdef SHOW_SEND
        printf("Wyslano pozwolenie do procesu: %d\n",siginfo->si_pid);
    #endif // SHOW_SEND
        kill(siginfo->si_pid, SIGUSR1);   // grant permission
    }
    else
    {
        childArrayCalled[childArrayCalledId]=siginfo->si_pid;
        childArrayCalledId++;
    }

    childfIndex++;
  }
  else {    // youu're a child so you've just received permission

     ifAccepted= true;
  }

}

void handleSIGINT() {

  if (getpid() == motherPID) {

  for (int i=0; i<childrenNum; i++) {

        kill(childArray[i].PID, SIGKILL);
  }

     exit(-1);
  }

  else {     // you're a child
    ;
  }

}

void handleSIGRT(int sig,siginfo_t *siginfo, void* context)
{
    if (getpid() == motherPID){
        int pid= siginfo->si_pid;
        #ifdef SHOW_GOT_RT
            printf("Wysalono sygnal SIGRTMIN+%d przez proces: %d\n",sig-SIGRTMIN,pid);
        #endif // SHOW_GOT_RT
        for(int i=0;i<childrenNum;i++){
            if(childArray[i].PID==pid){
                childArray[i].ifEnded=true;
                break;
            }
        }

    }
}



void childLife() {

    int secondsToSleep = rand()%11;
    sleep(secondsToSleep);
    kill(getppid(),SIGUSR1);
    while(true){
        if (ifAccepted) {
            int RTsignal = SIGRTMIN + rand()%32;
            kill(getppid(), RTsignal);
            exit (secondsToSleep);
        }
    }
    exit(-1);
}


void createChildren(){

  for(int i=0; i<childrenNum; i++) {   // create n processes

        srand(rand());
        int PID=fork();

        if(PID !=0){
            #ifdef SHOW_CREATED
            printf("Stworzono dziecko o PID:%d DBG i=%d, getpid()=%d\n",PID,i,getpid());
            #endif // SHOW_CREATED
            childArray[i].PID = PID;// MOVED FROM if(PID ==0)
            childArray[i].ifEnded=false;

        }

        if( PID == 0 ) {

            childLife();
        }
    }
    //raise(SIGINT);
}

void checkArguments(int args, char * argv[]) {

    if(args == 3) {                                 // check the correctness of arguments
        childrenNum = atoi(argv[1]);
        requestNum = atoi(argv[2]);
    }

    if(requestNum > childrenNum || requestNum<0) {
        printf("Nieprawidłowa liczba próśb! Wykonuję z wartościami domyślnymi.\n");
    }

}



int main(int args, char * argv[])
{

    checkArguments(args, argv);

    motherPID= getpid();
    childArray = malloc(childrenNum * sizeof( struct Child));
    childArrayCalled =  malloc(requestNum * sizeof(int));
    for(int i=0;i<requestNum;i++){
        childArrayCalled[i]=0;
    }

    for (int i=0; i<childrenNum; i++){
        childArray[i].ifEnded = false;
        childArray[i].PID = -1;
    }

    // structs for handling signals

    struct sigaction act1;
    act1.sa_flags = SA_SIGINFO;
    sigemptyset(&act1.sa_mask);
    act1.sa_sigaction = handleSIGINT;
    sigaction(SIGINT,&act1,NULL);

    struct sigaction act2;
    act2.sa_flags = SA_SIGINFO;
    sigemptyset(&act2.sa_mask);
    act2.sa_sigaction = handleSIGUSR1;
    sigaction(SIGUSR1,&act2,NULL);

    struct sigaction act3;
    act3.sa_flags=SA_SIGINFO;
    sigemptyset(&act3.sa_mask);
    act3.sa_sigaction = handleSIGRT;

    for(int i= SIGRTMIN; i<=SIGRTMAX; i++) sigaction(i, &act3, NULL);


    createChildren();


    int childrenLivingNum = childrenNum;


    while(asked < requestNum) pause();


    for(int i=0;i <(requestNum-1); i++) {                 // grant permissions to every process, beacuse k==n, and -1 cause requestNum'bers procces has been alredy asnwered
    #ifdef SHOW_SEND
        printf("Wyslano pozowlenie do procesu: %d\n",(*(childArrayCalled+i)));
    #endif // SHOW_SEND
        if((*(childArrayCalled+i))<=0){
           printf("(*(childArrayCalled+i)<=0; retreat!, (*(childArrayCalled+i)=%d; i=%d \n",(*(childArrayCalled+i)),i);
           raise(SIGINT);
        }
        else
            kill((*(childArrayCalled+i)), SIGUSR1);
    }



    int status;

    while(childrenLivingNum>0){
        for(int i=0;i<childrenNum;i++){
            if(childArray[i].ifEnded==true){
                waitpid(childArray[i].PID,&status,0);
                childArray[i].ifEnded=false;
                childrenLivingNum--;
                #ifdef SHOW_DEATH
                printf("Proces potomny o ID:%d  zwrocil wartosc %d\n",childArray[i].PID,WEXITSTATUS(status));
                #endif // SHOW_DEATH
            }
        }
    }

    free(childArrayCalled);
    free(childArray);
  return 0;
}
