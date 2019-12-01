#include <stdio.h>
#include <string.h>
#include "libs/ctest.h"
#include "cgrade.c"
#include <stdio.h>
#include <unistd.h>

#define TEST_CSV_FILE "./test_cgrade.csv"

/**
 * Creates a csv file and writes the specified content to it.
 * The file is opened with read and write permissions.
 *
 * @param content to be written into the file
 * @return the file descriptor
 */
static int test_csv_create(char *content) {
    int mode_t = S_IRUSR | S_IWUSR;
    int flags = O_RDWR | O_CREAT | O_TRUNC | O_APPEND;
    int fd = open(TEST_CSV_FILE, flags, mode_t);

    for (int i = 0; i < strlen(content); i++) {
        if (write(fd, &content[i], sizeof(content[i])) == -1) {
        	printerrno("write test csv failed");
        }
    }
    return fd;
}

/**
 * Deletes the test csv file.
 */
static void test_csv_delete() {
	if (remove(TEST_CSV_FILE) == -1) {
		printerrno("deleting test csv failed");
	}
}

static int test_command_rm() {
	/*int fd = test_csv_create("subject;grade;comment\nmath;5.2;First Exam\nmath;5.2;Second Exam\nmath;4.6;Third Exam\n");
	char *cmd[] = {"math", "5.2"};
	command_rm(2, cmd);*/
	return 1;
}

static int test_subject_create() {
    char *name = "math";
    Subject s = subject_create(name);
    assrt(strcmp(s.name, name) == 0);
    assrt(s.n_grades == 0);
    return 1;
}

static int test_cmd_get_option() {
    char *cmd_1[] = { "cgrade", "--csv", "cgrade.csv", "somethingElse" };
    char *cmd_2[] = { "cgrade", "--csv" }; 
    char *cmd_3[] = { "cgrade", "--csv", "--other" }; 
    char *cmd_4[] = { "cgrade", "--csv", "first", "--csv", "second" }; 

    char *csv_option = cmd_get_option(4, cmd_1, "--csv");
    assrt(strcmp(csv_option, "cgrade.csv") == 0);

    csv_option = cmd_get_option(2, cmd_2, "--csv");
    assrt(strlen(csv_option) == 0);

    csv_option = cmd_get_option(3, cmd_3, "--csv");
    assrt(strlen(csv_option) == 0);

    csv_option = cmd_get_option(3, cmd_3, "--not-existing");
    assrt(csv_option == NULL);

    csv_option = cmd_get_option(5, cmd_4, "--csv");
    assrt(strcmp(csv_option, "first") == 0);

    return 1;
}

static int test_cmd_skip_options() {
    char *cmd_1[] = { "cgrade", "--opt" };
    char *cmd_2[] = { "--opt1", "--opt2", "cmd" };
    char *cmd_3[] = { "--opt1", "--opt2", "--opt3" };
    char *cmd_4[] = { "--csv", "value", "cmd" };
    
    int size = 2;
    char **cmd_1_res = cmd_skip_options(&size, cmd_1);
    assrt(size == 2);
    assrt(strcmp(cmd_1_res[0], "cgrade") == 0);

    size = 3;
    char **cmd_2_res = cmd_skip_options(&size, cmd_2);
    assrt(size == 1);
    assrt(strcmp(cmd_2_res[0], "cmd") == 0);

    size = 3;
    char **cmd_3_res = cmd_skip_options(&size, cmd_3);
    assrt(size == 0);

    size = 3;
    char **cmd_4_res = cmd_skip_options(&size, cmd_4);
    assrt(size == 1);
    assrt(strcmp(cmd_4_res[0], "cmd") == 0);

    return 1;
}

static int command_rm_test() {

    return 1;
}

void pointer(char ***args) {
    (*args)++;
}
static int test_playground() {
    char *names_arr[] = { "Mike", "Pascal" };
    char **names = names_arr;
    pointer(&names);
    return 1;
}

static void all_tests() {
    ctest_run(test_subject_create);
    ctest_run(test_cmd_get_option);
    ctest_run(test_cmd_skip_options);
    ctest_run(test_playground);
}

int main(int argc, char **argv) {
    all_tests();
    ctest_print_result();

    return tests_failed;
}
