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

#define KEY_ESCAPE  0x001b
#define KEY_ENTER   0x000a
#define KEY_UP      0x0105
#define KEY_DOWN    0x0106
#define KEY_RIGHT   0x0107
#define KEY_LEFT    0x0108

#define DEFAULT_PORT 60069
#define DEFAULT_SOCKET "/tmp/socket"
#define BUFFER_SIZE 1024
#define MAX_COMMAND_LENGTH 1024
#define MAX_HOSTNAME_LENGTH 256

// Flag for server shutdown
volatile sig_atomic_t server_running = 1;

// Function prototypes
void help();
void prompt();
int run_server(int port, bool daemon_mode, const char *log_file);
int run_client(int port, const char *server_address);
int execute_command(char *command, bool redirect_output, int output_fd);
void handle_client(int client_socket);
int getch(void);
int read_key(void);
void sigchld_handler(int s);
void handle_shutdown(int sig);
void setup_signal_handlers();
int send_all(int socket, const char *buffer, size_t length);

// Signal handler to prevent zombie processes
void sigchld_handler(int s) {
	// Wait for all dead processes
	while (waitpid(-1, NULL, WNOHANG) > 0);
	
	// Re-establish the signal handler
	signal(SIGCHLD, sigchld_handler);
}

// Signal handler for server shutdown
void handle_shutdown(int sig) {
	server_running = 0;
	printf("Server shutting down...\n");
}

// Set up all signal handlers
void setup_signal_handlers() {
	// Set up signal handlers for server shutdown
	signal(SIGINT, handle_shutdown);
	signal(SIGTERM, handle_shutdown);
	signal(SIGUSR1, handle_shutdown);
	
	// Set up SIGCHLD handler to prevent zombie processes
	signal(SIGCHLD, sigchld_handler);
}

// Safe send function to handle partial sends and errors
int send_all(int socket, const char *buffer, size_t length) {
	size_t sent = 0;
	while (sent < length) {
		ssize_t result = send(socket, buffer + sent, length - sent, 0);
		if (result <= 0) {
			if (errno == EINTR) continue;
			return -1;
		}
		sent += result;
	}
	return 0;
}

// Gets a character without waiting for Enter key
int getch(void) {
	struct termios oldattr, newattr;
	int ch;
	
	tcgetattr(STDIN_FILENO, &oldattr);
	newattr = oldattr;
	newattr.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
	
	ch = getchar();
	
	tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
	return ch;
}

// Read a key including special keys like arrows
int read_key(void) {
	int ch = getch();
	
	if (ch == KEY_ESCAPE) {
		ch = getch();
		if (ch == '[') {
			ch = getch();
			switch (ch) {
				case 'A': return KEY_UP;
				case 'B': return KEY_DOWN;
				case 'C': return KEY_RIGHT;
				case 'D': return KEY_LEFT;
			}
		}
	}
	
	return ch;
}

void help()
{
	printf("Author: Name Surname\n");
	printf("About: This program is a simple interactive shell using client-server architecture.\n");
	printf("Usage: ./main [OPTIONS]\n");
	printf("Options:\n");
	printf("  -h, --help \tShow this help message and exit\n");
	printf("  -p PORT, \tSet the port number (default: 60069)\n");
	printf("  -u SOCKET, \tSet the socket (default: /tmp/socket)\n");
	printf("  -c, \t\tRun as client\n");
	printf("  -s, \t\tRun as server\n");
	printf("  -i, \t\tShow IP address\n");
	printf("  -v, \t\tEnable verbose output\n");
	printf("  -d, \t\tRun server as daemon\n");
	printf("  -l FILE, \tWrite logs to a file\n");
	printf("  -C FILE, \tSet the config file\n");
	printf("Commands in shell:\n");
	printf("  help \t\tShow this help message\n");
	printf("  halt \t\tHalt the server\n");
	printf("  quit \t\tExit the shell\n");
	printf("Special characters:\n");
	printf("  # \t\tComment (everything after # is ignored)\n");
	printf("  ; \t\tCommand separator\n");
	printf("  \\ \t\tEscape character\n");
	printf("  > \t\tRedirect output to file\n");
}

// Get current time, username, and hostname for the prompt
void prompt() {
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	char time_str[9];
	strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);
	
	// Get username
	struct passwd *pw = getpwuid(getuid());
	const char *username = pw ? pw->pw_name : "user";
	
	// Get hostname
	char hostname[MAX_HOSTNAME_LENGTH];
	if (gethostname(hostname, sizeof(hostname)) != 0) {
		strcpy(hostname, "unknown");
	}
	
	printf("%s %s@%s$ ", time_str, username, hostname);
	fflush(stdout);
}


