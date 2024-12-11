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

int readResponse(const int socketfd, char* buffer, char* res)
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
                if (responsebyte == '-')
                {
                    status = NEW_LINE;
                }
                else if (responsebyte == ' ')
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
    if (getRegexMatch(buffer,REGEX_GET_FTP_RESPONSE,0,0,res) != 0)
    {
        return -1;
    }
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
    int socketA = createSocket(url.ip, FTP_PORT);
    char buffer[MAX_LENGTH_RES];
    char res[4];
    memset(res,0,4);
    if (readResponse(socketA, buffer, res) != 0)
    {
        perror("Error reading response not found");
        printf("%s",buffer);
        exit(-1);
    }
    if (atoi(res) != 220)
    {
        printf("%s",buffer);
        printf("Error reading response 220 expected and %s found\n", res);
        exit(-1);
    }
    return 0;
}