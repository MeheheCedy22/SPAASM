// Author: Name Surname
/* MANDATORY TASKS */
/*
- [x] -help ; -halt ; -quit program arguments
- [x] help, halt, quit commands in shell
- [x] at least 4 of these 6 special characters: # ; < > | \ implemented: # ; > \
- [x] option to specify -p port and -u socket as program arguments
- [x] shell prompt should look like: hh:mm user@hostname$ use system calls and libraries to get time, user and hostname
- [x] running commands and file redirects must be implemented using syscalls CANNOT USE popen() OR SIMILAR
- [x] program argument -s is for server which will listen on socket and accept connections from clients and will do all the above work basically
- [x] program argument -c is for client which will establish connection to server through socket where client will send its stdin and will read the data from server for output to its stdout
- [x] implement basic error handling for all the above tasks
- [x] no warning can be shown when compiling even with -Wall
*/

/* VOLUNTARY TASKS I HAVE IMPLEMENTED */
/*
- [x] 7. (2 body) option to specify -i ip address as program argument
- [x] 11. (2 body) use of external custom library
- [x] 14. (1 bod) option to specify -v verbose output as program argument
- [x] 18. (2 body) option to specify -l for log file as program argument
- [x] 21. (2 body) functional Makefile
- [x] 23. (1 bod) good comments in english
*/

/* SOURCES */
/*
- Custom Shell
	- https://brennan.io/2015/01/16/write-a-shell-in-c/

- Getopts
	- https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
	- https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html


- https://www.youtube.com/@JacobSorber/videos
	- Multithreading
	- https://www.youtube.com/watch?v=Pg_4Jz8ZIH4
	- https://www.youtube.com/watch?v=FMNnusHqjpw
	- https://www.youtube.com/watch?v=P6Z5K8zmEmc
	- Network programming
	- https://www.youtube.com/watch?v=HpAdCgs9HCI
	- https://www.youtube.com/watch?v=il4N6KjVQ-s
	- https://www.youtube.com/watch?v=Y6pFtgRdUts

- Lectures from SPAASM course
- In some parts of the program, AI (LLM) is a co-author

*/

// Termín odovzdávania: 20.04.2025 23:59
// Ročník, ak. rok, semester, odbor: tretí, 2024/2025, letný, informatika
// Riešenie:

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
#include <time.h>
#include <pwd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <netdb.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <stdarg.h>

#include "custom_queue.h"

#define DEFAULT_IP_ADDRESS "127.0.0.1"
#define DEFAULT_SERVER_PORT 60069
#define BUFFER_SIZE 1024 // size of buffer for client command send to server
#define SERVER_BACKLOG 5 // max number of pending connections
#define THREAD_POOL_SIZE 10
#define MAX_HOSTNAME_LENGTH 256

// flag for server shutdown
volatile sig_atomic_t server_running = 1;

// global log file pointer
FILE *log_file_ptr = NULL;

// thread pool and synchronization variables
pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond_var = PTHREAD_COND_INITIALIZER;

typedef struct prompt_data
{
	char time_str[6]; // hh:mm format with null terminator
	char username[32];
	char hostname[MAX_HOSTNAME_LENGTH];
} prompt_data_t;

// function prototypes
void log_message(const char *format, ...);
void help();
prompt_data_t prompt(bool print);
int run_unified_server(const char *port_str, const char *socket_path, const char *ip_address, bool verbose, const char *log_file);
int run_unified_client(const char *port_str, const char *socket_path, const char *ip_address, bool verbose);
int execute_command(char *command, bool redirect_output, int output_fd);
void handle_client(int client_socket);
void sigchld_handler(int s);
void handle_shutdown(int sig);
void setup_signal_handlers();
void *thread_function(void *arg);

// function to log messages
void log_message(const char *format, ...)
{
	if (!log_file_ptr)
		return;

	// get current timestamp
	time_t now = time(NULL);
	struct tm *tm_info = localtime(&now);
	char timestamp[20];
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

	// write timestamp to log
	fprintf(log_file_ptr, "[%s] ", timestamp);

	// write message with variable arguments
	va_list args;
	va_start(args, format);
	vfprintf(log_file_ptr, format, args);
	va_end(args);

	// write to log
	fflush(log_file_ptr);
}

// signal handler to prevent zombie processes
void sigchld_handler(int s)
{
	// wait for all dead processes
	while (waitpid(-1, NULL, WNOHANG) > 0)
	{
		// do nothing, just collect	 the dead processes
	};

	// re-establish the signal handler
	signal(SIGCHLD, sigchld_handler);
}

// signal handler for server shutdown
void handle_shutdown(int sig)
{
	server_running = 0;
	printf("Server shutting down...\n");
}

// set up all signal handlers
void setup_signal_handlers()
{
	// set up signal handlers for server shutdown
	signal(SIGINT, handle_shutdown);
	signal(SIGTERM, handle_shutdown);
	signal(SIGUSR1, handle_shutdown);

	// set up SIGCHLD handler to prevent zombie processes
	signal(SIGCHLD, sigchld_handler);
}