// Enhanced execute_command function with output capture
int execute_command(char *command, bool redirect_output, int output_fd) {
	if (!command || strlen(command) == 0) {
		return 0;
	}
	
	// Trim leading and trailing whitespace
	char *cmd = command;
	while (*cmd == ' ' || *cmd == '\t') cmd++;
	
	char *end = cmd + strlen(cmd) - 1;
	while (end > cmd && (*end == ' ' || *end == '\t' || *end == '\n')) {
		*end = '\0';
		end--;
	}
	
	if (strlen(cmd) == 0) {
		return 0;
	}
	
	// Check for built-in commands
	if (strcmp(cmd, "help") == 0) {
		help();
		return 0;
	} else if (strcmp(cmd, "halt") == 0) {
		printf("Server halting...\n");
		return -1; // Signal to halt the server
	} else if (strcmp(cmd, "quit") == 0) {
		printf("Exiting shell...\n");
		return -2; // Signal to quit
	}
	
	// Fork and execute command
	pid_t pid = fork();
	if (pid < 0) {
		perror("fork failed");
		return 1;
	} else if (pid == 0) {
		// Child process
		if (redirect_output && output_fd > 0) {
			// Redirect stdout to file
			dup2(output_fd, STDOUT_FILENO);
			close(output_fd);
		}
		
		// Execute the command
		char *args[64];
		int i = 0;
		
		char *token = strtok(cmd, " \t");
		while (token != NULL && i < 63) {
			args[i++] = token;
			token = strtok(NULL, " \t");
		}
		args[i] = NULL;
		
		execvp(args[0], args);
		
		// If execvp returns, it means an error occurred
		perror("execvp failed");
		exit(EXIT_FAILURE);
	} else {
		// Parent process
		int status;
		waitpid(pid, &status, 0);
		return WEXITSTATUS(status);
	}
}

