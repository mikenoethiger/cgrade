#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "libs/ctoolbox.c"

/* Interface:
 * cgrade init                      // init the cgrade.csv in working directory
 * cgrade add algd2 5.25 "test 1"   // add grade to algd2 with comment 
 * cgrade status                    // show status for all subjects
 * cgrade status algd2              // show status for algd2 
 * cgrade rm                        // removes last added grade
 * cgrade rm algd2 5.25             // remove grade in algd2 that is 5.25 (equivalent to `rm algd2 5.25 0`)
 * cgrade rm algd2 5.25 1           // remove the 2nd grade in algd2 that is 5.25
 *
 * Improvements Pending:
 * - Instead of reading and writing the csv file manually whenever some
 *   change is required, abstract the file by providing a data structure
 *   that can hold grades and then implement a function that reads from
 *   the csv file into the data structure and a function that writes the
 *   data structure into the csv file. This way, the data can be manipulated
 *   on a C level rather than on OS based read/write operations.
 */

#define APP_NAME "cgrade"
#define DEFAULT_CSV_NAME "cgrade.csv"
#define TMP_CSV_PATH "./tmp_cgrade.csv"
#define CSV_HEADER "subject;grade;comment"
#define CSV_DELIMITER ';'

#define CMD_NAME_INIT "init"
#define CMD_NAME_ADD "add"
#define CMD_NAME_STATUS "status"
#define CMD_NAME_RM "rm"

#define CMD_USAGE_ADD "Add a new grade"
#define CMD_USAGE_STATUS "Show grade stats"

#define OPT_NAME_HELP "--help"
#define OPT_NAME_CSV "--csv"

#define OPT_USAGE_CSV "Path to the csv file that contains the grades"

char *opt_default_csv = "./" DEFAULT_CSV_NAME;

typedef struct subject {
    char *name;
    float *grades;
    int n_grades;
} Subject;

Subject subject_create(char* name) {
    Subject s;
    s.name = name;
    s.grades = malloc(0);
    s.n_grades = 0;
    return s;
}

void subject_insert_grade(Subject *subject, float grade) {
    subject->n_grades++;
    subject->grades = (float *) realloc(subject->grades, sizeof(float) * subject->n_grades);
    subject->grades[subject->n_grades-1] = grade;
}

void exit_usage(int exit_code) {
    printf("\nUsage: %s COMMAND\n", APP_NAME);
    printf("\n");
    printf("A lightweight tool to manage school grades\n");
    printf("\n");
    printf("Commands:\n");
    printf("\tadd \t%s\n", CMD_USAGE_ADD);
    printf("\tstatus \t%s\n", CMD_USAGE_STATUS);
    printf("\n");
    printf("Run '%s COMMAND --help' for more information on a command.\n", APP_NAME);
    exit(exit_code);
}

void exit_unknown_option(char *option) {
    printf("Unknown option %s\n", option);
    exit(1);
}

void exit_no_csv() {
    printf("%s does not exist. Init a new .csv file with:\n\t%s %s\n", opt_default_csv, APP_NAME, CMD_NAME_INIT);
    exit(1);
}

void exit_unknown_cmd(char *cmd) {
    printf("Unknown command '%s'\n", cmd);
    exit(1);
}

void exit_csv_already_initialized() {
    printf("%s already initialized.\n", DEFAULT_CSV_NAME);
    exit(1);
}

void command_add_usage(int exit_code) {
    printf("\nUsage: %s add SUBJECT GRADE [COMMENT]\n", APP_NAME);
    printf("\n");
    printf("%s\n", CMD_USAGE_STATUS);
    printf("\n");
    printf("Examples:\n");
    printf("\t%s add math 5.2\n", APP_NAME);
    printf("\t%s add math 5.5 \"second exam\"\n", APP_NAME);
    printf("\n");
    exit(exit_code);
}

void command_status_usage(int exit_code) {

}

bool csv_exists() {
    struct stat st;
    return stat(opt_default_csv, &st) != -1;
}

/**
 * Ensures that the csv file exists.
 */
void csv_must_exist() {
    struct stat st;
    if (!csv_exists()) {
        exit_no_csv();
    }
}

