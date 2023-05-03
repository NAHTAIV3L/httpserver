#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int gethandler(int cfd, char *filepath, char *msg)
{
    char *part1 = strtok(msg, " ");
    char *path = strtok(NULL, " ");
    char *part3 = strtok(NULL, " ");
    char *file = NULL;

    int pathlength = strlen(path) + strlen(filepath);

    if (path[strlen(path)-1] == '/')
    {
        pathlength += strlen("index.html");
    }

    pathlength += 10;
    file = malloc(pathlength);
    if (!file)
    { perror("!malloc"); return -1; }


    if (path[strlen(path)-1] == '/')
    {
        snprintf(file, pathlength, "%s%sindex.html%c", filepath, path, 0);
    }
    else
    {
        snprintf(file, pathlength, "%s%s%c", filepath, path, 0);
    }

    perror((const char *)file);
    char * buffer = 0;
    long length;
    FILE * f = fopen(file, "r");

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
        char tosend[4096] = "HTTP/1.1 404 Not Found";
        send(cfd, tosend, 4096-1, 0);
        perror("!file not found");
        return -1;
    }

    if (buffer)
    {
        // start to process your data / extract strings here...
        char tosend[4096];
        snprintf(tosend, 4096, "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: %d\n\n%s", (int)length, buffer);
        send(cfd, tosend, 4096-1, 0);
    }
    else { return -1; }
    return 0;
}

int main(int argc, char **argv)
{
    char filepath[1024];
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
    serverinfo.sin_port = htons(3000);
    socklen_t serverinfolen = sizeof(serverinfo);

    struct sockaddr clientinfo = {0};
    socklen_t clientinfolen = sizeof(clientinfo);

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) { perror("!socket"); return -1; }

    if (0 > bind(sfd, (struct sockaddr*)&serverinfo, serverinfolen))
    {
        perror("!bind");
        return -1;
    }

    while (1)
    {
        if (0 > listen(sfd, 0))
        {
            perror("!listen");
            return -1;
        }

        int cfd = accept(sfd, &clientinfo, &clientinfolen);
        if (0 > cfd) { perror("!accept"); return -1; }

        char msg[4096];
        recv(cfd, msg, 4096-1, 0);

        char temp[4096];
        strncpy(temp, msg, 4096);

        if (0 > gethandler(cfd, filepath, temp))
        { perror("!gethandler"); }

        close(cfd);
    }
    return 0;
}
