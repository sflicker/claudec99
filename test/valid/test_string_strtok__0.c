#include <string.h>
int main(void) {
    char s[] = "one,two,three";
    char *tok = strtok(s, ",");
    if (!tok || strcmp(tok, "one") != 0) return 1;
    tok = strtok(0, ",");
    if (!tok || strcmp(tok, "two") != 0) return 1;
    tok = strtok(0, ",");
    if (!tok || strcmp(tok, "three") != 0) return 1;
    return strtok(0, ",") != 0;
}
