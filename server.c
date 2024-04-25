#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>

#define BUFER_SIZE 256
#define BUFER_1MB 1000000
#define BACK_LOG_NUM 10 
#define EMPTY_CHAR ' '
#define DEFAULT_PORT 8080 
#define MAX_ERROR_MESSAGES 4

#define ERROR_BINDING_SOCKET 0
#define ERROR_LISTENING 1
#define MEMORY_ALLOC_ERROR 2
#define ERROR_WHILE_ACCEPTING_CLIENT 3

char *error_messages[ MAX_ERROR_MESSAGES ] = 
{
    "Error while biding the socket to the server address", 
    "Error while listening to the port", 
    "Memory allocation failed!\n", 
    "There was an error while accepting the client" 
};

char SPACE = ' '; 

const char *get_file_extension(const char *file_name) {
    const char *dot = strrchr(file_name, '.');
    if (!dot || dot == file_name) {
        return "";
    }
    return dot + 1;
}

char *get_file_name( const char *src) {

    size_t src_len = strlen( src );
    char *decoded = malloc( src_len + 1 );
    size_t current_char = 0;

    for (size_t i = 0; i < src_len; i++) {

        // Decode special characters 
        if (src[i] == '%' && i + 2 < src_len) {

            int hex_val;
            sscanf(src + i + 1, "%2x", &hex_val);
            decoded[ current_char++ ] = hex_val;
            i += 2;

        } else {
            decoded[ current_char++ ] = src[i];
        }

    }

    // add null terminator
    decoded[ current_char ] = '\0';
    return decoded;

}



const char *create_Content_Type(const char *file_ext) {

    if (strcasecmp(file_ext, "html") == 0 || strcasecmp(file_ext, "htm") == 0) { return "text/html"; } 
    else if (strcasecmp(file_ext, "txt") == 0) { return "text/plain"; } 
    else if (strcasecmp(file_ext, "jpg") == 0 || strcasecmp(file_ext, "jpeg") == 0) { return "image/jpeg"; } 
    else if (strcasecmp(file_ext, "png") == 0) { return "image/png"; } 
    else if (strcasecmp(file_ext, "js") == 0) { return "text/javascript"; } 
    else if (strcasecmp(file_ext, "css") == 0) { return "text/css"; } 
    else { return "application/octet-stream"; }

}

void create_http_response(const char *file_name,  const char *file_ext,  char *response, size_t *response_len) {
    
    // Add 200 OK to HTTP Header 
    const char *mime_type = create_Content_Type(file_ext);
    char *header = (char *) malloc(BUFER_1MB * sizeof(char));
    snprintf(header, BUFER_1MB,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "\r\n",
             mime_type);

    // Respond with 404 not found if file does not exist
    int file_fd = open(file_name, O_RDONLY);
    
    if (file_fd == -1) {
        snprintf(response, BUFER_1MB,
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: text/plain\r\n"
                 "\r\n"
                 "404 Not Found");
        *response_len = strlen(response);
        return;
    }

    // Content length
    struct stat file_metadata;
    fstat( file_fd, &file_metadata ); 
    off_t file_size = file_metadata.st_size; 

    if( file_size > BUFER_1MB ){
        snprintf(response, BUFER_1MB,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "404 Not Found");
        *response_len = strlen(response);
        return;
    }

    // Add the headers to the response and update the response length 
    *response_len = 0;
    memcpy(response, header, strlen(header));
    *response_len += strlen(header);

    // Add file bytes to response 
    ssize_t bytes_read;
    while (( bytes_read = read(file_fd, response + *response_len, BUFER_1MB - *response_len) ) > 0) {
        *response_len += bytes_read;
    }

    free(header);
    close(file_fd);

}

void *handle_client( void *args){
    
    // sizeof( char ) it should be 1 but it is done like this to ensure portability between systems 

    int *client_fd_ptr = (int*) args;
    int client_fd = *client_fd_ptr; 
    char *buffer = ( char * ) malloc( BUFER_1MB * sizeof( char )); 
    ssize_t bytes_recived = recv( client_fd, buffer, BUFER_1MB, 0 ); 

    if( bytes_recived > 0 ){

        regex_t regex;
        regcomp(&regex, "^GET /([^ ]*) HTTP/1", REG_EXTENDED);
        regmatch_t matches[2];

        if ( regexec(&regex, buffer, 2, matches, 0) == 0 ) {
            
            buffer[ matches[1].rm_eo ] = '\0';
            const char *encoded_file_name = buffer + matches[1].rm_so;
            char *file_name = get_file_name( encoded_file_name );

            char file_ext[32];
            strcpy(file_ext, get_file_extension(file_name));

            // sizeof( char ) should be 1  but it is done like for portability reasons 
            char *response = (char *) malloc( BUFER_1MB * 2 * sizeof( char ) );
            size_t response_len;
            create_http_response( file_name, file_ext, response, &response_len );


            // send response 
            send(client_fd, response, response_len, 0);

            free(response);
            free(file_name);

        }

        regfree(&regex);
    }

    close( client_fd ); 
    free( buffer );
    free( client_fd_ptr );

    return NULL; 

}


int main(){

    // Set up server to receive client connections

    char buffer[ BUFER_1MB ] = { 0 }; 
    int server_socket = socket( AF_INET, SOCK_STREAM, 0 );


    if (server_socket == -1) {
        perror( error_messages[ ERROR_BINDING_SOCKET ]); 
        exit(1);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0){
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1); 
    }


    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons( DEFAULT_PORT ); 
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if( bind(server_socket,   (struct sockaddr *)  &server_addr, sizeof( server_addr ))  < 0  ){

        perror( error_messages[ ERROR_BINDING_SOCKET ] );
        close(server_socket);
        exit(1); 

    }     

    if( listen(server_socket, BACK_LOG_NUM) < 0  ){

        perror( error_messages[ ERROR_LISTENING ] );
        close(server_socket);
        exit(1); 

    }

    // Handle client connections 
    while ( 1 ) {

        struct sockaddr_in client_addr; 
        socklen_t client_addr_len = sizeof( client_addr ); 
        int client_fd = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

        if( client_fd < 0 ){

            perror( error_messages[ ERROR_WHILE_ACCEPTING_CLIENT ] );
            continue;
        }

        pthread_t p_id; 
        int *client_fd_ptr = malloc(sizeof(int));
        *client_fd_ptr = client_fd; 
        pthread_create( &p_id, NULL, handle_client, (void * ) client_fd_ptr ); 
        pthread_detach( p_id );

    }

    return 0;
}

