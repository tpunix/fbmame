
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <pthread.h>
#include <sys/wait.h>
#include <sys/time.h>

#include "mmaster.h"

/******************************************************************************/


static u64 start_sec = 0;

u64 osd_ticks(void)
{
	struct timeval    tp;

	gettimeofday(&tp, NULL);
	if (start_sec==0)
		start_sec = tp.tv_sec;

	return (tp.tv_sec - start_sec) * (u64) 1000000 + tp.tv_usec;
}


/******************************************************************************/


int main (int argc, char *argv[])
{
	int pid, status;
	char *new_argv[16];
	int i, retv, np = 0;

	input_event_init();
    fb_init();

	pid = fork();
	if(pid==0){
		new_argv[np++] = (char*)"minimametiny64";
		new_argv[np++] = (char*)"-verbose";
		new_argv[np++] = (char*)"-slave";
		for(i=1; i<argc; i++){
			new_argv[np++] = argv[i];
		}
		new_argv[np++] = NULL;
		execvp("./minimametiny64", new_argv);
	}else{
		while(1){
			retv = waitpid(pid, &status, 0);
			if(retv<0){
				perror("waitpid");
				break;
			}
			if(WIFEXITED(status)){
				printf("child exit: status=%d\n", WEXITSTATUS(status));
				break;
			}else if(WIFSIGNALED(status)){
				printf("child killed by signal %d\n", WTERMSIG(status));
				break;
			}else if (WIFSTOPPED(status)){
				printf("stopped by signal %d\n", WSTOPSIG(status));
			}else if (WIFCONTINUED(status)){
				printf("continued\n");
			}
		}
	}


	fb_exit();
	input_event_exit();

    return 0;
}

