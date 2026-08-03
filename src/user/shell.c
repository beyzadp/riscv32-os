#include "common.h"
#include "user.h"

void main(void) {
    while (1) {
    prompt:
        printf("> ");
        char cmdline[128];

        // 1. Read user input character by character
        for (int i = 0;; i++) {
            char ch = getchar();
            putchar(ch);
            if (i == sizeof(cmdline) - 1) {
                printf("command line too long\n");
                goto prompt;
            } else if (ch == '\r') {
                printf("\n");
                cmdline[i] = '\0';
                break;
            } else {
                cmdline[i] = ch;
            }
        }

// 2. Tokenizer (The Parser)
// This chops the single cmdline string into an array of words (argv)
#define MAX_ARGS 16
        char *argv[MAX_ARGS];
        int argc = 0;

        char *p = cmdline;
        while (*p != '\0' && argc < MAX_ARGS) {
            // Skip any extra spaces before a word
            while (*p == ' ')
                p++;
            if (*p == '\0')
                break;

            // Save the starting pointer of the word into argv
            argv[argc++] = p;

            // Fast-forward to the end of the current word
            while (*p != ' ' && *p != '\0')
                p++;

            // If we hit a space, replace it with a null-terminator
            // This isolates the word so strcmp() works perfectly!
            if (*p == ' ') {
                *p = '\0';
                p++;
            }
        }

        // 3. Command Evaluator
        // Now we use argv[0] for the command, and argv[1] for the file!

        if (argc == 0) {
            // User just pressed Enter, do nothing and show prompt again
            continue;
        } else if (strcmp(argv[0], "hello") == 0) {
            printf("Hello world from shell!\n");
        } else if (strcmp(argv[0], "ls") == 0) {
            listfiles();
        } else if (strcmp(argv[0], "cat") == 0) {
            // Check if they forgot to type the filename (e.g., just typed
            // "cat")
            if (argc < 2) {
                printf("Usage: cat <filename>\n");
            } else {
                static char buf[1024]; // Read the file specified in argv[1]
                                       // (like "suvari.txt")
                int len = readfile(argv[1], buf, sizeof(buf) - 1);

                if (len < 0) {
                    printf("File not found: %s\n", argv[1]);
                } else {
                    buf[len] = '\0'; // Ensure the string terminates safely
                    printf("%s\n", buf);
                }
            }
        } /* else if (strcmp(argv[0], "write") == 0) {
             // Check if they forgot the filename
             if (argc < 2) {
                 printf("Usage: write <filename>\n");
             } else {
                 // Hardcoding the message for now to avoid complex quote
         parsing const char *msg = "This file was created entirely from inside
         the OS!\n"; int written = writefile(argv[1], msg, 51); // String is
         exactly 51 bytes

                 if (written > 0) {
                     printf("Successfully wrote to %s\n", argv[1]);
                 } else {
                     printf("Failed to write file.\n");
                 }
             }
         }*/
        else if (strcmp(argv[0], "exit") == 0) {
            exit();
        } else {
            printf("unknown command: %s\n", argv[0]);
        }
    }
}