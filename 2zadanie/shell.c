#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <limits.h>
#include <pthread.h>
#include "custom_queue.h"

/* THESE ARE THE MAIN TASKS WHICH ARE MANDATORY */
/* WHAT TO IMPLEMENT / TODOs / TASKS
- [x] -help ; -halt ; -quit program arguments
- [ ] help, halt, quit commands in shell
- [ ] at least 4 of these 6 special characters: # ; < > | \ i want to implement at least: # ; \ >
- [ ] option to specify -p port and -u socket as program arguments
- [ ] shell prompt should look like: hh:mm user@hostname$ use system calls and libraries to get time, user and hostname
- [ ] running commands and file redirects must be implemented using syscalls CANNOT USE popen() OR SIMILAR
- [ ] program argument -s is for server which will listen on socket and accept connections from clients and will do all the above work basically
- [ ] program argument -c is for client which will establish connection to server through socket where client will send its stdin and will read the data from server for output to its stdout
- [ ] implement basic error handling for all the above tasks
- [ ] no warning can be shown when compiling even with -Wall
*/

/* VOLUNTARY TASKS*/
/*
- [ ] option to specify -i ip address as program argument
- [ ] option to specify -v verbose output as program argument
- [ ] option to specify -l for log file as program argument
*/

/* NOTES
- need to use mutex for queue and thread pool
- need to use condition variable for queue and thread pool
- need to use signal handling for server and client
- check the return statement of the dequeue to program not get stuck
- use dynamic memory allocation for the output buffer with getline so it can grow as needed
*/

/* GENERAL INFO
- `g_` is used for global variables
- `m_` is used for member variables
*/

#define SERVER_PORT 60069
#define BUFFER_SIZE 1024 // TODO - not sure about the number (make it dynamic because of when user tries to cat a file which is too big)
#define SOCKET_ERROR (-1)
#define SERVER_BACKLOG 5 // Maximum number of pending connections
#define THREAD_POOL_SIZE 10 // Number of threads in the pool

// threading inspired by 
// https://www.youtube.com/watch?v=Pg_4Jz8ZIH4
// 
pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond_var = PTHREAD_COND_INITIALIZER;

int check_socket_error(int socket_fd, const char *msg)
{
	if (socket_fd == SOCKET_ERROR)
	{
		perror(msg);
		exit(1);
	}
	return socket_fd;
}

void help()
{
	printf("Author: Name Surname\n");
	printf("About: This program is a simple interactive shell written in C using client-server architectire.\n");
	printf("Usage: ./shell [OPTIONS]\n");
	printf("Options:\n");
	printf("  -h, \t\tShow this help message and exit\n");
	printf("  -p [port_number], \t\tSet the port number (default: 60069)\n");
	printf("  -u [socket_path], \t\tSet the socket (default: /tmp/socket)\n");
	printf("  -c, \t\tRun as client\n");
	printf("  -s, \t\tRun as server\n");
	printf("  -i, \t\tSet IP address\n");
	printf("  -v, \t\tEnable verbose output\n");
	printf("  -l [log_path], \t\tWrite logs to a file\n");
	printf("Commands in shell:\n");
	printf("  help, \tShow this help message\n");
	printf("  halt, \tHalt the server\n");
	printf("  quit, \tQuit the shell\n");
}
 

int main(int argc, char **argv) 
{
	int c;
	int index;
	char * p_value = NULL;
	char * u_value = NULL;
	char * l_value = NULL;
	char * i_value = NULL;
	bool c_flag, s_flag, v_flag;
	c_flag = s_flag = v_flag = false;

	for (index = 1; index < argc; index++) {
		if (strcmp(argv[index], "-halt") == 0) {
			printf("Halting the server...\n");
			// Add halt logic here
			return 0;
		}
		else if (strcmp(argv[index], "-quit") == 0) {
			printf("Quitting the shell...\n");
			return 0;
		} 
		else if (strcmp(argv[index], "-help") == 0) {
			help();
			return 0;
		}
	}

	static struct option long_options[] = {
		{"help", no_argument, 0, 'h'},
		{"port", required_argument, 0, 'p'},
		{"socket", required_argument, 0, 'u'},
		{"client", no_argument, 0, 'c'},
		{"server", no_argument, 0, 's'},
		{"ip", required_argument, 0, 'i'},
		{"verbose", no_argument, 0, 'v'},
		{"log", required_argument, 0, 'l'},
		{"quit", no_argument, 0, 'q'},
		{"halt", no_argument, 0, 'x'},
		{0, 0, 0, 0}
	};

	int option_index = 0;
	
	opterr = 0;
  
	while ((c = getopt_long(argc, argv, "hcsvp:u:i:l:qx", long_options, &option_index)) != -1)
	{
	  switch (c)
		{
		case 'h':
			help();
			return 0;
		case 'q':  // quit command
			printf("Quitting the shell...\n");
			return 0;
		case 'x':  // halt command
			printf("Halting the server...\n");
			return 0;
		case 'p':
			// set port
			printf("%c\n", c);
			p_value = optarg;
			if (p_value != NULL)
			{
				printf("Port: %s\n", p_value);
			}
			else
			{
				printf("Port not set\n");
			}
			break;
		case 'u':
			// set socket
			printf("%c\n", c);
			u_value = optarg;
			if (u_value != NULL)
			{
				printf("Socket: %s\n", u_value);
			}
			else
			{
				printf("Socket not set\n");
			}
			break;
		case 'c':
			// run as client
			printf("%c\n", c);
			c_flag = true;
			break;
		case 's':
			// run as server
			printf("%c\n", c);
			s_flag = true;
			break;
		case 'i':
			// set IP
			printf("%c\n", c);
			i_value = optarg;
			if (i_value != NULL)
			{
				printf("IP: %s\n", i_value);
			}
			else
			{
				printf("IP not set\n");
			}
			break;
		case 'v':
			// verbose output (show debug info)
			printf("%c\n", c);
			v_flag = true;
			break;
		case 'l':
			// write logs to a file
			printf("%c\n", c);
			l_value = optarg;
			if (l_value != NULL)
			{
				printf("Log file: %s\n", l_value);
			}
			else
			{
				printf("Log file not set\n");
			}
			break;
		case '?':
			if (optopt == 'p' || optopt == 'u' || optopt == 'l' || optopt == 'i')
			{
				fprintf (stderr, "Option -%c requires an argument.\n", optopt);
			}
			else if (isprint (optopt))
			{
				fprintf (stderr, "Unknown option `-%c'.\n", optopt);
			}
			else
			{
				fprintf (stderr, "Unknown option character `%x'.\n", optopt);
			}

			return 1;
		default:
			abort ();
		}
	}

	// check if c_flag and s_flag are set at the same time
	if (c_flag && s_flag)
	{
		fprintf(stderr, "Error: Cannot run as client and server at the same time\n");
		return 1;
	}
  
	// Check for any unknown arguments
	if (optind < argc) {
		printf("Unknown arguments:");
		for (index = optind; index < argc; index++) {
			printf(" %s", argv[index]);
		}
		printf("\n");
		return 1;
	}


	return 0;
  }