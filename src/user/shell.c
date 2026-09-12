#include "common.h"
#include "user.h"

void main(void) {
    while (1) {
    prompt:
        printf("> ");
        char cmdline[128];

        // 1. Read user input character by character
        int i = 0;
        while (1) {
            char ch = getchar();

            if (ch == '\r') {
                putchar(ch);
                printf("\n");
                cmdline[i] = '\0';
                break;
            } else if (ch == 127 || ch == '\b') {
                // Backspace/Delete: erase the last character, if any
                if (i > 0) {
                    i--;
                    printf("\b \b");
                }
            } else if (i == sizeof(cmdline) - 1) {
                printf("command line too long\n");
                goto prompt;
            } else {
                putchar(ch);
                cmdline[i++] = ch;
            }
        }

// Tokenizer: split cmdline into argv words
#define MAX_ARGS 16
        char *argv[MAX_ARGS];
        int argc = 0;

        char *p = cmdline;
        while (*p != '\0' && argc < MAX_ARGS) {
            // Skip leading spaces
            while (*p == ' ')
                p++;
            if (*p == '\0')
                break;

            argv[argc++] = p;

            // Advance to the end of the word
            while (*p != ' ' && *p != '\0')
                p++;

            // Null-terminate the word in place
            if (*p == ' ') {
                *p = '\0';
                p++;
            }
        }

        if (argc == 0) {
            // User just pressed Enter, do nothing and show prompt again
            continue;
        } else if (strcmp(argv[0], "hello") == 0) {
            printf("Hello world from shell!\n");
        } else if (strcmp(argv[0], "ls") == 0) {
            listfiles();
        } else if (strcmp(argv[0], "cat") == 0) {
            if (argc < 2) {
                printf("Usage: cat <filename>\n");
            } else {
                static char buf[1024];
                int len = readfile(argv[1], buf, sizeof(buf) - 1);

                if (len < 0) {
                    printf("File not found: %s\n", argv[1]);
                } else {
                    buf[len] = '\0'; // Ensure the string terminates safely
                    printf("%s\n", buf);
                }
            }
        } else if (strcmp(argv[0], "write") == 0) {
            if (argc < 3) {
                printf("Usage: write <filename> <content>\n");
            } else {
                // Re-join argv[2..] with single spaces; the tokenizer already
                // collapsed the original whitespace, so exact spacing isn't
                // preserved.
                static char content[512];
                int len = 0;

                for (int a = 2; a < argc && len < (int)sizeof(content) - 1;
                     a++) {
                    if (a > 2)
                        content[len++] = ' ';

                    for (int k = 0;
                         argv[a][k] != '\0' && len < (int)sizeof(content) - 1;
                         k++)
                        content[len++] = argv[a][k];
                }
                content[len] = '\0';

                int written = writefile(argv[1], content, len);

                if (written > 0) {
                    printf("Successfully wrote to %s\n", argv[1]);
                } else {
                    printf("Failed to write file.\n");
                }
            }
        } else if (strcmp(argv[0], "exit") == 0) {
            exit();
        } else {
            printf("unknown command: %s\n", argv[0]);
        }
    }
}