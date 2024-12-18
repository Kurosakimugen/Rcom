#include "download.h"

int getUser_Pass(char* inputURL, struct URL_RFC1738 *url)
{
    regex_t regex;
    regmatch_t match[1];
    int start, end;

    if (regcomp(&regex, REGEX_GET_USER_PASSWORD , REG_EXTENDED) != 0)
    {
        perror("Error parsing URL USER:PASSWORD");
        exit(-1);
    }
    if (regexec(&regex, inputURL, 1,match,0) != 0) return -1;
    start   = match[0].rm_so;
    end     = match[0].rm_eo;
    char user_password[MAX_LENGTH]; 
    strncpy(user_password, inputURL + start, end - start);
    user_password[end - start] = '\0';
    char* token = strtok(user_password, ":");
    strcpy(url->user, token+2);
    token = strtok(NULL, ":");
    if (token == NULL)
        strcpy(url->password,"");
    else
        token[strlen(token)-1] = '\0';
        strcpy(url->password,token);

    return 0;
}
int getRegexMatch(char* input, char* regexFormula,int startOffset,int endOffset, char* res)
{
    regex_t regex;
    regmatch_t match[1];
    int start,end;
    if (regcomp(&regex, regexFormula, REG_EXTENDED) != 0) return -1;
    if (regexec(&regex, input, 1, match,0) != 0) return -1;
    start   = match[0].rm_so;
    end     = match[0].rm_eo;
    strncpy(res, input + start + startOffset, end - start - startOffset - endOffset);
    res[end - start - startOffset - endOffset] = '\0';
    return 0;
    
}
int parseURL(char* inputURL, struct URL_RFC1738 *url)
{
    regex_t regex;

    regcomp(&regex, REGEX_BASE_URL_STRUCT, REG_EXTENDED);
    if (regexec(&regex, inputURL, 0, NULL, 0) != 0) return -1;

    regcomp(&regex, REGEX_HAS_USER, REG_EXTENDED);

    if (regexec(&regex, inputURL, 0, NULL, 0) == 0) //Has Login info
    {
        if (getUser_Pass(inputURL, url) != 0)
        {
            perror("Wrong User:Password Formating");
            exit(-1);
        }
        getRegexMatch(inputURL,REGEX_HOST_WITH_USER,1,0,url->host);
    }
    else                                            //Default user and Password
    {
        strcpy(url->user, DEFAULT_USER);
        strcpy(url->password, DEFAULT_PASSWORD);
        regcomp(&regex, REGEX_HOST_WITHOUT_USER, REG_EXTENDED);
        getRegexMatch(inputURL,REGEX_HOST_WITHOUT_USER,3,0,url->host);
    }

    if (getRegexMatch(inputURL,REGEX_FILENAME,1,0,url->filename) != 0) return -1;
    if (getRegexMatch(inputURL,REGEX_PATH,2,0,url->path) != 0)
    {
        strcpy(url->path, ""); 
    }

    return 0;
}
void buildURL(char* address, struct URL_RFC1738 *url)
{
    struct hostent *h;
    if (parseURL(address, url) != 0)
    {
        perror("URL in wrong format");
        exit(-1);
    }
    if ((h = gethostbyname(url->host)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }
    strcpy(url->hostname,h->h_name);
    strcpy(url->ip,inet_ntoa(*((struct in_addr *) h->h_addr)));
}

int createSocket(char* ip, int port)
{
    int socket_fd;
    struct sockaddr_in socketAddress;

    memset(&socketAddress,0,sizeof(socketAddress));
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &(socketAddress.sin_addr)) > 1)  //Error or ip messedup
    {
        perror("Error: inet_pton()");
        exit(-1);
    }
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, getprotobyname("tcp")->p_proto)) < 0)
    {
        perror("Error creating socket: socket()");
        exit(-1);
    }
    if  (connect(socket_fd, (struct sockaddr *) &socketAddress ,sizeof(socketAddress)) < 0)
    {
        perror("Error Conecting socket: connect()");
        exit(-1);
    }

    return socket_fd;
}