// help command implementation
void help()
{
	printf("Author: Name Surname\n");
	printf("About: Simple interactive shell written in C using client-server architecture.\n");
	printf("Usage: ./shell [OPTIONS]\n");
	printf("Options:\n");
	printf("  -h, --help \tShow this help message and exit\n");
	printf("  -p PORT \tSet the port number\n");
	printf("  -u SOCKET \tSet the socket\n");
	printf("  -c \tRun as client, sends to default port 60069\n");
	printf("  -s \tRun as server, receives on default port 60069\n");
	printf("  -i IP \tSet IP address\n");
	printf("  -v \tEnable verbose output\n");
	printf("  -l FILE \tWrite logs to a file\n");
	printf("Commands in shell:\n");
	printf("  help \tShow this help message\n");
	printf("  halt \tHalt the server\n");
	printf("  quit \tQuit the current shell\n");
	printf("Special characters:\n");
	printf("  # \tComment (everything after # is ignored)\n");
	printf("  ; \tCommand separator (example: echo example1 ; echo example 2)\n");
	printf("  > \tRedirect output to file (example: ls > example.txt)\n");
	printf("  \\ \tLine continuation character (allows writing commands across multiple lines, not an escape char)\n");
}

// get current time, username, and hostname for the prompt
prompt_data_t prompt(bool print)
{
	prompt_data_t prompt_data;

	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	char time_str[6]; // hh:mm plus null terminator
	strftime(time_str, sizeof(time_str), "%H:%M", tm);

	// get username
	struct passwd *pw = getpwuid(getuid());
	const char *username = pw ? pw->pw_name : "user";

	// get hostname
	char hostname[MAX_HOSTNAME_LENGTH];
	if (gethostname(hostname, sizeof(hostname)) != 0)
	{
		strcpy(hostname, "unknown");
	}

	// print the prompt for the server stdin
	// prompt for client is send by server
	if (print)
	{
		printf("%s %s@%s$ ", time_str, username, hostname);
		fflush(stdout);
	}

	// copy the strings to the struct with null termination
	strncpy(prompt_data.time_str, time_str, sizeof(prompt_data.time_str) - 1);
	prompt_data.time_str[sizeof(prompt_data.time_str) - 1] = '\0';
	strncpy(prompt_data.username, username, sizeof(prompt_data.username) - 1);
	prompt_data.username[sizeof(prompt_data.username) - 1] = '\0';
	strncpy(prompt_data.hostname, hostname, sizeof(prompt_data.hostname) - 1);
	prompt_data.hostname[sizeof(prompt_data.hostname) - 1] = '\0';

	return prompt_data;
}

// worker function for thread pool
void *thread_function(void *arg)
{
	// set thread to be cancelable
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	while (server_running)
	{
		int *pclient;
		pthread_mutex_lock(&queue_mutex);

		// wait for a client to be available in the queue
		while ((pclient = dequeue()) == NULL && server_running)
		{
			// wait for signal that a client was added to queue
			pthread_cond_wait(&queue_cond_var, &queue_mutex);
		}

		pthread_mutex_unlock(&queue_mutex);

		// check if server is shutting down
		if (!server_running)
		{
			break;
		}

		// handle client if we got one
		if (pclient != NULL)
		{
			handle_client(*pclient);
			close(*pclient);
			free(pclient);
		}
	}

	return NULL;
}

int execute_command(char *command, bool redirect_output, int output_fd)
{
	if (!command || strlen(command) == 0)
	{
		return 0;
	}

	// trim leading and trailing whitespace
	char *cmd = command;
	while (*cmd == ' ' || *cmd == '\t')
		cmd++;

	char *end = cmd + strlen(cmd) - 1;
	while (end > cmd && (*end == ' ' || *end == '\t' || *end == '\n'))
	{
		*end = '\0';
		end--;
	}

	if (strlen(cmd) == 0)
	{
		return 0;
	}

	// check for built-in commands
	if (strcmp(cmd, "help") == 0)
	{
		help();
		return 0;
	}
	else if (strcmp(cmd, "halt") == 0)
	{
		printf("Server halting...\n");
		// signal to halt the server
		server_running = 0;

		// force immediate return to main to handle shutdown
		if (getpid() == getpgid(0)) // only if this is the main process
		{
			exit(0); // force immediate exit
		}

		return -1; // signal to halt the server
	}
	else if (strcmp(cmd, "quit") == 0)
	{
		printf("Exiting shell...\n");
		return -2; // signal to quit
	}

	// fork and execute command
	pid_t pid = fork();
	if (pid < 0)
	{
		perror("fork failed");
		return 1;
	}
	else if (pid == 0) // child process
	{
		if (redirect_output && output_fd > 0)
		{
			// redirect stdout to file
			dup2(output_fd, STDOUT_FILENO);
			close(output_fd);
		}

		// execute the command
		// max of 63 arguments + NULL terminator for a command
		char *args[64];
		int i = 0;

		char *token = strtok(cmd, " \t");
		while (token != NULL && i < 63)
		{
			args[i++] = token;
			token = strtok(NULL, " \t");
		}
		args[i] = NULL;

		execvp(args[0], args);

		// if execvp returns, it means an error occurred
		perror("execvp failed");
		exit(EXIT_FAILURE);
	}
	else
	{
		// parent process
		int status;
		waitpid(pid, &status, 0);
		return WEXITSTATUS(status);
	}
}

