#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <netdb.h>
#include <netinet/in.h>
#include<arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define MAX_LENGTH  200
#define FTP_PORT    21

#define REGEX_BASE_URL_STRUCT   "^[a-zA-Z]+:\\/\\/[^ ]+$"
#define REGEX_HAS_USER          ":\\/\\/[^@: ]+:[^@ :/]*@"
#define REGEX_HOST_WITHOUT_USER ":\\/\\/[^ :@\\/]+"
#define REGEX_HOST_WITH_USER    "@[^\\/ :@]+"
#define REGEX_GET_USER_PASSWORD ":\\/\\/[^@/ :]+:[^@:/ ]*@"
#define REGEX_FILENAME          "\\/[^\\/ ]+$"
#define REGEX_PATH              "[^:\\/ ]\\/[^ ]+\\/"           //  "m/path/to/file/""




#define DEFAULT_USER "anonymous"
#define DEFAULT_PASSWORD ""

struct URL_RFC1738 {
    char host[MAX_LENGTH];
    char hostname[MAX_LENGTH];
    char path[MAX_LENGTH];
    char filename[MAX_LENGTH];
    char user[MAX_LENGTH];
    char password[MAX_LENGTH];
    char ip[MAX_LENGTH];
};