/**
 * writes a \0 terminated string to an fd or exits
 * if write did not work
 */
ssize_t csv_write_string(int fd, char *s) {
    ssize_t b = write(fd, s, sizeof(*s) * strlen(s));
    if (b == -1) {
        printerrno("write failed");
    }
    return b;
}

void csv_init_file() {
    int oflag = O_WRONLY | O_CREAT | O_EXCL | O_APPEND;
    int fd = open(opt_default_csv, oflag);
    if (fd == -1) printerrno("init csv failed");
    csv_write_string(fd, "subject;grade;comment");
    csv_write_string(fd, "\n");
    if (close(fd) == -1) printerrno(NULL);
}



size_t csv_move_to_next_line(int csv_fd) {
    char buf;
    size_t s = 0; 
    ssize_t r;
    while ((r = read(csv_fd, &buf, sizeof(buf)))) { 
        if (r == -1) printerrno("csv_move_to_next_line read failed");
        s += r;
        if (buf == '\n') break;
    }
    return s;
}

/**
 * Read char squence up to the next occurence of terminator.
 * Allocates memory on the heap to store the char sequence.
 * arg(csv_fd) the csv file descriptor
 * arg(terminator) the terminator char
 * returns char sequnce up to terminator (excluding terminator)
 */
char *csv_read_to_next(int csv_fd, char terminator) {
    char *out = malloc(sizeof(char));
    *out = '\0';
    int out_size = 0;
    char buf;
    ssize_t r;
    while ((r = read(csv_fd, &buf, sizeof(buf))) && buf != terminator) {
        if (r == -1) printerrno("csv_read_to_next read failed");
        out_size++;
        out = (char *) realloc(out, sizeof(char) * (out_size+1));
        out[out_size-1] = buf;
        out[out_size] = '\0';
    }
    return out;
}


/**
 * Get float array with all grades.
 * arg(csv_fd) the csv file fd
 * arg(*o_size) an output parameter, containing the size of the output array 
 * returns a float array containing all grades
 */
float *csv_grades(int csv_fd, int *o_size) {
    float *out = malloc(0);
    *o_size = 0;
    char *s; // subject
    char *g; // grade
    while (csv_move_to_next_line(csv_fd) && *(s = csv_read_to_next(csv_fd, ';'))) {
        g = csv_read_to_next(csv_fd, ';');
        out = (float *) realloc(out, sizeof(float) * (*o_size+1));
        out[*o_size] = strtof(g, NULL);
        (*o_size)++;
        free(g);
        free(s);
    } 
    return out;
}

/**
 * Get float array with all grades of a subject.
 * arg(csv_fd) the csv file fd
 * arg(*subject) the target subject
 * arg(*o_size) an output parameter, containing the size 
 * of the returned array.
 * returns a float array containing all grades
 * of a subject. 
 */
float *csv_subject_grades(int csv_fd, char *subject, int *o_size) {
    float *out = malloc(0);
    *o_size = 0;
    char *s; // subject
    char *g; // grade
    while (csv_move_to_next_line(csv_fd)) {
        s = csv_read_to_next(csv_fd, ';');
        if (strcmp(s, subject) == 0) {
            g = csv_read_to_next(csv_fd, ';');
            out = (float *) realloc(out, sizeof(float) * (*o_size+1));
            out[*o_size] = strtof(g, NULL);
            (*o_size)++;
            free(g);
        }
        free(s);
    } 
    return out;
}

/**
 * traverses an array and searches for an option value.
 * e.g. let arr=["cgrade", "--csv", "cgrade.csv", "--more-options"]
 * then cmd_get_option(4, arr, "--csv") returns cgrade.csv
 * everything that starts with "-" is considered an option
 * and not a value.
 * 
 * arg(n) the size of the cmd array
 * arg(cmd) the cmd array to be traversed
 * arg(option) the option to be searched
 *
 * returns the value for the first option occurence or NULL if the option could not be found.
 * if the option is directly followed by another option, then an empty string is returned,
 * which indicates that the option was present without any value.
 */
