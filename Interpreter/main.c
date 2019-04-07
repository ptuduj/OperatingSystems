#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#define true 1
#define false 0


#define PATH_LENGHTS 200
#define MAX_PIPES 10
#define MAX_ARGS 10
#define MAX_LINES 15


char interpret(char* filePath)
{
    FILE* file=fopen(filePath,"r");
    if(file==NULL) return false;

    char chBuff[MAX_LINES*PATH_LENGHTS];
    long long int iCharCount;
    long long int iChCr=0;
    iCharCount=fread(chBuff,1,MAX_LINES*PATH_LENGHTS-2,file);
    char* cLine;
    cLine=chBuff;
    char* nLine;
    nLine=cLine;
    int iCrPipes=0;
    int iPipesTabNumbers[MAX_PIPES];
    char* sArgTab[MAX_PIPES*(MAX_ARGS+2)]; 
    int iBytesRead;
    int status=0;

    int pID;
    int i;
    int iArgTabId;


    char fEnd=false;
    int pIDs[MAX_PIPES+1];
    while(!fEnd)
    {
        cLine=nLine;

        if(cLine[0]=='\0') return true;
        iBytesRead=0;

        for(iBytesRead =0; iBytesRead < PATH_LENGHTS && iChCr<=iCharCount;iBytesRead++)
        {
            iChCr++;
            if(*(cLine+ iBytesRead)=='\n' || *(cLine+ iBytesRead)=='\0' )
            {
                *(cLine+ iBytesRead)='\0';
                nLine=(cLine+iBytesRead +1);
                break;
            }
        }

        if(cLine[0]=='#')
        {
            nLine=cLine+iBytesRead+1;
            continue;// jump to while
        }
        if(iChCr>=iCharCount) fEnd=true;
        if(iBytesRead==PATH_LENGHTS)
        {
            return false;
        }
        if(iBytesRead==0 ||iBytesRead==1 ) continue; // if empty line
        printf("%s\n", cLine);

        for(i=0;i<iBytesRead;i++)
        {
            if(cLine[i]==' ')
                cLine[i]='\0';
        }


        for(i=0;i<MAX_PIPES;i++)
        {
            iPipesTabNumbers[i]=0;
        }

        
        for(i=0;i<MAX_PIPES*(MAX_ARGS+2);i++)
            sArgTab[i]=NULL;
        
        iArgTabId=0;
        i=0;
        char fAnyArgs=false; //   To skip line like "   | ls -l"
        iCrPipes=0;


        // Parsing line --------------------------------------------
        for(int i=0;i<iBytesRead;i++)
        {
            for(;i<iBytesRead && cLine[i]=='\0';i++) // skip all '\0'
                ;
            if(cLine[i]=='|')
            {
                if(!fAnyArgs) return false;
                iCrPipes++;                           // start next pipe
                sArgTab[iArgTabId]=NULL;
                iArgTabId++;
                if(iArgTabId >= (MAX_PIPES*(MAX_ARGS+2) )|| iCrPipes >= MAX_PIPES) return false; // overflow
                iPipesTabNumbers[iCrPipes]=iArgTabId;
                continue;
            }
            if(i==iBytesRead) break;
            sArgTab[iArgTabId]=&cLine[i];
            iArgTabId++;
            fAnyArgs=true;
            if(iArgTabId>=(MAX_PIPES*(MAX_ARGS+2))) return false;

            for(;i<iBytesRead && cLine[i]!='\0';i++);
                ;
        }
        sArgTab[iArgTabId]=NULL; // terminate last Args tab for exec

        #ifdef SHOW
        dbgPrint(sArgTab);
        printf("iCrPipes:%d\n",iCrPipes);
        for(int k =0;k<=iCrPipes;k++)
            printf("iPipesMaxNumbers[%d]:%d\n",k,iPipesTabNumbers[k]);
        #endif //SHOW


        int iCrPipesCpy=iCrPipes;
        if(iCrPipes<=0) iCrPipesCpy=1; // to avoid crashes on table creation
        int pipesFd [iCrPipesCpy][2];// proces 0 uses 0 pipe, process n uses n-1 and n pipe, and final proces(m) uses only m pipe.

        for(i=0;i<iCrPipes;i++) {
            pipe(&pipesFd[i][0]);
        }

        for(i=0;i<(iCrPipes+1);i++ )
        {
            pID=fork();
            if(pID == 0)
                break;
            pIDs[i]=pID;
        }

        int iMyNumber=i;         

        if(pID==0)
        {
            // CHILD
            fclose(file);
            for(i=0;i<iCrPipes;i++)
            {
                if(i!=(iMyNumber) && i!=(iMyNumber-1))        // if i will not use pipes, close them
                {
                    close(pipesFd[i][0]);
                    close(pipesFd[i][1]);
                }

            }
            if(iCrPipes!=0)
            {
                if(iMyNumber==0)
                {
                    close(pipesFd[0][0]);
                    dup2(pipesFd[0][1],STDOUT_FILENO); 
                }
                else    if(iMyNumber==iCrPipes)
                {
                    close(pipesFd[iCrPipes-1][1]);
                    dup2(pipesFd[iCrPipes-1][0],STDIN_FILENO);
                }
                else
                {
                    close(pipesFd[iMyNumber-1][1]);                   // close writing handle, on read only pipe
                    dup2(pipesFd[iMyNumber-1][0],STDIN_FILENO);       // prepare listening pipe
                    close(pipesFd[iMyNumber][0]);                     // close reading handle of write only pipe;
                    dup2(pipesFd[iMyNumber][1],STDOUT_FILENO);        // prepare writing handle
                }
            }

            // CALL EXECS
            if(*sArgTab[iPipesTabNumbers[iMyNumber]]=='.' || *sArgTab[iPipesTabNumbers[iMyNumber]]=='/')
            {
                execv(sArgTab[iPipesTabNumbers[iMyNumber]],&sArgTab[iPipesTabNumbers[iMyNumber]]);
            }
            else
            {
                execvp(sArgTab[iPipesTabNumbers[iMyNumber]],&sArgTab[iPipesTabNumbers[iMyNumber]]);
            }

            exit(-1);
            //ENDCHILD
        }


        // MOTHER
        for(i=0;i<(iCrPipes+ 1 ) && pID!=0; i++)
        {
           // printf("mother waiting for: %d",pIDs[i]);
            waitpid(pIDs,&status,0);
            if(status!=0)
            {
                return false;
            }
        }
      
    }  // end while


    return true;
}



int main(int args, char* argv[])
{
    char* cb1="/home/paulinat/Desktop/zestaw5/lab5/cw1/script";
    char* path=cb1;
    if(args==2)
    {
        path=argv[1];
    }
    if(false==interpret(path))
        printf("\n-----An fatal error has occured-----\n");

    return 0;
}
