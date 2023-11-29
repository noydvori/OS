// Noy Dvori 211908256

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>

void handle_alarm(int s) {}

// this function compiles and execute a given .c file with the provided input file and writes the output to the specified output file.
int compile_and_execute(char* c_file, int input_file, int student_output, int errors_file) {
    // compile the .c file using gcc:
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error in: fork");
        return 0;
    } else if (pid == 0) {
        dup2(errors_file, STDERR_FILENO);
        execlp("gcc", "gcc", c_file, "-o", "a.out", NULL);
        exit(1);
    } else {
        int status;
        wait(&status);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // compilation successful, execute the program
            pid_t pid2 = fork();
            if (pid2 == -1) {
                perror("Error in: fork");
                return 0;
            } else if (pid2 == 0) {
                lseek(input_file, 0, SEEK_SET);
                dup2(input_file, STDIN_FILENO);
                dup2(student_output, STDOUT_FILENO);
                dup2(errors_file, STDERR_FILENO);

                signal(SIGALRM,handle_alarm);
                alarm(5);

                execl("a.out", "a.out", NULL);
                exit(1);
            } else {
                wait(&status);
                alarm(0);

                // timeout:
                if(WIFSIGNALED(status)){
                    return 2;
                }

                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    // execution successful
                    return 1;
                } else {
                    // execution failed
                    return 0;
                }
            }
        } else {
            // compilation failed
            return 0;
        }
    }
}

// this function checks validity of given file.
int is_file(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

// this function checks validity of given folder.
int is_directory(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

// this function compares between the two files using ex21.c
int ret_status(char* correct_output_file, char* student_output) {
    // Compare the output with the correct output
    pid_t pid = fork();
    if (pid < 0) {
        perror("Error in: fork");
    }
    if (pid == 0) {
        execl("comp.out", "comp.out", student_output, correct_output_file, NULL);
        perror("Error in: exec");
    } else {
        int status;
        waitpid(pid,&status,0);
        if(WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
    }
    return -1;
}

// this function reads one line from the given file.
void readline(int fd, char *buf) {
    int i = 0;
    char c;
    while (read(fd, &c, 1) > 0) {
        if (c == '\n')
            break;
        buf[i++] = c;
    }
    buf[i] = '\0';
}


// MAIN FUNCTION:
int main(int argc, char* argv[]){

    // handle args error:
    if (argc != 2) {
        const char *err_msg = "Not a valid directory\n";
        write(1, err_msg, strlen(err_msg));
        exit(-1);
    }

    // get the 3 directories from configuration file:
    int fd = open(argv[1], O_RDONLY);
    char student_folder[151] = "", input_file[151] = "", correct_output_file[151] = "";
    readline(fd, student_folder);
    readline(fd, input_file);
    readline(fd, correct_output_file);
    close(fd);

    // check validity of the files:
    if (!is_directory(student_folder)) {
        const char *err_msg = "Not a valid directory\n";
        write(1, err_msg, strlen(err_msg));
        exit(-1);
    }
    if (!is_file(input_file)) {
        const char *err_msg = "Input file not exist\n";
        write(1, err_msg, strlen(err_msg));
        exit(-1);
    }
    if (!is_file(correct_output_file)) {
        const char *err_msg = "Output file not exist\n";
        write(1, err_msg, strlen(err_msg));
        exit(-1);
    }

    // create results.csv file
    int results = open("results.csv", O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
    if (results == -1) {
        perror("Error in: open");
    }

    // create error.txt file
    int errors = open("errors.txt", O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
    if (errors == -1) {
        perror("Error in: open");
    }

    // take input file identifier
    int input = open(input_file, O_RDWR);
    if (input == -1) {
        perror("Error in: open");
    }

    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(student_folder)) != NULL) {
        // iterate over each student folder:
        while ((ent = readdir(dir)) != NULL) {
            bool foundC = false; // .c file flag
            // ignore ../.
            if (ent->d_type == DT_DIR && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                char* student_dir = malloc(strlen(student_folder) + strlen(ent->d_name) + 2); // +2 for the '/' and the terminating null character
                sprintf(student_dir, "%s/%s", student_folder, ent->d_name);

                // open output file for student output:
                int output = open("output.txt", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
                if (output == -1) {
                    perror("Error in: open");
                }

                DIR* student_dir_ptr = opendir(student_dir);
                struct dirent* student_ent;
                // iterate over each file in the student directory
                while ((student_ent = readdir(student_dir_ptr)) != NULL) {
                    if (strcmp(student_ent->d_name + strlen(student_ent->d_name) - 2, ".c") == 0) {
                        // if the file ends with .c:
                        foundC = true; // turn on c flag
                        char* c_file = malloc(strlen(student_dir) + strlen(student_ent->d_name) + 2); // +2 for the '/' and the terminating null character
                        sprintf(c_file, "%s/%s", student_dir, student_ent->d_name);
                        int t = compile_and_execute(c_file, input, output, errors); // compile the current file
                        if (t == 0) {
                            // compilation error:
                            write(results, ent->d_name, strlen(ent->d_name));
                            write(results, ",10,COMPILATION_ERROR\n", strlen(",10,COMPILATION_ERROR\n"));
                            break;
                        }
                        if (t == 2) {
                            // timeout:
                            write(results, ent->d_name, strlen(ent->d_name));
                            write(results, ",20,TIMEOUT\n", strlen(",20,TIMEOUT\n"));
                            break;
                        }
                        // execute ex21.c on the files:
                        int exit_status= ret_status(correct_output_file,"output.txt");
                        // add scores according to ex21.c output:
                        switch (exit_status) {
                            case 1:
                                // excellent:
                                write(results, ent->d_name, strlen(ent->d_name));
                                write(results, ",100,EXCELLENT\n", strlen(",100,EXCELLENT\n"));
                                break;
                            case 2:
                                // wrong:
                                write(results, ent->d_name, strlen(ent->d_name));
                                write(results, ",50,WRONG\n", strlen(",50,WRONG\n"));
                                break;
                            case 3:
                                // similar:
                                write(results, ent->d_name, strlen(ent->d_name));
                                write(results, ",75,SIMILAR\n", strlen(",75,SIMILAR\n"));
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                }
                // close student folder.
                closedir(student_dir_ptr);

                // close output file.
                if (close(output)) {
                    perror("Error in: close");
                }

                // remove output.txt
                if (remove("output.txt")) {
                    perror("Error in: remove");
                    return -1; // This will be a problem
                }

                // if there's no .c file:
                if (!foundC) {
                    write(results, ent->d_name, strlen(ent->d_name));
                    write(results, ",0,NO_C_FILE\n", strlen(",0,NO_C_FILE\n"));
                }
            }
        }
        // close students' folder.
        closedir(dir);

        // close input file.
        if (close(input)) {
            perror("Error in: close");
        }

        // close errors file.
        if (close(errors)) {
            perror("Error in: close");
        }
    }
    return 0;
}