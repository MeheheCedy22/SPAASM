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
#include <pthread.h>
#include "custom_queue.h"

/* THESE ARE THE MAIN TASKS WHICH ARE MANDATORY OTHERS ARE OPTIONAL*/
/* WHAT TO IMPLEMENT / TODOs / TASKS
- [ ] -help ; -halt ; -quit program arguments
- [ ] help, halt, quit commands in shell
- [ ] at least 4 of these 6 special characters: # ; < > | \ i want to implement at least: # ; \ >
- [ ] option to specify -p port and -u socket as program arguments
- [ ] shell prompt should look like: time user@hostname$ use system calls and libraries to get time, user and hostname
- [ ] running commands and file redirects must be implemented using syscalls CANNOT USE popen() OR SIMILAR
- [ ] program argument -s is for server which will listen on socket and accept connections from clients and will do all the above work basically
- [ ] program argument -c is for client which will establish connection to server through socket where client will send its stdin and will read the data from server for output
- [ ] implement basic error handling for all the above tasks
- [ ] no warning can be shown when compiling even with -Wall
*/


#define THREAD_POOL_SIZE 10

void help()
{
	printf("Author: Name Surname\n");
	printf("About: This program is a simple interactive shell using client-server architectire.\n");
	printf("Usage: ./main [OPTIONS]\n");
	printf("Options:\n");
	printf("  -h, \t\tShow this help message and exit\n");
	printf("  -p, \t\tSet the port number (default: 60069)\n");
	printf("  -u, \t\tSet the socket (default: /tmp/socket)\n");
	printf("  -c, \t\tRun as client\n");
	printf("  -s, \t\tRun as server\n");
	printf("  -i, \t\tShow IP address\n");
	printf("  -v, \t\tEnable verbose output\n");
	printf("  -d, \t\tRun server as daemon\n");
	printf("  -l, \t\tWrite logs to a file\n");
	printf("  -C, \t\tSet the config file\n");
}
 

int main(int argc, char **argv) 
{
	int c;
	int index;
	char * p_value = NULL;
	char * u_value = NULL;
	char * l_value = NULL;
	char * C_value = NULL;
	bool h_flag, p_flag, u_flag, c_flag, s_flag, i_flag, v_flag, d_flag, l_flag, C_flag;
	h_flag = p_flag = u_flag = c_flag = s_flag = i_flag = v_flag = d_flag = l_flag = C_flag = false;
	
	opterr = 0;
  
	while ((c = getopt (argc, argv, "hp:u:csivdl:C:")) != -1)
	{
	  switch (c)
		{
		case 'h':
			help();
			break;
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
			p_value = optarg;
			if (p_value != NULL)
			{
				printf("Socket: %s\n", p_value);
			}
			else
			{
				printf("Socket not set\n");
			}
			break;
			break;
		case 'c':
			// run as client
			printf("%c\n", c);
		  break;
		case 's':
			// run as server
			printf("%c\n", c);
			break;
		case 'i':
			// show IP
			printf("%c\n", c);
			break;
		case 'v':
			// verbose output (show debug info)
			printf("%c\n", c);
			break;
		case 'd':
			// run server as deamon
			printf("%c\n", c);
			break;
		case 'l':
			// write logs to a file
			printf("%c\n", c);
			p_value = optarg;
			if (p_value != NULL)
			{
				printf("Log file: %s\n", p_value);
			}
			else
			{
				printf("Log file not set\n");
			}
			break;
			break;
		case 'C':
			// set config file
			printf("%c\n", c);
			p_value = optarg;
			if (p_value != NULL)
			{
				printf("Config file: %s\n", p_value);
			}
			else
			{
				printf("Config file not set\n");
			}
			break;
			break;
		case '?':
			if (optopt == 'p' || optopt == 'u' || optopt == 'l' || optopt == 'C')
			{
				fprintf (stderr, "Option -%c requires an argument.\n", optopt);
			}
			else if (isprint (optopt))
			{
				fprintf (stderr, "Unknown option `-%c'.\n", optopt);
			}
			else
			{
				fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
			}

			return 1;
		default:
			abort ();
		}
	}
  
	for (index = optind; index < argc; index++)
	{
		printf ("Non-option argument %s\n", argv[index]);
	}


	return 0;
  }