#include <stdio.h> 
#include <string.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <syslog.h>
#include <semaphore.h>

int signal_flag = 0;
sem_t sem;

void FLAG_SETTER(int signal);

int ddaemon(char** argv){
    signal(SIGPIPE, FLAG_SETTER);
	signal(SIGUSR1, FLAG_SETTER);
    
    sem_init(&sem, 0, 1); 
	
    while(1) {
		pause();
		
		if(signal_flag==1) {
            sem_wait(&sem);
            int file = open("log.log", O_CREAT|O_RDWR, S_IRWXU);
			lseek(file, 0, SEEK_END);
			write(file, "SIGUSR1, exit\n", strlen("SIGUSR1, exit\n"));
			close(file);
			sem_post(&sem);
			sem_destroy(&sem);
            exit(0);
        }
        
        if(signal_flag == 2) {
            char buffer[512] = "";
            int file_read = open(argv[1], O_CREAT|O_RDWR, S_IRWXU);
            int buffer_size = read(file_read, buffer, sizeof(buffer));
            close(file_read);

            int file = open("log.log", O_CREAT|O_RDWR, S_IRWXU);
			lseek(file, 0, SEEK_END);
			write(file, "SIGPIPE\n", strlen("SIGPIPE\n"));
			close(file);	
            
            int file_write = open("output.log", O_CREAT|O_RDWR, S_IRWXU);
            lseek(file_write, 0, SEEK_END);
            close(1);
            dup2(file_write, 1);
            
            pid_t new_pid;
            int command_number = 0;
            char* string = strtok(buffer, "\n");
            char* command[512];
            
            do {
                *(command + command_number) = string;
                *(*(command + command_number++) + strlen(string)) = '\0';
            } while(string = strtok(NULL, "\n"));
            
            *(command + command_number) = NULL;
            
            for(int i = 0; i < command_number; i++) {
                char* arguments[512];
                int count = 0;
                char* splited = strtok(command[i], " ");
                
                do {
                    *(arguments + count) = splited;
                    *(*(arguments + count++) + strlen(splited)) = '\0';
                } while(splited = strtok(NULL, " "));
                
                *(arguments + count) = NULL;
                
				new_pid = fork();
				
                if(new_pid == 0 ){
                    sem_wait(&sem);	
                    int fd = open("log.log", O_CREAT|O_RDWR, S_IRWXU);
					lseek(fd, 0, SEEK_END);
					write(fd, command[i], strlen(command[i]));
					write(fd, "\n", strlen("\n"));
					close(fd);
                    execv(arguments[0], arguments);
                    sem_post(&sem);
                    sem_destroy(&sem);
                } 
            }
        }

        

    }
}

void FLAG_SETTER(int signal)
{
	switch(signal)
	{
		case SIGUSR1:
			signal_flag = 1;
			break;

		case SIGPIPE:
			signal_flag = 2;
			break;

		default:
			break;
	}
}

int main(int argc, char* argv[])
{
	pid_t pid = fork();
	if (pid < 0) exit(1);
	else if (pid == 0) {
		setsid();
		ddaemon(argv);
	}

	return 0;
}
