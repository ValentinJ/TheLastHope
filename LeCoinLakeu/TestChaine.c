#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <sys/msg.h>

#define CLE 123

typedef struct {
        long  type;
        pid_t numPID;
        } tMessage;

int main(){

	pid_t pid1, pid2;
	int msg;


}