// Improved handle_client function with proper input validation and error handling
void handle_client(int client_socket) {
	char buffer[BUFFER_SIZE];
	ssize_t bytes_read;
	int pipes[2];  // For capturing command output
	
	// Send initial welcome message
	const char *welcome_msg = "Connected to shell server. Type commands or 'quit' to exit.\n";
	if (send_all(client_socket, welcome_msg, strlen(welcome_msg)) < 0) {
		perror("send failed");
		return;
	}
	
	while (1) {
		// Send prompt
		char prompt_buffer[BUFFER_SIZE];
		time_t t = time(NULL);
		struct tm *tm = localtime(&t);
		char time_str[9];
		strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);
		
		struct passwd *pw = getpwuid(getuid());
		const char *username = pw ? pw->pw_name : "user";
		
		char hostname[MAX_HOSTNAME_LENGTH];
		if (gethostname(hostname, sizeof(hostname)) != 0) {
			strcpy(hostname, "unknown");
		}
		
		snprintf(prompt_buffer, BUFFER_SIZE, "%s %s@%s$ ", time_str, username, hostname);
		if (send_all(client_socket, prompt_buffer, strlen(prompt_buffer)) < 0) {
			perror("send failed");
			break;
		}
		
		// Receive command from client
		bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
		if (bytes_read <= 0) {
			break; // Client disconnected or error
		}

		buffer[bytes_read] = '\0';

		// Remove newline character if present
		if (bytes_read > 0 && buffer[bytes_read - 1] == '\n') {
			buffer[bytes_read - 1] = '\0';
		}
		
		// Validate input - remove non-printable characters
		for (int i = 0; i < bytes_read; i++) {
			if (!isprint(buffer[i]) && !isspace(buffer[i])) {
				buffer[i] = ' ';
			}
		}
		
		// Process the command
		if (strncmp(buffer, "quit", 4) == 0) {
			const char *goodbye = "Disconnecting from server. Goodbye!\n";
			send_all(client_socket, goodbye, strlen(goodbye));
			break;
		}
		
		// Create pipe for capturing command output
		if (pipe(pipes) < 0) {
			perror("pipe failed");
			continue;
		}
		
		// Save stdout and stderr
		int stdout_backup = dup(STDOUT_FILENO);
		int stderr_backup = dup(STDERR_FILENO);
		
		if (stdout_backup < 0 || stderr_backup < 0) {
			perror("dup failed");
			if (stdout_backup >= 0) close(stdout_backup);
			if (stderr_backup >= 0) close(stderr_backup);
			close(pipes[0]);
			close(pipes[1]);
			continue;
		}
		
		// Redirect stdout and stderr to the pipe
		if (dup2(pipes[1], STDOUT_FILENO) < 0 || dup2(pipes[1], STDERR_FILENO) < 0) {
			perror("dup2 failed");
			close(stdout_backup);
			close(stderr_backup);
			close(pipes[0]);
			close(pipes[1]);
			continue;
		}
		close(pipes[1]);  // Close write end of pipe in parent
		
		// Process command: handle comments, command separator, and redirection
		char *command = buffer;
		char *cmd_copy = strdup(command);
		if (!cmd_copy) {
			perror("Memory allocation failed");
			dup2(stdout_backup, STDOUT_FILENO);
			dup2(stderr_backup, STDERR_FILENO);
			close(stdout_backup);
			close(stderr_backup);
			close(pipes[0]);
			continue;
		}
		
		// Handle comments (ignore everything after #)
		char *comment = strchr(cmd_copy, '#');
		if (comment) {
			*comment = '\0';
		}
		
		// Handle command separators (;)
		char *cmd_part = strtok(cmd_copy, ";");
		while (cmd_part != NULL) {
			// Handle redirection (>)
			char *redirect = strchr(cmd_part, '>');
			if (redirect) {
				*redirect = '\0';  // Split the command at '>'
				redirect++;        // Move to the filename
				
				// Skip leading whitespace
				while (*redirect == ' ' || *redirect == '\t') redirect++;
				
				// Get the filename
				char filename[256] = {0};
				sscanf(redirect, "%255s", filename);
				
				if (strlen(filename) > 0) {
					int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if (fd < 0) {
						fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
					} else {
						execute_command(cmd_part, true, fd);
						close(fd);  // Close the file descriptor after use
					}
				}
			} else {
				// Regular command execution
				int result = execute_command(cmd_part, false, -1);
				if (result == -1) {  // halt command
					free(cmd_copy);
					
					// Restore stdout and stderr
					dup2(stdout_backup, STDOUT_FILENO);
					dup2(stderr_backup, STDERR_FILENO);
					close(stdout_backup);
					close(stderr_backup);
					close(pipes[0]);
					
					const char *halt_msg = "Server halting command received\n";
					send_all(client_socket, halt_msg, strlen(halt_msg));
					
					// Signal main server to shut down
					kill(getppid(), SIGUSR1);
					return;
				}
			}
			
			// Get next command part
			cmd_part = strtok(NULL, ";");
		}
		
		free(cmd_copy);
		
		// Flush stdout to ensure all output is written to the pipe
		fflush(stdout);
		fflush(stderr);
		
		// Restore stdout and stderr
		dup2(stdout_backup, STDOUT_FILENO);
		dup2(stderr_backup, STDERR_FILENO);
		close(stdout_backup);
		close(stderr_backup);
		
		// Read command output from pipe with proper buffer size checking
		char output_buffer[BUFFER_SIZE] = {0};
		ssize_t bytes_available;
		if (ioctl(pipes[0], FIONREAD, &bytes_available) < 0) {
			perror("ioctl failed");
			close(pipes[0]);
			continue;
		}
		
		if (bytes_available > BUFFER_SIZE - 1)
			bytes_available = BUFFER_SIZE - 1;
			
		ssize_t output_size = read(pipes[0], output_buffer, bytes_available);
		close(pipes[0]);  // Close read end of pipe
		
		if (output_size > 0) {
			output_buffer[output_size] = '\0';
			// Send output to client
			if (send_all(client_socket, output_buffer, output_size) < 0) {
				perror("send failed");
				break;
			}
		}
	}
}

