#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <dirent.h>
#include <sched.h>
#include <signal.h>

// Function to get file extension from given path
// Credit: ThiefMaster on StackOverflow (http://stackoverflow.com/questions/5309471/getting-file-extension-in-c)
char *getFileExtension(char *filename) {
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) {
		return "";
	}    
	return dot + 1;
}

char *getRequestPath(char *request, int inset) {
	char *filePath = malloc(256);
	
	int i;
	for (i = inset; request[i] != ' ' && request[i] != '\n' && request[i] != '\0'; i++) {
   		filePath[i - inset] = request[i];
	}
	filePath[i - inset] = 0;
	
	return filePath;
}

void http_error(int errorCode, int client){
	char *buffer = malloc(256);
    
	switch(errorCode){
		case 404:
		    sprintf(buffer, "HTTP/ 1.1  404 NOT FOUND\n");   
		    write(client, buffer, strlen(buffer));
		    sprintf(buffer, "Content-Type: text/html\n\n");
		    write(client, buffer, strlen(buffer));
		    sprintf(buffer, "<html><head><title> 404 NOT FOUND</title></head>");
		    write(client, buffer, strlen(buffer));
		    sprintf(buffer, "<body><h1>404 NOT FOUND</h1></body></html>\n");
		    write(client, buffer, strlen(buffer));
		    break;
		case 501:
		    sprintf(buffer, "HTTP/ 1.1 501 NOT IMPLEMENTED\n");
		    write(client, buffer, strlen(buffer));
		    sprintf(buffer, "Content-Type: text/html\n\n");
		    write(client, buffer, strlen(buffer));
		    sprintf(buffer, "<html><head><title> 501 NOT IMPLEMENTED</title></head>");
		    write(client, buffer, strlen(buffer));
		    sprintf(buffer, "<body><h1>501 NOT IMPLEMENTED</h1></body></html>\n");
		    write(client, buffer, strlen(buffer));
		    break;
		default:
		    break;
    }
    
    close(client);  
    exit(0);
}

int create_bind(char *num) {
	int portNum, sockNum;
	struct sockaddr_in serverAddr;

	portNum = atoi(num);

	if ((sockNum = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "**(Error code 3)\n");
		exit(1);
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(portNum);

	int size = sizeof(serverAddr);

	if (bind(sockNum, (struct sockaddr *) &serverAddr, size) == -1) {
		perror("Error: could not bind.\n");
		exit(1);
	}

	return sockNum;
}

int getRequest(int client) {
	char output[4096];

	// Read the request from the client into the buffer
	char buffer[1024];
	int re = read(client, buffer, 1024);

	// Throw error if not GET request
	if (strncmp(buffer, "GET ", 4) != 0) {
		http_error(501, client);
		exit(1);
	}

	// Check if directory listing of root directory
	if (strncmp(buffer, "GET / ", 6) == 0) {
		strcpy(buffer, "GET /. ");
	}

	// Get the path from the request
	char *filePath = getRequestPath(buffer, 5);

	// Check if file at path exists
	FILE *file;
	if ((file = fopen(filePath, "r")) == NULL) {
		http_error(404, client);
		exit(1);
	}

	// Write success message
	sprintf(output, "HTTP/1.1 200 OK\r\n");
	write(client, output, strlen(output));

	// Get the file type from the extension
	char *fileType = getFileExtension(filePath);
	
	// Compare fileType to the different supported filetypes
	if (strcmp(fileType, "jpg") == 0 || strcmp(fileType, "jpeg") == 0 || strcmp(fileType, "gif") == 0) {
		// Write the content type		
		if (strcmp(fileType, "gif") == 0) {
			sprintf(output, "Content-Type: image/gif\n\n");
		} else {
			sprintf(output, "Content-Type: image/jpeg\n\n");
		}
		write(client, output, strlen(output));

		// Get file descriptor for file at path
		int img = open(filePath, O_RDONLY, 0);

		// Get information about the image
		struct stat stat_struct;
		if (fstat(img, &stat_struct) == -1) {
			perror("* ERROR: image stat issue\n");
		}
		int imgSize = stat_struct.st_size;
		if (imgSize == -1) {
			perror("* ERROR: image size issue\n");
		}
	
		// Send image
		size_t total = 0;
		ssize_t bytesSent;		
		while(total < imgSize) {
			bytesSent = sendfile(client, img, 0, imgSize-total);
			if (bytesSent <= 0) {
				perror("* ERROR: image transfer issue\n");
			}
			total += bytesSent;
		}
	} else if (strcmp(fileType, "html") == 0 || (strcmp(fileType, "txt") == 0)) {
		// Write the content type
		if ((strcmp(fileType, ".txt") == 0)) {
			sprintf(output, "Content-Type: text/plain\n\n");
		} else {
			sprintf(output, "Content-Type: text/html\n\n");
		}
		write(client, output, strlen(output));

		// Send file
		fgets(output, sizeof(output), file);
		while(!feof(file)) {
			send(client, output, strlen(output), 0);
			fgets(output, sizeof(output), file);
		}
	} else if (strcmp(fileType, "cgi") == 0) {
		// Create a pipe to redirect the output
		int pipe1[1];
		pipe(pipe1);
		
		// Fork the curent process to run the script 
		int pid = fork();
		if (pid == 0) {
			dup2(pipe1[1], 1);
			close(pipe1[0]);
			
			// Execute the script at filePath
			execl(filePath, filePath, NULL);

			exit(0);
		} else {
			close(pipe1[1]);
			waitpid(pid, NULL, 0);
			
			// Write the result
			char buf;
			while(read(pipe1[0], &buf, sizeof(buf)) > 0) {
				write(client, &buf, 1);
			}

			close(pipe1[0]);
		}
	} else {
		// Open the directory
		DIR *directory = opendir(filePath);
		if (directory != NULL) {
			// Write the content type
			sprintf(output, "Content-Type: text/plain:\n\nDirectory Listing:\n\n");
			write(client, output, strlen(output));
			
			// Traverse contents of directory
			struct dirent *dent;
			while((dent = readdir(directory)) != NULL) {
				// Write current entry
				sprintf(output, dent->d_name);
				write(client, output, strlen(output));
				write(client, "\n", 1);
			}
			
			// Close directory
			closedir(directory);
		} else {
			// Print error if directory can't be found/ opened
			http_error(404, client);
		}
	}

	
	close(client);
	exit(0);
	return 0;
}

int main(int argc, char *argv[]) {
	
	// Listen for connections
	int sockFd = create_bind(argv[1]);
	if (listen(sockFd, 10) < 0) {
		fprintf(stderr, "* ERROR: issues listening for a connection\n");
	}

	printf("* Web Server is online!\n");

	while(1) {
		struct sockaddr_in clientAddr;
		int size = sizeof(clientAddr);
		
		// Get file descriptor
		int clientFd;
		if ((clientFd = accept(sockFd, (struct sockaddr *) &clientAddr, &size)) < 0) {
			fprintf(stderr, "* ERROR: could not bind socket and get file descriptor\n");
			continue;
		}

		// Create a child process
		if (fork() == 0) {
			close(sockFd);

			// Execute the request
			getRequest(clientFd);
			close(clientFd);
			exit(0);
		}

		close(clientFd);
	}
}