int readResponse(const int socketfd, char* buffer, int* res)
{
    memset(buffer,0,MAX_LENGTH_RES);
    char responsebyte; 
    int  responsecode;
    int  index = 0;
    readResponseStatus status = START;
    memset(buffer, 0, MAX_LENGTH);
    while (status != END)
    {
        recv(socketfd, &responsebyte, 1, 0);
        switch (status)
        {
            case START:
                buffer[index++] = responsebyte;
                if (responsebyte >= '0' && responsebyte <= '9')
                {
                    status=SECOND_NUM;
                }
                else if(responsebyte == '\r')
                {
                    status = NEW_LINE_CR;
                }
                else
                {
                    status = NEW_LINE;
                }
                break;
            case SECOND_NUM:
                buffer[index++] = responsebyte;
                if (responsebyte >= '0' && responsebyte <= '9')
                {
                    status=THIRD_NUM;
                }
                else if(responsebyte == '\r')
                {
                    status = NEW_LINE_CR;
                }
                else
                {
                    status = NEW_LINE;
                }
                break;
            case THIRD_NUM:
                buffer[index++] = responsebyte;
                if (responsebyte == '-')
                {
                    status = NEW_LINE;
                }
                else if(responsebyte == '\r')
                {
                    status = FINAL_LINE_CR;
                }
                else if(responsebyte == ' ')
                {
                    status = FINAL_LINE;
                }
                break;
            case NEW_LINE:
                if (responsebyte == '\r')
                {
                    status = NEW_LINE_CR;
                }
                else
                {
                    buffer[index++] = responsebyte;
                }
                break;
            case NEW_LINE_CR:
                if (responsebyte == '\n' )
                {
                    buffer[index++] = responsebyte;
                    status = START;
                }
                else
                {
                    status = NEW_LINE;
                }
                break;

            case FINAL_LINE:
                if (responsebyte == '\r')
                {
                    status = FINAL_LINE_CR;
                }
                else
                {
                    buffer[index++] = responsebyte;
                }
                break;
            case FINAL_LINE_CR:
                if (responsebyte == '\n')
                {
                    buffer[index++] = responsebyte;
                    status = END;
                    break;
                }
                else
                {
                    status = FINAL_LINE;
                }
                break;
            case END:
                break;
            default:
                break;
        }
    }
    printf("Response:\t%s\n",buffer);
    char stringRes[4] = {0}; 
    if (getRegexMatch(buffer,REGEX_GET_FTP_RESPONSE,0,0,stringRes) != 0)
    {
        return -1;
    }
    *res = atoi(stringRes);
    return 0;    
}

int authenticate(const int socketfd, const char* user, const char* pass)
{
    char message[MAX_LENGTH] = {0};
    char response[MAX_LENGTH_RES] = {0};
    int responseCode = 0;
    snprintf(message, MAX_LENGTH,"USER %s\r\n",user);
    if (send(socketfd, message, strlen(message), 0) < 0)
    {
        perror("Error sending USER command");
        return -1;
    }
    if (readResponse(socketfd, response, &responseCode) != 0)
    {
        printf("%s",message);
        perror("Error during User readResponse found no response");
        exit(-1);
        return -1;
    }
    if (responseCode != 331)
    {
        printf("%s",response);
        printf("Error authenticating user 331 expected and %i found\n", responseCode);
        exit(-1);
    }

    snprintf(message, MAX_LENGTH,"PASS %s\r\n",pass);
    send(socketfd, message, strlen(message), 0);
    if (readResponse(socketfd, response, &responseCode) != 0)
    {
        printf("%s",message);
        perror("Error writing password readResponse found no response");
        exit(-1);
    }
    if (responseCode != 230)
    {
        printf("%s",response);
        printf("Error authenticating user 230 expected and %i found\n", responseCode);
        exit(-1);
    }
    return 0;
}

int binMode(const int socketfd)
{
    char message[MAX_LENGTH] = {0};
    char response[MAX_LENGTH_RES] = {0};
    int responseCode = 0;
    snprintf(message, 9, "TYPE I\r\n");
    send(socketfd, message, strlen(message), 0);
    if (readResponse(socketfd, response, &responseCode) != 0)
    {
        printf("%s",message);
        perror("Error Entering Bin mode readResponse found no response");
        exit(-1);
    }
    if (responseCode != 200)
    {
        printf("%s",response);
        printf("Error entering bun mode 200 expected and %i found\n", responseCode);
        exit(-1);
    }
    return 0;
}

