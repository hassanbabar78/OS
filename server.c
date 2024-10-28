
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <ctype.h>
#include <stdbool.h>
#define PORT 14000
#define STORAGE_LIMIT 1000000000 // 10 KB storage limit per client

#define MAX_ENTRIES 100    // Maximum number of username-password pairs
#define MAX_LENGTH 50      // Maximum length of each username/password

typedef struct {
    char username[MAX_LENGTH];
    char password[MAX_LENGTH];
} User;
struct client_info {
    int used_space;
} client;


#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

void handle_view(int new_socket, const char *username) {
    char dir[1024];
    snprintf(dir, sizeof(dir), "./server_dir/%s", username);  // Construct path to user's directory

    DIR *d;
    struct dirent *dir_entry;
    struct stat file_stat;
    char file_info[1024] = {0};
    char buffer[1024] = {0};

    // Open the directory
    if ((d = opendir(dir)) == NULL) {
        send(new_socket, "$FAILURE$NO_CLIENT_DATA$", strlen("$FAILURE$NO_CLIENT_DATA$"), 0);
        return;
    }

    int has_files = 0;

    // Read through the directory entries
    while ((dir_entry = readdir(d)) != NULL) {
        // Skip "." and ".." entries
        if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0)
            continue;

        has_files = 1;  // Mark that there is at least one file

        // Get the full file path
        char full_path[9999];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, dir_entry->d_name);

        // Get file statistics
        if (stat(full_path, &file_stat) == 0) {
            // File name
            snprintf(buffer, sizeof(buffer), "Name: %s, ", dir_entry->d_name);
            strcat(file_info, buffer);

            // File size
            snprintf(buffer, sizeof(buffer), "Size: %ld bytes, ", file_stat.st_size);
            strcat(file_info, buffer);

            // Last modified time
            struct tm *timeinfo = localtime(&file_stat.st_mtime);
            char time_buf[64];
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", timeinfo);
            snprintf(buffer, sizeof(buffer), "Last Modified: %s\n", time_buf);
            strcat(file_info, buffer);
        }
    }
    closedir(d);

    if (has_files) {
        send(new_socket, file_info, strlen(file_info), 0);
    } else {
        send(new_socket, "$FAILURE$NO_CLIENT_DATA$", strlen("$FAILURE$NO_CLIENT_DATA$"), 0);
    }
}

void handle_upload(int new_socket, char *command, const char *username) {
    char *file_path = strtok(command, "$");
    file_path = strtok(NULL, "$");

    if (client.used_space >= STORAGE_LIMIT) {
        send(new_socket, "$FAILURE$LOW_SPACE$", strlen("$FAILURE$LOW_SPACE$"), 0);
        return;
    }

    send(new_socket, "$SUCCESS$", strlen("$SUCCESS$"), 0);

    char *base_name = strrchr(file_path, '/');
    if (!base_name) {
        base_name = file_path;  // No '/' found, use the full path as the name
    } else {
        base_name++;  // Skip the '/'
    }

    // Include the username in the directory path
    char dir[1024];
    snprintf(dir, sizeof(dir), "./server_dir/%s", username);
    mkdir(dir, 0777);  // Create the directory for the user

    char full_path[9999];
    snprintf(full_path, sizeof(full_path), "%s/%s", dir, base_name);

    FILE *file = fopen(full_path, "wb");
    if (!file) {
        perror("File open failed");
        return;
    }

    char buffer[1024];
    int bytes_read;
    while ((bytes_read = read(new_socket, buffer, sizeof(buffer))) > 0) {
        fwrite(buffer, 1, bytes_read, file);
        client.used_space += bytes_read;

        if (client.used_space >= STORAGE_LIMIT) {
            send(new_socket, "$FAILURE$LOW_SPACE$", strlen("$FAILURE$LOW_SPACE$"), 0);
            break;
        }
    }

    fclose(file);
    send(new_socket, "$SUCCESS$", strlen("$SUCCESS$"), 0);
}
void handle_download(int new_socket, char *command, const char *username) {
    char *file_name = strtok(command, "$");
    file_name = strtok(NULL, "$");

    // Include the username in the directory path
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "./server_dir/%s/%s", username, file_name);

    // Try to open the file in read mode
    FILE *file = fopen(full_path, "rb");
    if (!file) {
        send(new_socket, "$FAILURE$FILE_NOT_FOUND$", strlen("$FAILURE$FILE_NOT_FOUND$"), 0);
        return;
    }

    // Send $SUCCESS$ to notify client that file transfer will begin
    send(new_socket, "$SUCCESS$", strlen("$SUCCESS$"), 0);

    // Introduce a small delay to allow the client to process the success message
    usleep(100000);  // 100ms delay to ensure the client reads $SUCCESS$ before file data

    char buffer[1024];
    int bytes_read;

    // Read the file and send it to the client in chunks
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(new_socket, buffer, bytes_read, 0);  // Send file content
    }

    fclose(file);
    shutdown(new_socket, SHUT_WR);  // Signal EOF and close the writing end of the socket
}


