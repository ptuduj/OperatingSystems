#ifndef SYSTEMV_H_INCLUDED
#define SYSTEMV_H_INCLUDED

#define MAX_MSG_LEN 100
#define MAX_SEND_LEN 90
#define MAIN_QUE_NAME "/QueTest000000"
#define MAX_MSG_COUNT 10

typedef struct{
    long mtype;
    char mtext[MAX_MSG_LEN];
} my_msg;


typedef struct{
    int PID;
    int q_desc;
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
