#ifndef SYSTEMV_H_INCLUDED
#define SYSTEMV_H_INCLUDED

#define MAX_MSG_LEN 100
#define PROJECT 'V'

typedef struct{
    long mtype;
    char mtext[MAX_MSG_LEN];
} my_msg;


typedef struct{
    int PID;
    int queueID;
} Client;


typedef enum mtype {
    START = 1,
    MIRROR = 2,
    CALC = 3,
    TIME = 4,
    END = 5,          // end server
    STOP = 6
} mtype;

#endif // SYSTEMV_H_INCLUDED
