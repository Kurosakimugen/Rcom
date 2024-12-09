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

int getRegexMatch(char* inputURL, char* regexFormula,int startOffset,int endOffset, char* res)
{
    regex_t regex;
    regmatch_t match[1];
    int start,end;
    if (regcomp(&regex, regexFormula, REG_EXTENDED) != 0) return -1;
    if (regexec(&regex, inputURL, 1, match,0) != 0) return -1;
    start   = match[0].rm_so;
    end     = match[0].rm_eo;
    strncpy(res, inputURL + start + startOffset, end - start - startOffset - endOffset);
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

int main()
{
    struct URL_RFC1738 url; 
    int res = parseURL("http://website.com/path/to/dest/res", &url);
    if (res != 0)
    {
        perror("URL in wrong format");
        exit(-1);
    }
    printf("User: %s\n", url.user);
    printf("Password: %s\n", url.password);
    printf("Host: %s\n", url.host);
    printf("Path: %s\n", url.path);
    printf("Filename: %s\n", url.filename);


    printf("%i\n",res);
    return 0;
}