
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
// Include necessary header files
#define buffer_size 1000
#define BASE_DIRECTORY "txt files";

  
/**
  * 
  * this function will run if GET is sent to the server 
  * it will display the contents of a specified file to client
  * 
  * @param *file name: a pointer to the name of the file to be open
  * @param client_fd: is the file descriptor of the client being communicated to 
  * @param buffer[]: holding the message that will be sent back to the client 
  * 
  * 
  */
 void handle_get(char *file_name, int client_fd, char buffer[]){
    //making the file path into the txt files folder
    char file_path[50]; // Adjust size as needed
	snprintf(file_path, sizeof(file_path), "txt file/%s", file_name);

	//opening file and setting it to READ mode 
	FILE *file = fopen(file_path, "r");
	if(file != NULL){
		send(client_fd, "SERVER 200 OK\n\n", 15, 0);

		while(fgets(buffer, buffer_size, file)){
			send(client_fd, buffer, strlen(buffer), 0);
		}

		send(client_fd, "\n\n", 2, 0);

		fclose(file);
	}

	//if no file was found 
	else{
		//sending error 
		send(client_fd, "SERVER 404 Not Found\n" , strlen("SERVER 404 Not Found\n"), 0);
	}
 }
 
 /**
  * this program will run if PUT is sent to the server, and it will open one of the files within the same directory 
  * it will then overwrite the contents of the file and onlyterminate once sent two empty lines
  * @param *file name: a pointer to the name of the file to be open
  * @param client_fd: is the file descriptor of the client being communicated to 
  * @param buffer[]: holding the message that will be sent back to the client 
  * 
  * 
  */

 void handle_put(char *file_name, int client_fd, char buffer[]){
    //making the file path into the txt files folder
    char file_path[50]; // Adjust size as needed
	snprintf(file_path, sizeof(file_path), "txt files/%s", file_name);
	
	//opening file and setting it to WRITE mode
	FILE *file = fopen(file_path, "w");

	if(file != NULL){
		//varaible to keep count of empty lines();
		int empty_count = 0;
		while(1){
			ssize_t bytes_recevieved = recv(client_fd, buffer, buffer_size, 0);
			if(bytes_recevieved < 0){
				perror("error recieving message");
				fclose(file);
			}
			else if(bytes_recevieved == 0){
				printf("client disconnected");
				break;
			}
			
			//if its a empty line add to empty count
			else if(bytes_recevieved == 1 && buffer[0] == '\n'){
				empty_count++;
				fwrite(buffer, 1, bytes_recevieved, file);
				
				//checking whether there have been two consectuive lines
				if (empty_count >= 2) {
					break;
				}
			}
			//if no new line detected reset count and write to file
			else{
				fwrite(buffer, 1, bytes_recevieved, file);
				empty_count = 0;
			}
		}
		fclose(file);
		send(client_fd, "SERVER 201 Created\n", 19, 0);
	}
	//if error occurs with command
	else{
		send(client_fd, "SERVER 501 Put Error\n" , 21, 0);
	}
}

//void handle 
  
  
/**
 * The main function should be able to accept a command-line argument
 * argv[0]: program name
 * argv[1]: port number
 * 
 * Read the assignment handout for more details about the server program
 * design specifications.
 */ 
int main(int argc, char *argv[]){  
    //checking if port number is valid
    int port = atoi(argv[1]);
    if(port < 1024){
        perror("Invalid portnumber");
        return -1;
    }

	//if an invalid amount of arugments are ran with the program
    if (argc != 2) {
        return -1;
    }
    
    //creating the TCP socket 
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1){
        perror("Error with socket creation");
        return -1;
    }

    //creation of socket address
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;\
    addr.sin_port = ntohs(port);
    addr.sin_addr.s_addr = INADDR_ANY; 
	
	//binding socket to a port
    if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("Error binding socket");
        return -1;
    }
    printf("binding successful");
	
	//setting up the listener 
	if(listen(fd, SOMAXCONN) < 0){
		perror("Error listening for connections");
		return -1;
	}
	printf("Listen successful");
	
	//accepting the client connection
	struct sockaddr_in clientaddr;
	socklen_t addrlen = sizeof(clientaddr);
	int client_fd = accept(fd, (struct sockaddr *)&clientaddr, (socklen_t *)&addrlen);
	if(client_fd < 0){
		perror("Error accepting connection to client");
		return -1;
	} 
	char msg[] = "HELLO\n";

	ssize_t r = send(client_fd, &msg, strlen(msg), 0);
	if(r < 0 ){
		perror("Error sending message;");
		return -1;
	}
	
	//continuous loop to accept client messages and send back to them
	while(1){
		
		//variable to hold the messages that will be sent
		char buffer[buffer_size];
		ssize_t recvieved = recv(client_fd, buffer,buffer_size, 0);
		if(recvieved < 0){
			perror("Error recvieing messsage");
			close(client_fd);
			break;
		}
		if(recvieved == 0){
			printf("client disconnected");
			break;
		}

		if(strncasecmp(buffer, "BYE", 3) == 0){ 	
			//closing the socket from client
			close(client_fd);
			printf("Connection closed");
			break;
		} 
		else if(strncasecmp(buffer, "GET", 3) == 0){

			//getting the file name with the offset of the mes_recv and setting delimiters
			char *file_name = strtok(buffer + 4, " \n");
			if(file_name != NULL && strcmp(file_name, "") != 0){
			   handle_get(file_name, client_fd, buffer);
			}
			else{
			   send(client_fd, "SERVER 500 Get Error\n", 21, 0);
			}
			  
		}
		else if(strncasecmp(buffer, "PUT", 3) == 0){
			//geting file name again
			char *file_name = strtok(buffer + 4, " \n");
			if(file_name != NULL && strcmp(file_name, "") != 0){
			   handle_put(file_name, client_fd, buffer);
			}
			else{
			   send(client_fd, "SERVER 501 put Error\n", 21, 0);
			}
		}

		else if(strncasecmp(buffer, "DELETE", 3) == 0 ){
			char *file_name = strtok(buffer + 4, " \n");
		}

		//if error with command being sent
		else{
			// Send 502 Command Error
			send(client_fd, "SERVER 502 Command Error\n", 25, 0);
		}
		
		
	}
	return 0;
}


