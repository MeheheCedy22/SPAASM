#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

// TODO: implement -help and -halt arguments when program is run as client

void help()
{
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
	char * pvalue = NULL;
	char * uvalue = NULL;
	char * lvalue = NULL;
	char * Cvalue = NULL;
  
	opterr = 0;
  
	while ((c = getopt (argc, argv, "::hp:u:csivdl:C:")) != -1)
	{
	  switch (c)
		{
		case 'h':
			help();
			break;
		case 'p':
			// set port
			printf("%c\n", c);
			pvalue = optarg;
			if (pvalue != NULL)
			{
				printf("Port: %s\n", pvalue);
			}
			else
			{
				printf("Port not set\n");
			}
			break;
		case 'u':
			// set socket
			printf("%c\n", c);
			pvalue = optarg;
			if (pvalue != NULL)
			{
				printf("Socket: %s\n", pvalue);
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
			pvalue = optarg;
			if (pvalue != NULL)
			{
				printf("Log file: %s\n", pvalue);
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
			pvalue = optarg;
			if (pvalue != NULL)
			{
				printf("Config file: %s\n", pvalue);
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