char *cmd_get_option(int n, char **cmd, char *option) {
    for (int i = 0; i < n; i++) {
        if (strcmp(option, cmd[i]) == 0) {
            if (i+1 < n && cmd[i+1][0] != '-') {
                return cmd[i+1];
            } else {
                return "";
            }
        }
    }
    return NULL;
}

bool cmd_is_option(char *cmd) {
    if (cmd[0] == '-') return true;
    return false;
}

/**
 * moves the cmd array pointer forward to the first string
 * that is not an option. Everything that starts with
 * an '-' is considered an option.
 *
 * arg(n)   size of the cmd array
 * arg(cmd) a pointer to an array of strings
 *
 * returns a pointer that points to the first string of 
 * the cmd array which isn't an option. Furthermore, the
 * size pointer arg(n) will be set to the correct size
 * of the new cmd array
 */
char **cmd_skip_options(int *n, char *cmd[]) {
    int counter = 0;
    int prevSkipped = 0;
    while (*n > 0 && (cmd[0][0] == '-' || prevSkipped)) {
        prevSkipped = !prevSkipped;
        cmd++;
        *n -= 1;
    }
    return cmd;
}

/**
 * Execute add command.
 *
 * arg(length)   size of args
 * arg(args)     array with the command arguments
 */
void command_add(int length, char *args[]) {
    if (length > 0 && streq(*args, OPT_NAME_HELP)) command_add_usage(0);
    if (length < 2) command_add_usage(-1);
    csv_must_exist();
    char *subject = args[0];
    char *grade = args[1];
    float gradef = strtof(grade, NULL);
    if (gradef == 0) {
        printf("invalid grade '%s'\n", grade);
        exit(-1);
    }
    char *msg = "";
    if (length > 2) msg = args[2];

    int o_flag = O_WRONLY | O_APPEND;
    int fd = open(opt_default_csv, o_flag);
    if (fd == -1) {
        printerrno("open file failed");
    }
    csv_write_string(fd, subject);
    csv_write_string(fd, ";");
    csv_write_string(fd, grade);
    csv_write_string(fd, ";");
    csv_write_string(fd, msg);
    csv_write_string(fd, "\n");

    if (close(fd) == -1) {
        printerrno(NULL);
    }

    printf("added grade %.2f to %s!\n", gradef, subject);
}

/**
 * Execute status command.
 *
 * arg(length)   size of args
 * arg(args)     array with the command arguments
 */
void command_status(int length, char *args[]) {
    if (length > 0 && streq(args[0], OPT_NAME_HELP)) command_status_usage(0);
    csv_must_exist();
    int fd = open(opt_default_csv, O_RDONLY);
    int n_grades;
    float *grades;
    char *title;
    if (length > 0) {
        grades = csv_subject_grades(fd, args[0], &n_grades);
        printf("Stats for %s\n", args[0]);
    } else {
        grades = csv_grades(fd, &n_grades);
        printf("Stats\n");
    }

    printf("Grades: %.2f", *grades);
    float avg = *grades;
    for (float* f = (grades+1); f < (grades + n_grades); f++) {
        printf(", %.2f", *f);
        avg += *f;
    }
    printf("\n");
    avg /= n_grades;
    printf("Avg: %.2f\n", avg);

    free(grades);
    if (close(fd) == -1) printerrno("command_default close failed");
}

/**
 * Execute init command.
 *
 * arg(length)   size of args
 * arg(args)     array with the command arguments
 */
void command_init(int length, char **args) {
    if (csv_exists()) exit_csv_already_initialized();
    csv_init_file();
    printf("%s successfully initialized!\n", DEFAULT_CSV_NAME);
}