// client handling with dynamic output buffer
void handle_client(int client_socket)
{
	char buffer[BUFFER_SIZE];
	ssize_t bytes_read;
	int pipes[2]; // for capturing command output

	// get client address info for logging
	struct sockaddr_storage client_addr;
	socklen_t addr_len = sizeof(client_addr);
	char client_info[50] = "unknown";

	// try to get client information
	if (getpeername(client_socket, (struct sockaddr *)&client_addr, &addr_len) == 0)
	{
		if (client_addr.ss_family == AF_INET)
		{
			// ipv4 client
			struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
			char ip_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &s->sin_addr, ip_str, sizeof(ip_str));
			snprintf(client_info, sizeof(client_info), "%s:%d", ip_str, ntohs(s->sin_port));
		}
		else if (client_addr.ss_family == AF_UNIX)
		{
			// unix domain socket client
			strcpy(client_info, "unix-socket");
		}
	}

	// send initial welcome message
	const char *welcome_msg = "Connected to shell server. Type commands or 'quit' to exit.\n";
	send(client_socket, welcome_msg, strlen(welcome_msg), 0);

	while (server_running)
	{
		// send prompt
		char prompt_buffer[BUFFER_SIZE];
		prompt_data_t prompt_data = prompt(false);

		snprintf(prompt_buffer, BUFFER_SIZE, "%s %s@%s$ ", prompt_data.time_str, prompt_data.username, prompt_data.hostname);
		send(client_socket, prompt_buffer, strlen(prompt_buffer), 0);

		// receive command from client
		bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
		if (bytes_read <= 0)
		{
			break; // client disconnected or error
		}

		buffer[bytes_read] = '\0';

		// remove newline character if present
		if (bytes_read > 0 && buffer[bytes_read - 1] == '\n')
		{
			buffer[bytes_read - 1] = '\0';
		}

		// log the received command
		if (log_file_ptr)
		{
			log_message("Client [%s] command: %s\n", client_info, buffer);
		}

		// process the command
		if (strncmp(buffer, "quit", 4) == 0)
		{
			const char *goodbye = "Disconnecting from server. Goodbye!\n";
			send(client_socket, goodbye, strlen(goodbye), 0);
			break;
		}

		// create pipe for capturing command output
		if (pipe(pipes) < 0)
		{
			perror("pipe failed");
			continue;
		}

		// save stdout and stderr
		int stdout_backup = dup(STDOUT_FILENO);
		int stderr_backup = dup(STDERR_FILENO);

		// redirect stdout and stderr to the pipe
		dup2(pipes[1], STDOUT_FILENO);
		dup2(pipes[1], STDERR_FILENO);
		close(pipes[1]); // close write end of pipe in parent

		// process command: handle continuation, comments, command separator, and redirection
		char *command = buffer;
		char *cmd_copy = strdup(command);
		if (!cmd_copy)
		{
			perror("Memory allocation failed");
			dup2(stdout_backup, STDOUT_FILENO);
			dup2(stderr_backup, STDERR_FILENO);
			close(stdout_backup);
			close(stderr_backup);
			close(pipes[0]);
			continue;
		}

		// handle line continuation character (\) - collect additional input lines
		bool continuation = false;
		if (strlen(cmd_copy) > 0 && cmd_copy[strlen(cmd_copy) - 1] == '\\')
		{
			// remove the continuation character
			cmd_copy[strlen(cmd_copy) - 1] = '\0';

			// flag that we need to continue reading
			continuation = true;

			// send a continuation prompt to the client
			const char *cont_prompt = "> ";
			send(client_socket, cont_prompt, strlen(cont_prompt), 0);

			// read additional lines
			char cont_buffer[BUFFER_SIZE];
			while (continuation && server_running)
			{
				bytes_read = recv(client_socket, cont_buffer, BUFFER_SIZE - 1, 0);
				if (bytes_read <= 0)
				{
					break; // client disconnected or error
				}

				cont_buffer[bytes_read] = '\0';

				// remove newline character if present
				if (bytes_read > 0 && cont_buffer[bytes_read - 1] == '\n')
				{
					cont_buffer[bytes_read - 1] = '\0';
				}

				// check if this line also ends with continuation
				if (strlen(cont_buffer) > 0 && cont_buffer[strlen(cont_buffer) - 1] == '\\')
				{
					// remove the continuation character
					cont_buffer[strlen(cont_buffer) - 1] = '\0';

					// append to command without adding the backslash
					char *new_cmd = malloc(strlen(cmd_copy) + strlen(cont_buffer) + 2); // +2 for space and null terminator
					if (new_cmd)
					{
						sprintf(new_cmd, "%s %s", cmd_copy, cont_buffer);
						free(cmd_copy);
						cmd_copy = new_cmd;
					}

					// send another continuation prompt
					send(client_socket, cont_prompt, strlen(cont_prompt), 0);
				}
				else
				{
					// last continuation line - append and finish
					char *new_cmd = malloc(strlen(cmd_copy) + strlen(cont_buffer) + 2);
					if (new_cmd)
					{
						sprintf(new_cmd, "%s %s", cmd_copy, cont_buffer);
						free(cmd_copy);
						cmd_copy = new_cmd;
					}
					continuation = false;
				}
			}
		}

		// handle comments (ignore everything after #)
		char *comment = strchr(cmd_copy, '#');
		if (comment)
		{
			*comment = '\0';
		}

		// handle command separators (;)
		char *cmd_part = strtok(cmd_copy, ";");
		while (cmd_part != NULL)
		{
			// handle redirection (>)
			char *redirect = strchr(cmd_part, '>');
			if (redirect)
			{
				*redirect = '\0'; // split the command at '>'
				redirect++;		  // move to the filename

				// skip leading whitespace
				while (*redirect == ' ' || *redirect == '\t')
					redirect++;

				// get the filename
				char filename[256] = {0};
				sscanf(redirect, "%255s", filename);

				if (strlen(filename) > 0)
				{
					int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if (fd < 0)
					{
						fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
					}
					else
					{
						execute_command(cmd_part, true, fd);
						close(fd);
					}
				}
			}
			else
			{
				// regular command execution
				int result = execute_command(cmd_part, false, -1);
				if (result == -1)
				{ // halt command
					free(cmd_copy);

					// restore stdout and stderr
					dup2(stdout_backup, STDOUT_FILENO);
					dup2(stderr_backup, STDERR_FILENO);
					close(stdout_backup);
					close(stderr_backup);
					close(pipes[0]);

					const char *halt_msg = "Server halting command received\n";
					send(client_socket, halt_msg, strlen(halt_msg), 0);

					return;
				}
			}

			// get next command part
			cmd_part = strtok(NULL, ";");
		}

		free(cmd_copy);

		// flush stdout to ensure all output is written to the pipe
		fflush(stdout);
		fflush(stderr);

		// restore stdout and stderr
		dup2(stdout_backup, STDOUT_FILENO);
		dup2(stderr_backup, STDERR_FILENO);
		close(stdout_backup);
		close(stderr_backup);

		// read command output using dynamic buffer with getline
		FILE *pipe_stream = fdopen(pipes[0], "r");
		if (!pipe_stream)
		{
			perror("Failed to create stream from pipe");
			close(pipes[0]);
			continue;
		}

		// use dynamic buffer with getline to read all output
		char *output_line = NULL;
		size_t line_buf_size = 0;
		ssize_t line_length;
		char *complete_output = NULL;
		size_t total_size = 0;

		while ((line_length = getline(&output_line, &line_buf_size, pipe_stream)) != -1)
		{
			// allocate/reallocate combined output buffer
			char *new_complete = realloc(complete_output, total_size + line_length + 1);
			if (!new_complete)
			{
				perror("Memory allocation failed");
				break;
			}
			complete_output = new_complete;

			// copy new line to the end of complete output
			memcpy(complete_output + total_size, output_line, line_length);
			total_size += line_length;
			complete_output[total_size] = '\0';
		}

		// clean up the line buffer
		free(output_line);
		fclose(pipe_stream); // this also closes pipes[0]

		// send output to client
		if (complete_output && total_size > 0)
		{
			send(client_socket, complete_output, total_size, 0);

			// send extra newline if needed
			if (total_size > 0 && complete_output[total_size - 1] != '\n')
			{
				const char *newline = "\n";
				send(client_socket, newline, 1, 0);
			}
		}

		// free the complete output buffer
		free(complete_output);
	}
}

