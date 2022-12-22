#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdbool.h>

#define SERVER_PORT 21
#define IP_MAX_SIZE 16
#define FILENAME_MAX_SIZE 200
#define INVALID_INPUT_ERROR "Usage: ftp://[<user>:<password>@]<host>/<url-path>\n"

static char filename[500];

struct Input
{
    char *user;
    char *password;
    char *host;
    char *urlPath;
};

int parseUrl(char *url, char **uph, char **urlPath) {
    char *token = strtok(url, "//");
    if (strcmp(token, "ftp:") != 0) {
        printf(INVALID_INPUT_ERROR);
        return -1;
    }
    *uph = strtok(NULL, "/");
    *urlPath = strtok(NULL, "\0");
    return 0;
}

void getFilename(char *urlPath, char *filename) {
    char aux[1000];
    strcpy(aux, urlPath);
    char *token1 = strtok(aux, "/");
    while (token1 != NULL) {
        strcpy(filename, token1);
        token1 = strtok(NULL, "/");
    }
}

int parseUserPasswordHost(char *uph, struct Input *input) {
    if (strstr(uph, "@") != NULL) {
        if (strstr(uph, ":") == NULL) {
            printf(INVALID_INPUT_ERROR);
            return -1;
        }
        input->user = strtok(uph, ":");
        if (strstr(input->user, "@") != NULL) {
            printf(INVALID_INPUT_ERROR);
            return -1;
        }
        uph = strtok(NULL, "\0");
        if (uph[0] != '@') {
            input->password = strtok(uph, "@");
            uph = strtok(NULL, "\0");
        } else {
            printf(INVALID_INPUT_ERROR);
            return -1;
        }
        input->host = strtok(uph, "\0");
    } else {
        input->user = "anonymous";
        input->password = "password";
        input->host = strtok(uph, "\0");
    }
    return 0;
}

int parseInput(int argc, char **argv, struct Input *input) {
    if (argc < 2) {
        printf("Error: Not enough arguments.\n");
        return -1;
    }

    // Parse input URL
    char *uph;
    if (parseUrl(argv[1], &uph, &input->urlPath) < 0) {
        return -1;
    }

    // Get filename from url path
    getFilename(input->urlPath, filename);

    // Parse user, password, and host
    return parseUserPasswordHost(uph, input);
}

int writeToSocket(int fd, const char *cmd, const char *info) {
    // Write command to socket
    if (write(fd, cmd, strlen(cmd)) != strlen(cmd)) {
        printf("Error writing command to socket\n");
        return -1;
    }

    // Write info to socket
    if (write(fd, info, strlen(info)) != strlen(info)) {
        printf("Error writing info to socket\n");
        return -1;
    }

    // Write newline to socket
    if (write(fd, "\n", strlen("\n")) != strlen("\n")) {
        printf("Error writing newline to socket\n");
        return -1;
    }

    return 0;
}

int readFromSocket(char *result, FILE *sock) {
    // Read the first three characters of the response
    int first = fgetc(sock);
    int second = fgetc(sock);
    int third = fgetc(sock);
    if (first == EOF || second == EOF || third == EOF) {
        return -1;
    }

    // Read the space character after the response code
    int space = fgetc(sock);
    if (space == EOF) {
        return -1;
    }

    // Read the rest of the response
    if (space != ' ') {
        do {
            printf("%s\n", fgets(result, FILENAME_MAX_SIZE, sock));
            if (result == NULL) {
                return -1;
            }
        } while (result[3] != ' ');
    } else {
        printf("%s\n", fgets(result, FILENAME_MAX_SIZE, sock));
    }

    // Convert response code to integer and return
    int zero = '0';
    return (first - zero) * 100 + (second - zero) * 10 + (third - zero);
}

int createSocket(const char *ip, int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    // Set up server address
    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    // Create TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket()");
        return -1;
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        return -1;
    }

    return sockfd;
}

int main(int argc, char **argv)
{
    struct Input input;
    if (parseInput(argc, argv, &input) < 0)
    {
        exit(-1);
    }

    struct hostent *h;
    if ((h = gethostbyname(input.host)) == NULL)
    {
        herror("gethostbyname()");
        exit(-1);
    }

    char ip[IP_MAX_SIZE];
    strcpy(ip, inet_ntoa(*((struct in_addr *)h->h_addr)));
    printf("IP Address : %s\n", ip);

    int control_connection_fd;
    if ((control_connection_fd = createSocket(ip, SERVER_PORT)) < 0)
    {
        return -1;
    }

    FILE *sockfile1 = fdopen(control_connection_fd, "r+");

    char result[500];
    int response_code;


    if ((response_code = readFromSocket(result, sockfile1)) < 0)
    {
        printf("Error\n");
        return -1;
    }

    if (response_code != 220)
    {
        printf("Bad response\n");
        return -1;
    }

    if (writeToSocket(control_connection_fd, "user ", input.user) != 0)
    {
        printf("Error writing to socket");
        return -1;
    }

    if ((response_code = readFromSocket(result, sockfile1)) < 0)
    {
        printf("Error\n");
        return -1;
    }

    if (response_code != 331)
    {
        printf("Bad response\n");
        return -1;
    }

    if (writeToSocket(control_connection_fd, "pass ", input.password) != 0)
    {
        printf("Error writing to socket");
        return -1;
    }

    if ((response_code = readFromSocket(result, sockfile1)) < 0)
    {
        printf("Error\n");
        return -1;
    }

    if (response_code != 230)
    {
        printf("Bad response\n");
        return -1;
    }

    if (writeToSocket(control_connection_fd, "pasv ", "") != 0)
    {
        printf("Error writing to socket");
        return -1;
    }

    if ((response_code = readFromSocket(result, sockfile1)) < 0)
    {
        printf("Error\n");
        return -1;
    }

    if (response_code != 227)
    {
        printf("Bad response\n");
        return -1;
    }

    int a, b, c, d, e, f;
    sscanf(result, "Entering Passive Mode (%d,%d,%d,%d,%d,%d).", &a, &b, &c, &d, &e, &f);

    int real_port = e * 256 + f;

    int data_connection_fd;

    if ((data_connection_fd = createSocket(ip, real_port)) < 0)
    {
        return -1;
    }

    FILE *sockfile2 = fdopen(data_connection_fd, "r+");

    if (writeToSocket(control_connection_fd, "retr ", input.urlPath) != 0)
    {
        printf("Error writing to socket");
        return -1;
    }

    if ((response_code = readFromSocket(result, sockfile1)) < 0)
    {
        printf("Error\n");
        return -1;
    }

    if (response_code != 150)
    {
        printf("Bad response\n");
        return -1;
    }

    FILE *myFile = fopen(filename, "w");

    int size;
    while (true)
    {
        unsigned char final_result[300];
        bool end;
        size = fread(final_result, 1, 300, sockfile2);
        if (size < 0)
            return -1;
        if (size < 300)
            end = true;
        size = fwrite(final_result, 1, size, myFile);
        if (size < 0)
            return -1;
        if (end)
            break;
    }

    if ((response_code = readFromSocket(result, sockfile1)) < 0)
    {
        printf("Error\n");
        return -1;
    }

    if (response_code != 226)
    {
        printf("Bad response\n");
        return -1;
    }

    if (close(data_connection_fd) < 0)
    {
        perror("close()");
        exit(-1);
    }

    if (close(control_connection_fd) < 0)
    {
        perror("close()");
        exit(-1);
    }
    return 0;
}