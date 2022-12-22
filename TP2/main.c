#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdbool.h>

#define IP_MAX_SIZE 16

#define SERVER_PORT 21

struct input
{
    char *user;
    char *password;
    char *host;
    char *urlPath;
};
static char filename[500];

int inputParser(int argc, char **argv, struct input *input)
{
    // ftp://[<user>:<password>@]<host>/<url-path>
    if (argc < 2)
    {
        printf("Not enought arguments\n");
        return -1;
    }

    char *token;
    /* get the first token */
    token = strtok(argv[1], "//");

    printf("%s : ftp\n", token);

    if (strcmp(token, "ftp:") != 0)
    {
        printf("Usage: ftp://[<user>:<password>@]<host>/<url-path>\n");
        return -1;
    }
    token = strtok(NULL, "\0");

    char *uph; // user password host
    uph = strtok(token, "/");
    printf("%s : uph\n", uph);

    // url
    input->urlPath = strtok(NULL, "\0");
    printf("%s : url\n", input->urlPath);

    char aux[1000];

    strcpy(aux, input->urlPath);
    char *token1 = strtok(aux, "/");

    while (token1 != NULL)
    {
        strcpy(filename, token1);
        token1 = strtok(NULL, "/");
    }

    // Optional Part
    if (strstr(uph, "@") != NULL)
    {
        if (strstr(uph, ":") == NULL)
        {
            printf("Usage: ftp://[<user>:<password>@]<host>/<url-path>\n");
            return -1;
        }

        // USER
        input->user = strtok(uph, ":");
        if (strstr(input->user, "@") != NULL)
        {
            printf("Usage: ftp://[<user>:<password>@]<host>/<url-path>\n");
            return -1;
        }
        printf("%s : user\n", input->user);
        uph = strtok(NULL, "\0");

        // PASSWORD
        if (uph[0] != '@')
        {
            input->password = strtok(uph, "@");
            printf("%s : password\n", input->password);
            uph = strtok(NULL, "\0");
        }
        else
        {
            printf("Usage: ftp://[<user>:<password>@]<host>/<url-path>\n");
            return -1;
        }

        // HOST
        input->host = strtok(uph, "\0");
        printf("%s : host\n", input->host);
    }
    else
    {
        // HOST
        input->user = "anonymous";
        input->password = "qualquer-password";
        input->host = strtok(uph, "\0");
        printf("%s : host\n", input->host);
    }

    return 0;
}

int write_to_socket(int fd, char *cmd, char *info)
{

    int res = 0;
    if ((res = write(fd, cmd, strlen(cmd))) != strlen(cmd))
    {
        printf("Error cmd");
        return -1;
    }
    if ((res = write(fd, info, strlen(info))) != strlen(info))
    {
        printf("Error info");
        return -1;
    }
    if ((res = write(fd, "\n", strlen("\n"))) != strlen("\n"))
    {
        printf("Error \n");
        return -1;
    }
    return 0;
}

int read_from_socket(char *result, FILE *sock)
{
    int first, second, third, space;
    first = fgetc(sock);
    second = fgetc(sock);
    third = fgetc(sock);

    if (first == EOF || second == EOF || third == EOF)
    {
        return -1;
    }
    // in case of single line
    if ((space = fgetc(sock)) == EOF)
    {
        return -1;
    }

    // in case of multiline
    if (space != ' ')
    {
        do
        {
            printf("%s\n", fgets(result, 200, sock));
            if (result == NULL)
            {
                return -1;
            }
        } while (result[3] != ' ');
    }
    else
    {
        printf("%s\n", fgets(result, 200, sock));
    }
    int zero = '0';
    return (first - zero) * 100 + (second - zero) * 10 + (third - zero);
}

int create_socket(char *ip, int port)
{
    int sockfd;
    struct sockaddr_in server_addr;

    /*server address handling*/
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        return -1;
    }

    /*connect to the server*/
    if (connect(sockfd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect()");
        return -1;
    }

    return sockfd;
}

int main(int argc, char **argv)
{
    struct input input;
    if (inputParser(argc, argv, &input) < 0)
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

    int sockfd1;
    if ((sockfd1 = create_socket(ip, SERVER_PORT)) < 0)
    {
        return -1;
    }

    FILE *sockfile1 = fdopen(sockfd1, "r+");

    char result[500];
    int response_code;

    // read welcome message
    if ((response_code = read_from_socket(result, sockfile1)) < 0)
    {
        printf("Error\n");
        return -1;
    }

    if (response_code != 220)
    {
        printf("Bad response\n");
        return -1;
    }

    // sending username
    if (write_to_socket(sockfd1, "user ", input.user) != 0)
    {
        printf("Error writing to socket");
        return -1;
    }

    if ((response_code = read_from_socket(result, sockfile1)) < 0)
    {
        printf("Error\n");
        return -1;
    }

    if (response_code != 331)
    {
        printf("Bad response\n");
        return -1;
    }

    // sending password
    if (write_to_socket(sockfd1, "pass ", input.password) != 0)
    {
        printf("Error writing to socket");
        return -1;
    }

    // answer from password
    if ((response_code = read_from_socket(result, sockfile1)) < 0)
    {
        printf("Error\n");
        return -1;
    }

    if (response_code != 230)
    {
        printf("Bad response\n");
        return -1;
    }

    // enter passive mode command
    if (write_to_socket(sockfd1, "pasv ", "") != 0)
    {
        printf("Error writing to socket");
        return -1;
    }

    // answer from pasv
    if ((response_code = read_from_socket(result, sockfile1)) < 0)
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

    int sockfd2;

    if ((sockfd2 = create_socket(ip, real_port)) < 0)
    {
        return -1;
    }

    FILE *sockfile2 = fdopen(sockfd2, "r+");

    // Enter retrieve command
    if (write_to_socket(sockfd1, "retr ", input.urlPath) != 0)
    {
        printf("Error writing to socket");
        return -1;
    }

    // Retrieve command answer
    if ((response_code = read_from_socket(result, sockfile1)) < 0)
    {
        printf("Error\n");
        return -1;
    }

    if (response_code != 150)
    {
        printf("Bad response\n");
        return -1;
    }

    // Create new file
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

    // Read transfer complete
    if ((response_code = read_from_socket(result, sockfile1)) < 0)
    {
        printf("Error\n");
        return -1;
    }

    if (response_code != 226)
    {
        printf("Bad response\n");
        return -1;
    }

    if (close(sockfd2) < 0)
    {
        perror("close()");
        exit(-1);
    }

    if (close(sockfd1) < 0)
    {
        perror("close()");
        exit(-1);
    }
    return 0;
    // ftp://netlab1.fe.up.pt
    // ftp.up.pt
}
