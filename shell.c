/*
 * Copyright (C) 2002, Simon Nieuviarts
 */

#include <stdio.h>
#include <stdlib.h>
#include "readcmd.h"
#include "csapp.h"
/*
void handler(int sig) //SIGINT handler
{
    pid_t pid;
    int status;
    while (1)
    {
        pid = waitpid(-1, &status, WNOHANG|WUNTRACED);
        if(pid == -1) break;
    }
}
*/
int main()
{
	//Signal(SIGCHLD, handler);

	while (1) {
		int status;
		pid_t pid;

		struct cmdline *l;
		int i, j;

		printf("shell> ");
		l = readcmd();

		/* If input stream closed, normal termination */
		if (!l) {
			printf("exit\n");
			exit(0);
		}

		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		if(l->isBg) 
		{
			printf("background command\n");
		}

		/*
		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);
		*/

		/* Display each command of the pipe */
		
		for (i=0; l->seq[i]!=0; i++) {
			char **cmd = l->seq[i];
			printf("seq[%d]: ", i);
			for (j=0; cmd[j]!=0; j++) {
				printf("%s ", cmd[j]);
			}
			printf("\n");
		}
		

		int tube[100][2]; // multitube
		for ( i = 0; l->seq[i+1]!=0; i++)
		{
			int checkTube = pipe(tube[i]);
			if(checkTube)
			{
				perror("pipe error");
				exit(1);
			}
		}
		

		for ( i = 0; l->seq[i]!=0; i++)
		{
			char **cmd = l->seq[i];

			if(strcmp(cmd[0], "quit") == 0) // quit
			{
				printf("exit\n");
				exit(0);
			}

			if ((pid = Fork()) == 0)
			{
				int fd_in, fd_out;

				if(i!=0) //not the first
				{
					Dup2(tube[i-1][0], 0);
					Close(tube[i-1][1]);

					for (j = 0; j < i-1; j++) //close unused tube before
					{
						Close(tube[j][0]);
						Close(tube[j][1]);
					}
				}
				else 
				{
					if(l->in){ // maybe read from file
						fd_in = Open(l->in, O_CREAT|O_RDONLY|O_TRUNC,S_IRUSR|S_IWUSR);
						Dup2(fd_in, 0);
					}
				}

				if(l->seq[i+1] != 0) //not the last, write to tube
				{
					Dup2(tube[i][1], 1);
					Close(tube[i][0]);

					for (j = i+1; l->seq[j+1]!=0; j++) //close unused tube after
					{
						Close(tube[j][0]);
						Close(tube[j][1]);
					}
				}
				else
				{
					if(l->out){ //maybe write to file
						fd_out = open(l->out, O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR);
						if(fd_out == -1)
						{
							printf("%s: Permission denied\n", l->out);
							return 0;
						}
						else
						{
							Dup2(fd_out, 1);
						}
					}
				}

				int exe = execvp(cmd[0],cmd); //execute command
				if(exe == -1){
					printf("%s: command not found\n",cmd[0]);
				}

				if(l->in) Close(fd_in);
				if(l->out) Close(fd_out);

				exit(0);
			}
			else
			{
				
			}
		}
		
		for ( i = 0; l->seq[i+1]!=0; i++)
		{
			Close(tube[i][0]);
    		Close(tube[i][1]);
		}
		
		if(l->isBg == 0)
		{
			while (1)
			{
				pid = waitpid(-1, &status, WNOHANG|WUNTRACED);
				if(pid == -1) break;
			}
		}
	}
}

// ./sdriver.pl -t tests/* -s ./shell