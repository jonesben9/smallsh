/*
Program: smallsh
author: Benjamin Jones
Onid: jonesben
class: cs 344
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <dirent.h> 
#include <time.h>
#include <fcntl.h>
#include <signal.h>

int onlyf=0;//variable to control foreground only mode
//this function handles the control+z SIGTSTP signal
void handle_SIGTSTP(int signo) {
	if (onlyf == 0) {
		write(1, "Entering forground only mode\n:", sizeof("Entering forground only mode\n"));
		fflush(stdout);
		onlyf = 1;
	}
	else {
		write(1, "Exiting forground only mode\n:", sizeof("Exiting forground only mode\n"));
		fflush(stdout);
		onlyf = 0;
	}
}


int main() {
	//this sectin sets up signal handling
	struct sigaction SIGTSTP_action = { 0 }, ignore_action = { 0 };
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = SA_RESTART;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);
	//ignore_action.sa_handler = SIG_IGN;
	signal(SIGINT, SIG_IGN);
	// Register the ignore_action as the handler for SIGTERM, SIGHUP, SIGQUIT. So all three of these signals will be ignored.
	//sigaction(SIGINT, &ignore_action, NULL);
	//general variables
	char *input;
	char**arguments;
	int toknum;
	int fStatus;
	int bStatus;
	char* token;
	//variables for file redirection
	int outfile;
	int infile;
	int iflag=0;
	int oflag=0;
	char* ifile;
	char* ofile;
	int bflag;//whether or not background
	//child ids
	pid_t fspawn = 0;
	pid_t bspawn = 0;
	printf("Shell PID: %d\n", getpid());
	fflush(stdout);
	/*
	this while loop is the main part of the shell. each loop allows the user to enter a command.
	*/
	while (1) {
		//reinitialize variables
		toknum = 0;//number of arguments
		iflag = 0;//if input is going to be redirected
		oflag = 0;//if output is going to be redirected
		bflag = 0;//if it will be a background process
		input = calloc(2048, sizeof(char));
		arguments = calloc(512, sizeof(char*));
		ifile = calloc(256, sizeof(char));//input file name
		ofile = calloc(256, sizeof(char));//ouput file name
		/*
		this checks on background children
		*/
		bspawn = waitpid(bspawn, &bStatus, WNOHANG);
		while (bspawn != 0 && bspawn != -1) {
			printf("background process %d terminated. ", bspawn);
			if (WIFEXITED(bStatus)) {
				printf("terminated with status %d\n", WEXITSTATUS(bStatus));
			}
			else if (WIFSIGNALED(bStatus)) {
				printf("terminatedby signal %d\n", WTERMSIG(bStatus));
			}
			fflush(stdout);
			bspawn = waitpid(bspawn, &bStatus, WNOHANG);
		}
		/*
		gets user command
		*/
		printf(":");
		fflush(stdout);
		fgets(input, 2048, stdin);
		if (strcmp(input, "\n") == 0) {//accounts for user only hitting enter
			free(input);
			free(arguments);
			free(ifile);
			free(ofile);
			continue;
		}
		char* ok=input;
		/*
		this while loop breaks the initial input string into an array of words.
		also removes arguments for file redirection
		*/
		while ((token =strtok_r(ok, " ", &ok))){//breaks user input into individual words/command arguments
			if (strcmp(token, ">") == 0) {//filters output redirection
				oflag = 1;
				token = strtok_r(ok, " ", &ok);
				strcpy(ofile, token);
				continue;
			}
			else if (strcmp(token, "<") == 0) {//filters input redirection
				iflag = 1;
				token = strtok_r(ok, " ", &ok);
				strcpy(ifile, token);
				continue;
			}
			arguments[toknum] = token;
			toknum++;
		}
		if (arguments[0][0] == '#') {//filters out comments
			free(input);
			free(arguments);
			free(ifile);
			free(ofile);
			continue;
		}
		//these if/else statements remove the newline character that in input by the user and replaces w/ terminating null character
		if (ifile[strlen(ifile) - 1] == '\n') {
			char* temp1;
			temp1 = strchr(ifile, '\n');
			*temp1 = '\0';
		}
		else if (ofile[strlen(ofile) - 1] == '\n') {
			char* temp2;
			temp2 = strchr(ofile, '\n');
			*temp2 = '\0';
		}
		else {
			char* temp;
			temp = strchr(arguments[toknum - 1], '\n');//figured out how to use strchr from https://stackoverflow.com/questions/28802938/how-to-remove-last-part-of-string-in-c
			*temp = '\0';
		}
		/*
		these for loops go through each argument, and convert $$ to the process id
		*/
		for (int i = 0; i < toknum; i++) {//variable expansion. converts $$ to pid
			for (int j = 0; j < strlen(arguments[i]); j++) {
				if (arguments[i][j] == '$' && arguments[i][j + 1] == '$') {
					char* t1 = calloc(256, sizeof(char));
					char* t2 = calloc(256, sizeof(char));
					int lenght = snprintf(NULL, 0, "%d", getpid());
					char* t3 = malloc(lenght + 1);
					snprintf(t3, lenght + 1, "%d", getpid());
					strcpy(t1, arguments[i]);
					for (int k = j + 2; k < strlen(arguments[i]); k++) {
						t2[k - j - 2] = arguments[i][k];
					}
					t1[j] = '\0';
					strcat(t1, t3);
					strcat(t1, t2);
					//printf("%s\n", t1);
					strcpy(arguments[i], t1);
					free(t3);
					free(t1);
					free(t2);
				}
			}
		}
		/*
		these ints determine if the command is exit cd or status
		*/
		int repeat = strcmp(arguments[0], "exit");
		int cd = strcmp(arguments[0], "cd");
		int status = strcmp(arguments[0], "status");
		if (repeat == 0 || repeat == 10) {//exits when user enters exit
			free(input);
			free(arguments);
			free(ifile);
			free(ofile);
			return 0;
		}
		else if (cd == 0) {//sets directory to specifiewd directory
			if(toknum==1)
				chdir(getenv("HOME"));
			else
				chdir(arguments[1]);
			free(input);
			free(arguments);
			free(ifile);
			free(ofile);
			continue;
		}
		else if (status == 0) {//prints status of last foreground process
			printf("last child that terminated = %d\n", fspawn);
			fflush(stdout);
			if (WIFEXITED(fStatus)) {
				printf("child exited normally with status %d\n", WEXITSTATUS(fStatus));
				fflush(stdout);
			}
			else {
				printf("child had abnormal exit duie to signal %d\n", WTERMSIG(fStatus));
				fflush(stdout);
			}
			free(input);
			free(arguments);
			free(ifile);
			free(ofile);
			continue;
		}
		if (strcmp(arguments[toknum-1], "&")==0) {//determines if background process of not.
			//printf("background\n");
			//fflush(stdout);
			if(onlyf==0)
				bflag = 1;
			arguments[toknum - 1] = NULL;
			toknum = toknum - 1;
		}
		/*
		this if statment forks and runs the specified command, and then waits for it to finish
		*/
		if(bflag==0){//taken from given example
			fspawn = fork();
			switch (fspawn) {
			case -1://error case
				perror("fork()\n");
				free(input);
				free(arguments);
				free(ifile);
				free(ofile);
				exit(1);
				break;
			case 0://child process
				signal(SIGTSTP, SIG_IGN);
				signal(SIGINT, SIG_DFL);
				fflush(stdout);
				if (iflag) {//redirects input if applicable
					infile = open(ifile, O_RDONLY);
					if (dup2(infile, 0) < 0) {
						printf("unable to redirect input\n");
						free(input);
						free(arguments);
						free(ifile);
						free(ofile);
						exit(1);
					}
				}
				if (oflag) {//redirects output if applicable
					outfile = open(ofile, O_CREAT | O_WRONLY| O_TRUNC, 0640);
					if (dup2(outfile, 1) < 0) {
						printf("unable to redirect output\n");
						free(input);
						free(arguments);
						free(ifile);
						free(ofile);
						exit(1);
					}
				}
				execvp(arguments[0], arguments);
				// exec only returns if there is an error
				perror("execvp");
				free(input);
				free(arguments);
				free(ifile);
				free(ofile);
				exit(2);
				break;
			default://parent process
				fspawn = waitpid(fspawn, &fStatus, 0);
				if (WIFSIGNALED(fStatus)) {//outputs if foreground process was killed by a signal
					printf("child(%d) was terminated by signal %d.\n", fspawn, WTERMSIG(fStatus));
					fflush(stdout);
				}
				free(input);
				free(arguments);
				free(ifile);
				free(ofile);
				continue;
			}
		}
		/*
		this statement runs processes in the background does not wait for completion
		*/
		else if (bflag == 1) {
			bspawn = fork();
			switch (bspawn) {
			case -1://error
				perror("fork()\n");
				free(input);
				free(arguments);
				free(ifile);
				free(ofile);
				exit(1);
				break;
			case 0://child
				signal(SIGTSTP, SIG_IGN);
				if (iflag) {//redirects input if specified
					infile = open(ifile, O_RDONLY);
					if (dup2(infile, 0) < 0) {
						printf("unable to redirect input\n");
						free(input);
						free(arguments);
						free(ifile);
						free(ofile);
						exit(1);
					}
				}
				if (oflag) {//redirects output if specified
					outfile = open(ofile, O_CREAT | O_WRONLY | O_TRUNC, 0640);
					if (dup2(outfile, 1) < 0) {
						printf("unable to redirect output\n");
						free(input);
						free(arguments);
						free(ifile);
						free(ofile);
						exit(1);
					}
				}
				printf("Background process %d\n", getpid());
				fflush(stdout);
				execvp(arguments[0], arguments);
				// exec only returns if there is an error
				perror("execvp");
				free(input);
				free(arguments);
				free(ifile);
				free(ofile);
				exit(2);
				break;
			default://parent
				bspawn = waitpid(bspawn, &bStatus,WNOHANG);
				free(input);
				free(arguments);
				free(ifile);
				free(ofile);
				continue;
			}
		}
		free(input);
		free(arguments);
		free(ifile);
		free(ofile);
	}
	return 0;
}