// unified server with dynamic output buffer (unified means for both tcp and unix sockets)
int run_unified_server(const char *port_str, const char *socket_path, const char *ip_address, bool verbose, const char *log_file)
{
	// server socket file descriptors
	int tcp_server_fd = -1;
	int unix_server_fd = -1;

	// set flag that server is running
	server_running = 1;

	// open log file if specified
	if (log_file != NULL && strlen(log_file) > 0)
	{
		log_file_ptr = fopen(log_file, "a");
		if (!log_file_ptr)
		{
			perror("Failed to open log file");
			// continue without logging
		}
		else
		{
			log_message("Server started\n");
			if (verbose)
			{
				printf("Logging to: %s\n", log_file);
			}
		}
	}

	// set up socket address structures
	struct sockaddr_in tcp_address;
	struct sockaddr_un unix_address;
	int opt = 1;

	// get port number
	int port = port_str ? atoi(port_str) : DEFAULT_SERVER_PORT;

	// set up signal handlers
	setup_signal_handlers();

	// create tcp socket if port is specified or if no socket path is provided
	if (port_str != NULL || socket_path == NULL || strlen(socket_path) == 0)
	{
		if ((tcp_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
		{
			perror("TCP socket creation failed");
			// continue with unix socket if that's available
		}
		else
		{
			// set socket options
			if (setsockopt(tcp_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
			{
				perror("setsockopt failed for TCP socket");
				close(tcp_server_fd);
				tcp_server_fd = -1;
			}
			else
			{
				// set up tcp address
				tcp_address.sin_family = AF_INET;

				// use specified ip address if provided, otherwise use inaddr_any
				if (ip_address != NULL)
				{
					// use the specified ip address
					if (inet_pton(AF_INET, ip_address, &tcp_address.sin_addr) <= 0)
					{
						perror("Invalid IP address");
						close(tcp_server_fd);
						tcp_server_fd = -1;
					}
				}
				else
				{
					// default: listen on all interfaces
					tcp_address.sin_addr.s_addr = INADDR_ANY;
				}

				tcp_address.sin_port = htons(port);

				// bind the tcp socket to the port
				if (bind(tcp_server_fd, (struct sockaddr *)&tcp_address, sizeof(tcp_address)) < 0)
				{
					perror("TCP socket bind failed");
					close(tcp_server_fd);
					tcp_server_fd = -1;
				}
				else
				{
					// listen on tcp socket
					if (listen(tcp_server_fd, SERVER_BACKLOG) < 0)
					{
						perror("TCP socket listen failed");
						close(tcp_server_fd);
						tcp_server_fd = -1;
					}
					else if (verbose)
					{
						printf("Server listening on TCP port %d\n", port);
						if (log_file_ptr)
						{
							log_message("TCP server listening on port %d\n", port);
						}
					}
				}
			}
		}
	}

	// create unix domain socket if path is specified
	if (socket_path != NULL && strlen(socket_path) > 0)
	{
		if ((unix_server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == 0)
		{
			perror("Unix socket creation failed");
			// continue with tcp socket if that's available
		}
		else
		{
			// remove existing socket file if any
			unlink(socket_path);

			// set up unix address
			memset(&unix_address, 0, sizeof(struct sockaddr_un));
			unix_address.sun_family = AF_UNIX;
			strncpy(unix_address.sun_path, socket_path, sizeof(unix_address.sun_path) - 1);

			// bind the unix socket to the path
			if (bind(unix_server_fd, (struct sockaddr *)&unix_address, sizeof(unix_address)) < 0)
			{
				perror("Unix socket bind failed");
				close(unix_server_fd);
				unix_server_fd = -1;
			}
			else
			{
				// listen on unix socket
				if (listen(unix_server_fd, SERVER_BACKLOG) < 0)
				{
					perror("Unix socket listen failed");
					close(unix_server_fd);
					unix_server_fd = -1;
				}
				else
				{
					if (verbose)
					{
						printf("Server listening on Unix socket: %s\n", socket_path);
						if (log_file_ptr)
						{
							log_message("Unix domain socket server listening on: %s\n", socket_path);
						}
					}
				}
			}
		}
	}

	// check if at least one socket was successfully created
	if (tcp_server_fd < 0 && unix_server_fd < 0)
	{
		fprintf(stderr, "Failed to create any server sockets\n");
		if (log_file_ptr)
		{
			log_message("Failed to create any server sockets\n");
			fclose(log_file_ptr);
		}
		return -1;
	}

	// initialize the thread pool
	for (int i = 0; i < THREAD_POOL_SIZE; i++)
	{
		if (pthread_create(&thread_pool[i], NULL, thread_function, NULL) != 0)
		{
			perror("Failed to create thread");
			// continue with fewer threads
		}
	}

	// file descriptor sets for select()
	fd_set readfds;
	int max_fd;

	// show initial prompt
	prompt(true);

	// main server loop
	while (server_running)
	{
		// clear the file descriptor sets
		FD_ZERO(&readfds);

		// add stdin to readfds
		FD_SET(STDIN_FILENO, &readfds);
		max_fd = STDIN_FILENO;

		// add tcp socket to readfds if it exists
		if (tcp_server_fd >= 0)
		{
			FD_SET(tcp_server_fd, &readfds);
			max_fd = (tcp_server_fd > max_fd) ? tcp_server_fd : max_fd;
		}

		// add unix socket to readfds if it exists
		if (unix_server_fd >= 0)
		{
			FD_SET(unix_server_fd, &readfds);
			max_fd = (unix_server_fd > max_fd) ? unix_server_fd : max_fd;
		}

		// wait for activity on any of the file descriptors
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		int activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);
		if (activity < 0 && errno != EINTR)
		{
			perror("select error");
			break;
		}

		// check for new tcp connections
		if (tcp_server_fd >= 0 && FD_ISSET(tcp_server_fd, &readfds))
		{
			struct sockaddr_in client_addr;
			socklen_t addrlen = sizeof(client_addr);

			// accept incoming connection
			int client_socket = accept(tcp_server_fd, (struct sockaddr *)&client_addr, &addrlen);
			if (client_socket < 0)
			{
				perror("accept failed for TCP connection");
				continue;
			}

			if (verbose)
			{
				char client_ip[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
				printf("Client connected from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
				if (log_file_ptr)
				{
					log_message("Client connected from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
				}
			}

			// add client to the queue for thread pool
			int *pclient = malloc(sizeof(int));
			if (pclient == NULL)
			{
				perror("Failed to allocate memory for client socket");
				close(client_socket);
			}
			else
			{
				*pclient = client_socket;

				pthread_mutex_lock(&queue_mutex);
				enqueue(pclient);
				pthread_cond_signal(&queue_cond_var);
				pthread_mutex_unlock(&queue_mutex);
			}
		}

		// check for new unix socket connections
		if (unix_server_fd >= 0 && FD_ISSET(unix_server_fd, &readfds))
		{
			struct sockaddr_un client_addr;
			socklen_t addrlen = sizeof(client_addr);

			// accept incoming connection
			int client_socket = accept(unix_server_fd, (struct sockaddr *)&client_addr, &addrlen);
			if (client_socket < 0)
			{
				perror("accept failed for Unix socket connection");
				continue;
			}

			if (verbose)
			{
				printf("Client connected on Unix socket\n");
				if (log_file_ptr)
				{
					log_message("Client connected on Unix socket\n");
				}
			}

			// add client to the queue for thread pool
			int *pclient = malloc(sizeof(int));
			if (pclient == NULL)
			{
				perror("Failed to allocate memory for client socket");
				close(client_socket);
			}
			else
			{
				*pclient = client_socket;

				pthread_mutex_lock(&queue_mutex);
				enqueue(pclient);
				pthread_cond_signal(&queue_cond_var);
				pthread_mutex_unlock(&queue_mutex);
			}
		}

		// check if there's input from stdin
		if (FD_ISSET(STDIN_FILENO, &readfds))
		{
			char command[BUFFER_SIZE];
			static char full_command[BUFFER_SIZE * 4] = {0}; // larger buffer for multi-line commands
			static bool in_continuation = false;

			// read command
			if (fgets(command, sizeof(command), stdin) == NULL)
			{
				// EOF or error
				if (feof(stdin))
				{
					printf("\nEOF received, exiting...\n");
					server_running = 0;
					break;
				}
				perror("fgets error");
				continue;
			}

			// remove newline
			size_t len = strlen(command);
			if (len > 0 && command[len - 1] == '\n')
			{
				command[len - 1] = '\0';
				len--;
			}

			// check for line continuation
			if (len > 0 && command[len - 1] == '\\')
			{
				// remove the backslash
				command[len - 1] = '\0';

				// append this line to the full command
				if (in_continuation)
				{
					// add space between continuation lines
					strcat(full_command, " ");
					strcat(full_command, command);
				}
				else
				{
					// first line of a multi-line command
					strcpy(full_command, command);
					in_continuation = true;
				}

				// show continuation prompt
				printf("> ");
				fflush(stdout);
				continue; // continue reading more input
			}
			else if (in_continuation)
			{
				// last line of a multi-line command - append it
				strcat(full_command, " ");
				strcat(full_command, command);

				// use the complete multi-line command
				strcpy(command, full_command);

				// log the complete multi-line command
				if (log_file_ptr && strlen(command) > 0)
				{
					log_message("Server console command: %s\n", command);
				}

				in_continuation = false;
				memset(full_command, 0, sizeof(full_command));
			}
			else
			{
				// regular single-line command - log it normally
				if (log_file_ptr && strlen(command) > 0)
				{
					log_message("Server console command: %s\n", command);
				}
			}

			int tmp_prompt = 0;
			// process command only if non-empty
			if (strlen(command) > 0)
			{
				// create pipe for capturing command output
				int pipes[2];
				if (pipe(pipes) < 0)
				{
					perror("pipe failed");
					prompt(true);
					continue;
				}

				// save stdout and stderr
				int stdout_backup = dup(STDOUT_FILENO);
				int stderr_backup = dup(STDERR_FILENO);

				// redirect stdout and stderr to the pipe
				dup2(pipes[1], STDOUT_FILENO);
				dup2(pipes[1], STDERR_FILENO);
				close(pipes[1]); // close write end of pipe

				// create a copy of the command for processing
				char *cmd_copy = strdup(command);
				if (!cmd_copy)
				{
					perror("Memory allocation failed");
					dup2(stdout_backup, STDOUT_FILENO);
					dup2(stderr_backup, STDERR_FILENO);
					close(stdout_backup);
					close(stderr_backup);
					close(pipes[0]);
					prompt(true);
					continue;
				}

				// handle comments (ignore everything after #)
				char *comment = strchr(cmd_copy, '#');
				if (comment)
				{
					*comment = '\0';
				}

				// handle command separators (;)
				char *cmd_part = strtok(cmd_copy, ";");
				while (cmd_part != NULL && server_running)
				{
					// handle redirection (>)
					char *redirect = strchr(cmd_part, '>');
					if (redirect)
					{
						*redirect = '\0'; // split the command at '>'
						redirect++;		  // move to the filename

						// skip leading whitespace
						while (*redirect == ' ' || *redirect == '\t')
							redirect++;

						// get the filename
						char filename[256] = {0};
						sscanf(redirect, "%255s", filename);

						if (strlen(filename) > 0)
						{
							int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
							if (fd < 0)
							{
								fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
							}
							else
							{
								int result = execute_command(cmd_part, true, fd);
								if (result == -1 || result == -2)
								{
									tmp_prompt = result;
								}
								close(fd);
							}
						}
					}
					else
					{
						// regular command execution
						int result = execute_command(cmd_part, false, -1);
						if (result == -1 || result == -2)
						{
							tmp_prompt = result;
						}
					}

					// get next command part
					cmd_part = strtok(NULL, ";");
				}

				free(cmd_copy);

				// flush output
				fflush(stdout);
				fflush(stderr);

				// restore stdout and stderr
				dup2(stdout_backup, STDOUT_FILENO);
				dup2(stderr_backup, STDERR_FILENO);
				close(stdout_backup);
				close(stderr_backup);

				// read command output using dynamic buffer with getline
				FILE *pipe_stream = fdopen(pipes[0], "r");
				if (!pipe_stream)
				{
					perror("Failed to create stream from pipe");
					close(pipes[0]);
					prompt(true);
					continue;
				}

				// use dynamic buffer with getline to read all output
				char *output_line = NULL;
				size_t line_buf_size = 0;
				ssize_t line_length;
				char *complete_output = NULL;
				size_t total_size = 0;

				while ((line_length = getline(&output_line, &line_buf_size, pipe_stream)) != -1)
				{
					// allocate/reallocate combined output buffer
					char *new_complete = realloc(complete_output, total_size + line_length + 1);
					if (!new_complete)
					{
						perror("Memory allocation failed");
						break;
					}
					complete_output = new_complete;

					// copy new line to the end of complete output
					memcpy(complete_output + total_size, output_line, line_length);
					total_size += line_length;
					complete_output[total_size] = '\0';
				}

				// clean up the line buffer
				free(output_line);
				fclose(pipe_stream); // this also closes pipes[0]

				// print the output
				if (complete_output && total_size > 0)
				{
					printf("%s", complete_output);

					// add newline if output doesn't end with one
					if (total_size > 0 && complete_output[total_size - 1] != '\n')
					{
						printf("\n");
					}
				}

				// free the complete output buffer
				free(complete_output);

				// handle halt or quit command
				if (tmp_prompt == -1 || tmp_prompt == -2)
				{
					server_running = 0;
				}
			}

			// show prompt after command execution if not shutting down
			if (server_running)
			{
				prompt(true);
			}
		}
	}

	// shutdown procedure
	printf("Shutting down server...\n");

	// close all server sockets
	if (tcp_server_fd >= 0)
	{
		close(tcp_server_fd);
	}

	if (unix_server_fd >= 0)
	{
		close(unix_server_fd);
		// remove unix socket file
		if (socket_path != NULL)
		{
			unlink(socket_path);
		}
	}

	if (server_running)
	{
		// normal shutdown - wait for threads
		for (int i = 0; i < THREAD_POOL_SIZE; i++)
		{
			pthread_cond_broadcast(&queue_cond_var);
		}

		for (int i = 0; i < THREAD_POOL_SIZE; i++)
		{
			pthread_join(thread_pool[i], NULL);
		}
	}
	else
	{
		// halt command received - force immediate shutdown
		for (int i = 0; i < THREAD_POOL_SIZE; i++)
		{
			pthread_cancel(thread_pool[i]); // cancel threads immediately
		}
	}

	// close log file if it was opened
	if (log_file_ptr)
	{
		log_message("Server shutting down\n");
		fclose(log_file_ptr);
		log_file_ptr = NULL;
	}

	return 0;
}

// unified client function (unified means for both tcp and unix sockets)
int run_unified_client(const char *port_str, const char *socket_path, const char *ip_address, bool verbose)
{
	int sock = -1;
	char buffer[BUFFER_SIZE] = {0};

	// set terminal to raw mode
	// setup for terminal where backspace and other special characters are handled
	struct termios oldattr;
	tcgetattr(STDIN_FILENO, &oldattr);
	struct termios newattr = oldattr;
	newattr.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newattr);

	// try to connect using unix socket if specified
	if (socket_path != NULL && strlen(socket_path) > 0)
	{
		struct sockaddr_un address;

		// create unix socket
		if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		{
			perror("Unix socket creation error");
			// restore terminal
			tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
			// don't fall back to tcp if unix socket was explicitly requested
			return -1;
		}
		else
		{
			// set up unix address
			memset(&address, 0, sizeof(struct sockaddr_un));
			address.sun_family = AF_UNIX;
			strncpy(address.sun_path, socket_path, sizeof(address.sun_path) - 1);

			// connect to server
			if (connect(sock, (struct sockaddr *)&address, sizeof(struct sockaddr_un)) < 0)
			{
				perror("Unix socket connection failed");
				close(sock);
				// restore terminal
				tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
				// don't fall back to tcp if unix socket was explicitly requested
				return -1;
			}
			else if (verbose)
			{
				printf("Connected to server via Unix socket: %s\n", socket_path);
			}
		}
	}

	// if unix socket connection wasn't specified, try tcp
	if (sock < 0)
	{
		struct sockaddr_in serv_addr;

		// default values
		const char *server_addr = ip_address ? ip_address : DEFAULT_IP_ADDRESS;
		int port = port_str ? atoi(port_str) : DEFAULT_SERVER_PORT;

		// create tcp socket
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			perror("Socket creation error");
			// restore terminal
			tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
			return -1;
		}

		// set up tcp address
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(port);

		// convert ipv4 address from text to binary form
		if (inet_pton(AF_INET, server_addr, &serv_addr.sin_addr) <= 0)
		{
			perror("Invalid address / Address not supported");
			close(sock);
			// restore terminal
			tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
			return -1;
		}

		// connect to server
		if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		{
			perror("Connection Failed");
			close(sock);
			// restore terminal
			tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
			return -1;
		}

		if (verbose)
		{
			printf("Connected to server at %s:%d\n", server_addr, port);
		}
	}

	fd_set readfds;
	char current_cmd[BUFFER_SIZE] = {0};
	int cursor_pos = 0;

	// main client loop
	while (1)
	{
		// setup file descriptors
		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		FD_SET(sock, &readfds);

		int max_fd = (STDIN_FILENO > sock) ? STDIN_FILENO : sock;

		// wait for activity on stdin or socket
		int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

		if (activity < 0)
		{
			if (errno == EINTR)
				continue;
			perror("select failed");
			break;
		}

		// check if server has sent data
		if (FD_ISSET(sock, &readfds))
		{
			memset(buffer, 0, sizeof(buffer));
			int bytes_read = read(sock, buffer, sizeof(buffer) - 1);

			if (bytes_read <= 0)
			{
				if (verbose)
				{
					printf("Server disconnected\n");
				}
				break;
			}

			buffer[bytes_read] = '\0'; // ensure null termination

			// print server response
			printf("%s", buffer);

			// make sure output is flushed
			fflush(stdout);
		}

		// check if user has entered data
		if (FD_ISSET(STDIN_FILENO, &readfds))
		{
			char ch;
			if (read(STDIN_FILENO, &ch, 1) > 0)
			{
				if (ch == '\n')
				{
					// echo the newline to terminal
					printf("\n");

					// check if the command ends with a continuation character
					bool needs_continuation = false;
					if (cursor_pos > 0 && current_cmd[cursor_pos - 1] == '\\')
					{
						// this is a continuation - don't send the command yet
						needs_continuation = true;

						// remove the backslash from the command
						current_cmd[--cursor_pos] = '\0';

						// show continuation prompt to the client
						printf("> ");
						fflush(stdout);
					}

					if (!needs_continuation)
					{
						// send the complete command to the server
						current_cmd[cursor_pos] = '\n';

						/* TODO: TEMPORARY TEST FOR QUITING*/
						/* put the write after the if statement*/

						// write(sock, current_cmd, cursor_pos + 1);

						// // check for quit command
						// if (strncmp(current_cmd, "quit", 4) == 0)
						// {
						// 	if (verbose)
						// 	{
						// 		printf("Quitting client...\n");
						// 	}
						// 	break;
						// }

						


						// check for quit command
						if (strncmp(current_cmd, "quit", 4) == 0)
						{
							if (verbose)
							{
								printf("Quitting client...\n");
							}
							break;
						}
						
						write(sock, current_cmd, cursor_pos + 1);



						// reset command buffer
						memset(current_cmd, 0, sizeof(current_cmd));
						cursor_pos = 0;
					}
				}
				else if (ch == 127 || ch == '\b') // backspace
				{
					if (cursor_pos > 0)
					{
						cursor_pos--;
						current_cmd[cursor_pos] = '\0';
						printf("\b \b"); // erase character from terminal
						fflush(stdout);
					}
				}
				else if (isprint(ch)) // add character to command
				{
					if (cursor_pos < BUFFER_SIZE - 2) // leave room for \n\0
					{ 
						current_cmd[cursor_pos++] = ch;
						printf("%c", ch); // print the character
						fflush(stdout);
					}
				}
			}
		}
	}

	// restore terminal settings
	tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);

	close(sock);
	return 0;
}

int main(int argc, char **argv)
{
	int c;
	int index;
	char *p_value = NULL;
	char *u_value = NULL;
	char *l_value = NULL;
	char *i_value = NULL;
	bool c_flag, s_flag, v_flag;
	c_flag = s_flag = v_flag = false;

	// check for standalone -help, -halt, -quit options
	for (index = 1; index < argc; index++)
	{
		if (strcmp(argv[index], "-halt") == 0)
		{
			printf("Halting the server...\n");
			return 0;
		}
		else if (strcmp(argv[index], "-quit") == 0)
		{
			printf("Quitting the shell...\n");
			return 0;
		}
		else if (strcmp(argv[index], "-help") == 0)
		{
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
		{0, 0, 0, 0}};

	int option_index = 0;

	opterr = 0;

	while ((c = getopt_long(argc, argv, "hcsvp:u:i:l:qx", long_options, &option_index)) != -1)
	{
		switch (c)
		{
		case 'h': // help
			help();
			return 0;
		case 'q': // quit
			printf("Quitting the shell...\n");
			return 0;
		case 'x': // halt
			printf("Halting the server...\n");
			return 0;
		case 'p': // set port
			p_value = optarg;
			if (optarg != NULL && v_flag)
			{
				printf("Port: %s\n", p_value);
			}
			// check if port is valid
			if (p_value != NULL)
			{
				int port = atoi(p_value);
				if (port < 1 || port > 65535)
				{
					fprintf(stderr, "Invalid port number: %s\n", p_value);
					return 1;
				}
			}
			break;
		case 'u': // set socket
			u_value = optarg;
			if (optarg != NULL && v_flag)
			{
				printf("Socket: %s\n", u_value);
			}
			break;
		case 'c': // run as client
			c_flag = true;
			if (v_flag)
			{
				printf("Running as client\n");
			}
			break;
		case 's': // run as server
			s_flag = true;
			if (v_flag)
			{
				printf("Running as server\n");
			}
			break;
		case 'i': // set ip
			i_value = optarg;
			if (optarg != NULL && v_flag)
			{
				printf("IP: %s\n", i_value);
			}
			break;
		case 'v': // verbose output (show debug info)
			v_flag = true;
			printf("Verbose mode enabled\n");
			break;
		case 'l': // write logs to a file
			l_value = optarg;
			if (optarg != NULL && v_flag)
			{
				printf("Log file: %s\n", l_value);
			}
			break;
		case '?':
			if (optopt == 'p' || optopt == 'u' || optopt == 'l' || optopt == 'i')
			{
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			}
			else if (isprint(optopt))
			{
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
			}
			else
			{
				fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			}

			return 1;
		default:
			abort();
		}
	}

	// check for any unknown arguments
	if (optind < argc)
	{
		printf("Unknown arguments:");
		for (index = optind; index < argc; index++)
		{
			printf(" %s", argv[index]);
		}
		printf("\n");
		return 1;
	}

	// server and client cannot be set at the same time
	if (c_flag && s_flag)
	{
		fprintf(stderr, "Error: Cannot run as client and server at the same time\n");
		return 1;
	}

	if (c_flag && l_value != NULL)
	{
		fprintf(stderr, "Error: Cannot use log file with client mode\n");
		return 1;
	}

	// run in the appropriate mode (default: server)
	if (c_flag)
	{
		// run as client
		return run_unified_client(p_value, u_value, i_value, v_flag);
	}
	else
	{
		// run as server (default if no mode specified)
		return run_unified_server(p_value, u_value, i_value, v_flag, l_value);
	}
}
