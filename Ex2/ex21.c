// Noy Dvori 211908256

#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

// this function gets 2 files and returns 1 if they are equal, 3 if they are similar, else- 2.
int compare_files(int file1, int file2) {
    char ch1, ch2;
    bool error = false, similar = false;
    size_t readA = read(file1, &ch1, 1), readB = read(file2, &ch2, 1);
    while (readA && readB) {
        if (ch1 != ch2) {
            // ignoring case:
            if ( toupper(ch1) == toupper(ch2) ) {
                similar = true;
                readA = read(file1, &ch1, 1);
                readB = read(file2, &ch2, 1);
            }
            // in case of enter/whitespace for ch1:
            else if (ch1 == ' ' || ch1 == '\n') {
                similar = true;
                readA = read(file1, &ch1, 1);
            }
            // in case of enter/whitespace for ch2:
            else if (ch2 == ' ' || ch2 == '\n') {
                similar = true;
                readB = read(file2, &ch2, 1);
            } else {
                error = true;
                break;
            }
        } else {
            readA = read(file1, &ch1, 1);
            readB = read(file2, &ch2, 1);
        }
    }
    // if one file still have chars to read:
    while (readA) {
        if (ch1 != ' ' && ch1 != '\n') {
            error = true;
            break;
        }
        else {
            similar = true;
            readA = read(file1, &ch1, 1);
        }
    }
    while (readB) {
        if (ch2 != ' ' && ch2 != '\n') {
            error = true;
            break;
        }
        else {
            similar = true;
            readB = read(file2, &ch2, 1);
        }
    }
    if (error == true) {
        return 2;
    }
    if (similar == true) {
        return 3;
    }
    return 1;
}

int main(int argc, char* argv[]) {
    int res, file1, file2;
    if (argc != 3) {
        printf("Error : Not enough arguments");
        return 2;
    }
    file1 = open(argv[1], O_RDONLY);
    file2 = open(argv[2], O_RDONLY);
    if (file1 == -1 || file2 == -1) {
        printf("Error in : open");
        return 2;
    }
    res = compare_files(file1, file2);
    close(file1);
    close(file2);
    return res;
}