void command_rm(int argc, char **args) {
    // Vorgehen:
    // 1. Neues tmp.csv anlegen
    // 2. Lese für jede Zeile von cgrade.cs von cgrade.csvv:
    // 2.1 Fach
    // 2.2 Note
    // 2.3 Wenn Fach & Note nicht der entspricht, die gelöscht werden sollte, soll NOTE;GRADE;{restliche zeile} in tmp.csv geschrieben werden
    // 3. Remove cgrade.csv, rename tmp.csv to cgrade.csv
    // Edge Cases:
    // * 2 Mal gleiche Note in gleichem Fach vorhanden
    // Command Structure:
    // * cgrade rm algd2 5.25 1
    char *subject = args[0];
    char *grade = args[1];
    int delete_index = 1;
    if (argc > 2) {
        delete_index = strtol(args[2], NULL,10);
    }
    
    int read_fd = open(opt_default_csv, O_RDONLY);
    int mode_t = S_IRUSR | S_IWUSR;
    int write_fd = open(TMP_CSV_PATH, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, mode_t);

    char *header = csv_read_to_next(read_fd, '\n');
    csv_write_string(write_fd, header);
    csv_write_string(write_fd, "\n");
    char *pivot_subject = csv_read_to_next(read_fd, CSV_DELIMITER);
    char *pivot_grade = csv_read_to_next(read_fd, CSV_DELIMITER);
    char *pivot_comment = csv_read_to_next(read_fd, '\n');
    char *buffer;
    bool deleted = false;
    int delete_position = 1;
    while (strlen(pivot_subject) != 0) {
        if (streq(grade, pivot_grade) && streq(subject, pivot_subject)) {
            if (delete_index == delete_position) {
                delete_position++;
                deleted = true;
                // skip the line to be removed
                printf("skipping line")
                pivot_subject = csv_read_to_next(read_fd, ';');
                pivot_grade = csv_read_to_next(read_fd, ';');
                pivot_comment = csv_read_to_next(read_fd, '\n');
            }
        }
        csv_write_string(write_fd, pivot_subject);
        csv_write_string(write_fd, ";");
        csv_write_string(write_fd, pivot_grade);
        csv_write_string(write_fd, ";");
        csv_write_string(write_fd, pivot_comment);
        csv_write_string(write_fd, "\n");

        pivot_subject = csv_read_to_next(read_fd, ';');
        pivot_grade = csv_read_to_next(read_fd, ';');
        pivot_comment = csv_read_to_next(read_fd, '\n');
    }
    
    /*buffer = csv_read_to_next(read_fd, '\n');
    while (strlen(buffer) != 0) {
        csv_write_string(write_fd, buffer);
        csv_write_string(write_fd, "\n");
        buffer = csv_read_to_next(read_fd, '\n');
    }*/

    if (remove(opt_default_csv) == -1) {
        printerrno("failed to remove csv");
    }
    if (rename(TMP_CSV_PATH, opt_default_csv) != 0) {
        printerrno("failed to rename tmp csv");
    }
}

int cmd_process_option_csv(int length, char **cmd) {
    opt_default_csv = cmd[1]; 
    return 2;
}


/*
 * Processes a cmd option.
 *
 * arg(length)  length of the cmd array
 * arg(cmd)     array containing the cmd elements
 * returns the number of cmd items processed
 */
int cmd_process_option(int length, char **cmd) {
    char *opt = cmd[0];
    if (streq(OPT_NAME_CSV, opt)) {
        return cmd_process_option_csv(length, cmd);
    } else if (streq(OPT_NAME_HELP, opt)) {
        exit_usage(0);
    } else {
        exit_unknown_option(opt);
    }
    return 0;
}

void command(int argc, char **argv) {
    argv++;
    argc--;
    if (argc < 1) exit_usage(-1);

    while (cmd_is_option(argv[0])) { 
        int processed = cmd_process_option(argc, argv);
        argc -= processed;
        argv += processed;
    }

    //argv = cmd_skip_options(&argc, argv); 
    char *cmd = argv[0];
    if (strcmp(cmd, CMD_NAME_ADD) == 0) {
        command_add(argc-1, argv+1);
    } else if (strcmp(cmd, CMD_NAME_STATUS) == 0) {
        command_status(argc-1, argv+1);
    } else if (strcmp(cmd, CMD_NAME_RM) == 0) {
        command_rm(argc-1, argv+1);
    } else if (streq(cmd, CMD_NAME_INIT)) {
        command_init(argc-1, argv+1);
    } else {
        exit_unknown_cmd(cmd);
    }
}