// Improved server function with proper shutdown and error handling
int run_server(int port, bool daemon_mode, const char *log_file) {
	int server_fd, client_socket;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	
	// Initialize server_running flag
	server_running = 1;
	
	// Create socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		return -1;
	}
	
	// Set socket options
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		perror("setsockopt failed");
		close(server_fd);
		return -1;
	}
	
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
	
	// Bind the socket to the port
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed");
		close(server_fd);
		return -1;
	}
	
	// Listen for connections
	if (listen(server_fd, 3) < 0) {
		perror("listen failed");
		close(server_fd);
		return -1;
	}
	
	printf("Server started on port %d\n", port);
	if (daemon_mode) {
		printf("Running in daemon mode\n");
		// Fork to create daemon
		pid_t pid = fork();
		
		if (pid < 0) {
			perror("fork failed");
			close(server_fd);
			exit(EXIT_FAILURE);
		}
		
		if (pid > 0) {
			// Parent process exits
			printf("Daemon started with PID %d\n", pid);
			close(server_fd);
			exit(EXIT_SUCCESS);
		}
		
		// Child process continues
		umask(0);
		
		// Create new session
		if (setsid() < 0) {
			perror("setsid failed");
			close(server_fd);
			exit(EXIT_FAILURE);
		}
		
		// Redirect standard file descriptors to /dev/null
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		
		if (log_file != NULL) {
			int log_fd = open(log_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
			if (log_fd >= 0) {
				dup2(log_fd, STDOUT_FILENO);
				dup2(log_fd, STDERR_FILENO);
				close(log_fd);
			}
		} else {
			int null_fd = open("/dev/null", O_RDWR);
			if (null_fd >= 0) {
				dup2(null_fd, STDIN_FILENO);
				dup2(null_fd, STDOUT_FILENO);
				dup2(null_fd, STDERR_FILENO);
				close(null_fd);
			}
		}
	}
	
	// Set up signal handlers
	setup_signal_handlers();
	
	// Accept and handle connections in a loop with server_running flag
	while (server_running) {
		// Use select with timeout to make shutdown responsive
		struct timeval tv = {1, 0};  // 1 second timeout
		
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(server_fd, &readfds);
		
		int activity = select(server_fd + 1, &readfds, NULL, NULL, &tv);
		
		if (activity < 0 && errno != EINTR) {
			perror("select error");
			break;
		}
		
		if (!server_running) break;
		
		if (activity > 0 && FD_ISSET(server_fd, &readfds)) {
			if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
				perror("accept failed");
				continue;
			}
			
			// Handle client in a new process
			pid_t pid = fork();
			if (pid < 0) {
				perror("fork failed");
				close(client_socket);
				continue;
			} else if (pid == 0) {
				// Child process handles the client
				close(server_fd);  // Close listening socket in child
				handle_client(client_socket);
				close(client_socket);
				exit(EXIT_SUCCESS);
			} else {
				// Parent process
				close(client_socket);  // Close client socket in parent
			}
		}
	}
	
	// Cleanup code for graceful shutdown
	close(server_fd);
	printf("Server shut down successfully\n");
	return 0;
}