bool complete_database(const char* username) {
    printf("Searching for username: %s\n", username);
    FILE *file = fopen("database.txt", "r");  // Open in "r" mode for text reading
    if (file == NULL) {
        perror("Error opening file");
        return false;  // Return false as an indication of error
    }

    User users[MAX_ENTRIES];
    int count = 0;
    char line[1024];

    // Read each line from the file
    while (fgets(line, sizeof(line), file) != NULL && count < MAX_ENTRIES) {
        // Remove the newline character at the end of the line, if it exists
        line[strcspn(line, "\n")] = 0;

        // Split the line into username and password using ':' and ';'
        char *file_username = strtok(line, ":");  // Use a unique name for clarity
        char *password = strtok(NULL, ";");

        if (file_username && password) {
            strncpy(users[count].username, file_username, MAX_LENGTH - 1);
            users[count].username[MAX_LENGTH - 1] = '\0';  // Null-terminate
            strncpy(users[count].password, password, MAX_LENGTH - 1);
            users[count].password[MAX_LENGTH - 1] = '\0';  // Null-terminate
            count++;
        }
    }

    fclose(file);

    // Check if the specified username exists in the array
    for (int i = 0; i < count; i++) {
        printf("Username: %s, Password: %s\n", users[i].username, users[i].password);
        if (strcmp(username, users[i].username) == 0) {  // Use strcmp to compare strings
            return true;
        }
    }
    return false;
}

void add_user(const char* username, const char* password) {
    FILE *file = fopen("database.txt", "a");  // Open file in append mode
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    // Format the entry as a new line followed by [username]:[password];
    fprintf(file, "\n%s:%s;", username, password);  // Combine everything into one line
    printf("User added: %s\n", username);

    fclose(file);  // Close file to save changes
}
void create_user_directory(const char *username) {
    char dir_path[256];    // Buffer to store the directory path
    char file_path[270];   // Increased buffer size to accommodate "/semaphore.txt"

    // Assuming the server directory is "server_dir/"
    snprintf(dir_path, sizeof(dir_path), "server_dir/%s", username);

    // Create the directory with read/write/execute permissions for the owner
    if (mkdir(dir_path, 0700) == 0) {
        printf("Directory created: %s\n", dir_path);

        // Create the semaphore.txt file in the new directory
        snprintf(file_path, sizeof(file_path), "%s/semaphore.txt", dir_path);

        FILE *file = fopen(file_path, "w");
        if (file == NULL) {
            perror("Error creating semaphore.txt");
        } else {
            // Write "0;" three times in the file
            fprintf(file, "0;\n0;\n0;\n");

            // Close the file
            fclose(file);

            printf("File created: %s with content '0; 0; 0;'\n", file_path);
        }
    } else {
        perror("Error creating directory");
    }
}

void *client_handler(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);  // Free the allocated memory

    char getusername[1024] = {0};
    printf("The username is being stored\n");
    if (read(client_socket, getusername, sizeof(getusername)) > 0) {
        printf("%s\n", getusername);
    } else {
        perror("Error reading from socket");
    }
    bool userexist=complete_database(getusername);
    

    char getpassword[1024] = {0};
    printf("your password has been send\n");
    if (read(client_socket, getpassword, sizeof(getusername)) > 0) {
        printf("%s\n", getpassword);
    } else {
        perror("Error reading from socket");
    }

    if(!userexist){
        printf("Hello");
        add_user(getusername,getpassword);
        create_user_directory(getusername);
        
    }


    char command[1024] = {0};
    read(client_socket, command, sizeof(command));

    if (strncmp(command, "$UPLOAD$", 8) == 0) {
        handle_upload(client_socket, command,getusername);
    } else if (strncmp(command, "$DOWNLOAD$", 10) == 0) {
        handle_download(client_socket, command,getusername);
    } else if (strncmp(command, "$VIEW$", 6) == 0) {
        handle_view(client_socket,getusername);
    }

    close(client_socket);
    return NULL;
}


int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    client.used_space = 0;

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Server is listening on 127.0.0.1:%d\n", PORT);
    
    while (1) {
        // Accept a new client connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("Accept failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        // Create a thread to handle the new client
        pthread_t client_thread;
        int *client_socket = malloc(sizeof(int));
        *client_socket = new_socket;  // Pass client socket to thread
        if (pthread_create(&client_thread, NULL, client_handler, client_socket) != 0) {
            perror("Thread creation failed");
            free(client_socket);
        }

        // Detach the thread to clean up resources automatically when it finishes
        pthread_detach(client_thread);
    }

    close(server_fd);
    return 0;
}