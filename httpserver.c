#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define BUFFERSIZE 4096
#define PORT 3001

int sfd;
int cfd;
char redirect[BUFFERSIZE];

void mysighandler()
{
    close(cfd);
    close(sfd);
    puts("");
    exit(0);
}

void mysegfaulthandler()
{
    close(cfd);
    close(sfd);
    puts("segfault");
    abort();
}

void SignalInit()
{
struct sigaction sigIntHandler;
struct sigaction sigSegfaultHandler;

sigIntHandler.sa_handler = mysighandler;
sigSegfaultHandler.sa_handler = mysegfaulthandler;

sigemptyset(&sigIntHandler.sa_mask);
sigemptyset(&sigSegfaultHandler.sa_mask);

sigIntHandler.sa_flags = 0;
sigSegfaultHandler.sa_flags = 0;

sigaction(SIGINT, &sigIntHandler, NULL);
sigaction(SIGSEGV, &sigSegfaultHandler, NULL);
}

int ishtml(char* str)
{
    if (strlen(str) != 4)
        return 0;
    if (strcmp(str, "html"))
        return 0;
    return 1;
}

int gethandler(int cfd, char *filepath, char *msg)
{
    char *path = strtok(NULL, " ");
    char *protocol = strtok(NULL, "\n");
    char *file = NULL;
    int ret = 0;

    int protnum;

    if (protocol[strlen(protocol)] == '0')
        protnum = 0;
    else
        protnum = 1;

    //calculate length of the file path
    int pathlength = strlen(path) + strlen(filepath) + 1;
    if (path[strlen(path)-1] == '/')
        pathlength += strlen("index.html");

    //allocate memory for the file path
    file = malloc(pathlength);
    if (!file)
    { perror("!malloc"); return -1; }

    //combine path into one string
    if (path[strlen(path)-1] == '/')
        snprintf(file, pathlength, "%s%sindex.html", filepath, path);
    else
        snprintf(file, pathlength, "%s%s", filepath, path);

    //read full file requested into a buffer
    puts(file);
    char *buffer = 0;
    long length;
    FILE *f = fopen(file, "r");
    char tosend[BUFFERSIZE];

    if (f)
    {
        fseek(f, 0, SEEK_END);
        length = ftell (f);
        fseek(f, 0, SEEK_SET);
        buffer = malloc(length);
        if (buffer)
        {
            fread(buffer, 1, length, f);
        }
        fclose(f);
    }
    else
    {
        snprintf(tosend, BUFFERSIZE, "HTTP/1.%d 404 Not Found", protnum);
        send(cfd, tosend, strlen(tosend), 0);
        perror(file);
        free(file);
        return 0;
    }

    //get mime type
    char* filetype = file + pathlength;
    while (*filetype != '.')
        filetype--;
    filetype++;
    size_t l = 0;
    char* mimebuf = 0;
    char* mimetype = 0;
    FILE *mime = fopen("text.mime", "r");
    if (!mime) { perror("text.mime"); return 0; }
    while ((getline((char**)&mimebuf, &l, mime)) != -1)
    {
        if (!strncmp(mimebuf, filetype, strlen(filetype)))
        {
            mimetype = mimebuf + strlen(filetype) + 1;
            break;
        }
    }
    mimetype[strlen(mimetype) - 1] = 0;

    //send reply to get request
    if (buffer)
    {
        snprintf(tosend, BUFFERSIZE, "HTTP/1.%d 200 OK\nContent-Type: %s\nContent-Length: %d\n\n%s%c", protnum, mimetype, (int)length, buffer, 0);
        if (ishtml(filetype))
            sprintf(redirect, "HTTP/1.%d 301 Moved Permanently\nLocation: %s", protnum, path);
        send(cfd, tosend, strlen(tosend), 0);
    }
    else { ret = -1; }
    free(mimebuf);
    free(file);
    return ret;
}

int posthandler(int cfd, char* filepath, char* msg)
{
    send(cfd, redirect, strlen(redirect), 0);
    return 0;
}

int main(int argc, char **argv)
{
    SignalInit();
    char filepath[BUFFERSIZE];
    char msg[BUFFERSIZE];

    if (argc > 1)
    {
        strncpy(filepath, argv[1], strlen(argv[1]));
    }
    else
    {
        getcwd(filepath, sizeof(filepath));
    }

    struct sockaddr_in serverinfo = {0};
    serverinfo.sin_family = AF_INET;
    serverinfo.sin_port = htons(PORT);
    socklen_t serverinfolen = sizeof(serverinfo);

    struct sockaddr clientinfo = {0};
    socklen_t clientinfolen = sizeof(clientinfo);

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) { perror("!socket"); return -1; }

    if (0 > bind(sfd, (struct sockaddr*)&serverinfo, serverinfolen))
    { perror("!bind"); return -1; }

    while (1)
    {
        if (0 > listen(sfd, 0))
        { perror("!listen"); return -1; }

        cfd = accept(sfd, &clientinfo, &clientinfolen);
        if (0 > cfd) { perror("!accept"); return -1; }

        recv(cfd, msg, BUFFERSIZE-1, 0);

        char *request = strtok(msg, " ");

        if (strcmp(request, "GET") == 0)
        {
            if (0 > gethandler(cfd, filepath, msg + strlen(request) + 1))
            { perror("!gethandler"); }
        }
        else if (strcmp(request, "POST") == 0)
        {
            if (0 > posthandler(cfd, filepath, msg + strlen(request) + 1))
            { perror("!posthandler"); }
        }
        else if (strcmp(request, "HEAD") == 0)
        {}

        close(cfd);
        memset(msg, 0, 4096);
    }
    close(sfd);
    return 0;
}