// Improved client function with better error handling
int run_client(int port, const char *server_address) {
	struct termios oldattr;
	tcgetattr(STDIN_FILENO, &oldattr);
	struct termios newattr = oldattr;
	newattr.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
	int sock = 0;
	struct sockaddr_in serv_addr;
	char buffer[BUFFER_SIZE] = {0};
	
	// Remove history arrays and just keep current command
	char current_cmd[MAX_COMMAND_LENGTH] = {0};
	int cursor_pos = 0;
	printf("> ");  // Initial prompt
	fflush(stdout);
	
	// Create socket
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket creation error");
		tcsetattr(STDIN_FILENO, TCSANOW, &oldattr); // Restore terminal
		return -1;
	}
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	
	// Convert IPv4 and IPv6 addresses from text to binary form
	if (inet_pton(AF_INET, server_address, &serv_addr.sin_addr) <= 0) {
		perror("Invalid address/ Address not supported");
		close(sock);
		tcsetattr(STDIN_FILENO, TCSANOW, &oldattr); // Restore terminal
		return -1;
	}
	
	// Connect to server
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("Connection Failed");
		close(sock);
		tcsetattr(STDIN_FILENO, TCSANOW, &oldattr); // Restore terminal
		return -1;
	}
	
	printf("Connected to server at %s:%d\n", server_address, port);
	
	fd_set readfds;
	
	while (1) {
		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		FD_SET(sock, &readfds);
		
		int max_fd = (STDIN_FILENO > sock) ? STDIN_FILENO : sock;
		
		// Use select with timeout to make client more responsive
		struct timeval tv = {5, 0};  // 5 second timeout
		
		int activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);
		
		if (activity < 0) {
			if (errno == EINTR) continue;
			perror("select failed");
			break;
		}
		
		// Check if server has sent data
		if (FD_ISSET(sock, &readfds)) {
			memset(buffer, 0, sizeof(buffer));
			int bytes_read = read(sock, buffer, sizeof(buffer) - 1);
			
			if (bytes_read <= 0) {
				printf("Server disconnected\n");
				break;
			}
			
			buffer[bytes_read] = '\0'; // Ensure null termination
			
			// Clear the current line if there's a command in progress
			if (cursor_pos > 0) {
				printf("\r%*s\r", (int)(strlen(current_cmd) + 2), ""); // +2 for the "> " prompt
			}
			
			// Print server response without any filtering
			printf("%s", buffer);
			fflush(stdout);
			
			// Check if we need to restore prompt
			// Only show prompt if the server response doesn't end with one
			if (bytes_read < 2 || buffer[bytes_read-2] != '$' || buffer[bytes_read-1] != ' ') {
				// Reprint current command with prompt
				if (cursor_pos > 0) {
					printf("> %s", current_cmd);
					// Move cursor back to correct position
					for (int i = strlen(current_cmd); i > cursor_pos; i--) {
						printf("\b");
					}
				} else {
					printf("> ");
				}
				fflush(stdout);
			}
		}
		
		if (FD_ISSET(STDIN_FILENO, &readfds)) {
			int key = read_key();
			
			switch (key) {
				case KEY_LEFT:
					if (cursor_pos > 0) {
						printf("\b");
						fflush(stdout);
						cursor_pos--;
					}
					break;
					
				case KEY_RIGHT:
					if (cursor_pos < strlen(current_cmd)) {
						printf("%c", current_cmd[cursor_pos]);
						fflush(stdout);
						cursor_pos++;
					}
					break;
					
				case KEY_ENTER:
					printf("\n");
					
					// Send to server
					if (send_all(sock, current_cmd, strlen(current_cmd)) < 0 || 
						send_all(sock, "\n", 1) < 0) {
						printf("Error sending command to server\n");
						tcsetattr(STDIN_FILENO, TCSANOW, &oldattr); // Restore terminal
						close(sock);
						return -1;
					}
					
					// Check if user wants to quit
					if (strcmp(current_cmd, "quit") == 0) {
						printf("Exiting...\n");
						tcsetattr(STDIN_FILENO, TCSANOW, &oldattr); // Restore terminal
						close(sock);
						return 0;
					}
					
					// Reset for next command
					memset(current_cmd, 0, sizeof(current_cmd));
					cursor_pos = 0;
					printf("> ");  // New prompt
					fflush(stdout);
					break;
					
				case '\b':  // Backspace
				case 127:   // Delete
					if (cursor_pos > 0) {
						memmove(&current_cmd[cursor_pos - 1], &current_cmd[cursor_pos], 
								strlen(current_cmd) - cursor_pos + 1);
						cursor_pos--;
						
						// Redraw the line - use a wider clear to ensure the prompt is fully cleared
						printf("\r%*s\r", 80, ""); // Clear the entire line with 80 spaces
						printf("> %s", current_cmd);
						
						// Move cursor back to correct position
						for (int i = strlen(current_cmd); i > cursor_pos; i--) {
							printf("\b");
						}
						fflush(stdout);
					}
					break;
					
				default:
					if (isprint(key) && strlen(current_cmd) < MAX_COMMAND_LENGTH - 1) {
						// Insert character at cursor position
						memmove(&current_cmd[cursor_pos + 1], &current_cmd[cursor_pos], 
								strlen(current_cmd) - cursor_pos + 1);
						current_cmd[cursor_pos] = key;
						cursor_pos++;
						
						// Redraw the line from the cursor position
						printf("%s", &current_cmd[cursor_pos - 1]);
						
						// Move cursor back to correct position
						for (int i = strlen(current_cmd); i > cursor_pos; i--) {
							printf("\b");
						}
						fflush(stdout);
					}
					break;
			}
		}
	}

	// Restore normal terminal behavior when exiting
	tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
	
	close(sock);
	return 0;
}