int passiveMode(const int socketfd, char *ip, int *port)
{
    char response[MAX_LENGTH_RES];
    int responseCode = 0;
    send(socketfd, "PASV\r\n" , 6, 0);
    if (readResponse(socketfd, response, &responseCode) != 0)
    {
        printf("%s",response);
        perror("Error entering PASV mode found no response");
        exit(-1);
    }
    if (responseCode != 227)
    {
        printf("%s",response);
        printf("Error Entering PASV mode 227 expected and %i found\n", responseCode);
        exit(-1);
    }

     
    char ip_port[MAX_LENGTH];
    getRegexMatch(response, REGEX_GET_IP_PORT, 1, 1, ip_port);
    int serverIp1, serverIp2, serverIp3, serverIp4; 
    int serverPort1, serverPort2;

    char* token = strtok(ip_port, ",");
    serverIp1 = atoi(token);

    token = strtok(NULL, ",");
    serverIp2 = atoi(token);

    token = strtok(NULL, ",");
    serverIp3 = atoi(token);

    token = strtok(NULL, ",");
    serverIp4 = atoi(token);

    token = strtok(NULL, ",");
    serverPort1 = atoi(token);

    token = strtok(NULL, ",");
    serverPort2 = atoi(token);

    snprintf(ip, MAX_LENGTH,"%d.%d.%d.%d",serverIp1,serverIp2,serverIp3,serverIp4);
    *port = 256 * serverPort1 + serverPort2;
    return 0;
}

int requestResource(const int socketfd, char* path, char* filename)
{
    char message[MAX_LENGTH];
    snprintf(message, MAX_LENGTH,"RETR %s/%s\r\n",path,filename);
    printf("MES %s\n",message);
    send(socketfd, message, strlen(message), 0);

    char response[MAX_LENGTH_RES] = {0};
    int responseCode = 0;
    if (readResponse(socketfd, response, &responseCode) != 0)
    {
        printf("%s",message);
        perror("Error writing resource to fetch\n");
        exit(-1);
    }
    if (responseCode != 150 && responseCode != 125 )
    {
        printf("%s",response);
        printf("Error requesting resource 230 expected and %i found\n", responseCode);
        exit(-1);
    }
    return 0;
}

int getResource(const int socketfd1, const int socketfd2, char* filename)
{
    FILE *file = fopen(filename,"wb");
    if (file == NULL)
    {
        printf("Error creating local file \n");
        exit(-1);
    }

    char buffer[MAX_LENGTH];
    int readBytes;
    while (readBytes = recv(socketfd2,buffer, MAX_LENGTH, 0))
    {
        if (fwrite(buffer, readBytes, 1, file) < 0 )
            return -1;
    }
    fclose(file);
    
    char response[MAX_LENGTH_RES] = {0};
    int responseCode = 0;
    if (readResponse(socketfd1, response, &responseCode) != 0)
    {
        perror("Error while getting file\n");
        exit(-1);
    }
    if (responseCode != 226)
    {
        printf("%s",response);
        printf("Error getting resource 226 expected and %i found\n", responseCode);
        exit(-1);
    }
    close(socketfd2);
    return 0;
}

int closeConection(const int socketfd1,const int socketfd2)
{
    char response[MAX_LENGTH];
    int responseCode = 0;
    send(socketfd1, "QUIT\r\n", 7, 0);

    if (readResponse(socketfd1,response,&responseCode) != 0)
    {
        perror("Error Ending conectrion");
    }
    if (responseCode != 221)
    {
        return -1;
    }
    close(socketfd2);
    close(socketfd1);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <address to get IP address>\n", argv[0]);
        exit(-1);
    }
    struct URL_RFC1738 url;
    buildURL(argv[1], &url);
    int socket1 = createSocket(url.ip, FTP_PORT);
    char buffer[MAX_LENGTH_RES];
    int res = 0;
    if (readResponse(socket1, buffer, &res) != 0)
    {
        perror("Error reading response not found");
        printf("%s",buffer);
        exit(-1);
    }
    if (res != 220)
    {
        printf("%s",buffer);
        printf("Error reading response 220 expected and %d found\n", res);
        exit(-1);
    }
    if (authenticate(socket1,url.user,url.password) != 0)
    {
        perror("Error authenticating\n");
        exit(-1);
    }
    char ip[MAX_LENGTH] = {0};
    int port = 0;
    if (binMode(socket1) != 0)
    {
        exit(-1);
    }
    passiveMode(socket1,ip,&port);
    int socket2 = createSocket(ip,port);
    if (socket2 < 0)
    {
        perror("Error creating second socket");
        return -1;
    }
    requestResource(socket1, url.path, url.filename);

    getResource(socket1,socket2,url.filename);
    close(socket1);
    return 0;
}
