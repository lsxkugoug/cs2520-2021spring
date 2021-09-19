#include <string.h>
main(){
    char s[] = "filename@ip:1234";
    char *delim = "@:";
    char *p;
    printf("%s\n", strtok(s, delim));
    printf("%s\n", strtok(NULL, delim));
    printf("%s\n", strtok(NULL, delim));
}