// Improved interactive shell mode with better file descriptor management
int interactive_shell_mode() {
	char command[MAX_COMMAND_LENGTH];
	
	while (1) {
		prompt();
		
		if (fgets(command, sizeof(command), stdin) == NULL) {
			break; // EOF (Ctrl+D)
		}
		
		// Remove trailing newline
		size_t len = strlen(command);
		if (len > 0 && command[len-1] == '\n') {
			command[len-1] = '\0';
		}
		
		// Process command with all special characters
		char *cmd_copy = strdup(command);
		if (!cmd_copy) {
			perror("Memory allocation failed");
			continue;
		}
		
		// Handle escape character (\)
		for (size_t i = 0; i < strlen(cmd_copy); i++) {
			if (cmd_copy[i] == '\\' && i + 1 < strlen(cmd_copy)) {
				// Escape the next character
				memmove(&cmd_copy[i], &cmd_copy[i + 1], strlen(cmd_copy) - i);
			}
		}
		
		// Handle comments (ignore everything after #)
		char *comment = strchr(cmd_copy, '#');
		if (comment) {
			*comment = '\0';
		}
		
		// Handle command separators (;)
		char *cmd_part = strtok(cmd_copy, ";");
		while (cmd_part != NULL) {
			// Handle redirection (>)
			char *redirect = strchr(cmd_part, '>');
			if (redirect) {
				*redirect = '\0';  // Split the command at '>'
				redirect++;        // Move to the filename
				
				// Skip leading whitespace
				while (*redirect == ' ' || *redirect == '\t') redirect++;
				
				// Get the filename
				char filename[256] = {0};
				sscanf(redirect, "%255s", filename);
				
				if (strlen(filename) > 0) {
					int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if (fd < 0) {
						fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
					} else {
						execute_command(cmd_part, true, fd);
						close(fd);  // Close file descriptor after use
					}
				}
			} else {
				// Regular command execution
				int result = execute_command(cmd_part, false, -1);
				if (result == -1 || result == -2) {  // halt or quit command
					free(cmd_copy);
					return 0;
				}
			}
			
			// Get next command part
			cmd_part = strtok(NULL, ";");
		}
		
		free(cmd_copy);
	}
	
	return 0;
}

int main(int argc, char **argv) 
{
	int c;
	int index;
	char *p_value = NULL;
	char *l_value = NULL;
	bool c_flag, s_flag, i_flag, v_flag, d_flag, l_flag;
	c_flag = s_flag = i_flag = v_flag = d_flag = l_flag = false;
	int port = DEFAULT_PORT;
	
	opterr = 0;
  
	while ((c = getopt(argc, argv, "hp:u:csivdl:C:")) != -1)
	{
		switch (c)
		{
		case 'h':
			help();
			return 0;
			break;
		case 'p':
			p_value = optarg;
			if (p_value != NULL) {
				port = atoi(p_value);
				if (port <= 0) {
					fprintf(stderr, "Invalid port number: %s\n", p_value);
					return 1;
				}
			}
			break;
		case 'u':
			break;
		case 'c':
			c_flag = true;
			break;
		case 's':
			s_flag = true;
			break;
		case 'i':
			i_flag = true;
			if (i_flag) {
				char hostname[256];
				struct hostent *host_entry;
				
				gethostname(hostname, sizeof(hostname));
				host_entry = gethostbyname(hostname);
				
				if (host_entry == NULL) {
					printf("Could not get IP address\n");
				} else {
					char *ip = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
					printf("IP Address: %s\n", ip);
				}
			}
			break;
		case 'v':
			v_flag = true;
			break;
		case 'd':
			d_flag = true;
			break;
		case 'l':
			l_flag = true;
			l_value = optarg;
			break;
		case 'C':
			break;
		case '?':
			if (optopt == 'p' || optopt == 'u' || optopt == 'l' || optopt == 'C')
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
	
	// Check for non-option arguments
	for (index = optind; index < argc; index++)
	{
		if (strcmp(argv[index], "--help") == 0) {
			help();
			return 0;
		}
		else if (strcmp(argv[index], "--halt") == 0) {
			printf("Server halting...\n");
			return 0;
		}
		else if (strcmp(argv[index], "--quit") == 0) {
			printf("Exiting...\n");
			return 0;
		}
		else {
			printf("Unknown argument: %s\n", argv[index]);
		}
	}
	
	// Cannot be both client and server
	if (c_flag && s_flag) {
		fprintf(stderr, "Error: Cannot run as both client and server.\n");
		return 1;
	}
	
	if (s_flag) {
		// Run as server
		if (v_flag) printf("Running as server on port %d\n", port);
		return run_server(port, d_flag, l_flag ? l_value : NULL);
	} 
	else if (c_flag) {
		// Run as client
		const char *server_addr = "127.0.0.1"; // Default to localhost
		if (v_flag) printf("Running as client connecting to %s:%d\n", server_addr, port);
		return run_client(port, server_addr);
	}
	else {
		// Interactive shell mode
		return interactive_shell_mode();
	}

	return 0;
}