# About

A simple command line tool with which you can manage school grades in a csv file.

*Contact: noethiger.mike@gmail.com*


# Usage

Init a new cgrade.csv file in the working directory:

    $ cgrade init

Add a grade:

    $ cgrade add math 4.5 "First exam"

Show an overview of all grades and things like the average:

    $ cgrade status

For more actions refer to the user manual:

    $ cgrade --help

# Installation

Download the compiled C program located in this repository ([out/cgrade](https://github.com/mikenoethiger/cgrade/blob/master/out/cgrade)). In order to make the `cgrade` program available independent of the current working directory, copy it to some place that is in the system $PATH variable. For example:

    $ cp cgrade usr/local/bin

Check whether the program can be located:

    $ which cgrade

If everything worked, this should return the absolute path to the cgrade program.

# Developers

The cgrade source code is located in `cgrade.c`. The [ctest](https://github.com/mikenoethiger/ctest) library is used to do unit testing.
The unit tests can be found in `cgrade_test.c`. The main method for the `cgrade.c` file is outsourced to `cgrade_main.c`. 
This is required, because `cgrade_test.c` which imports `cgrade.c` also uses a main method and only one main method is allowed in a C program.
A makefile has been created to automate common tasks. The make targets are described in the following.

Compile and run unit tests (both targets described below):

    $ make

Compile the programs (cgrade and the unit tests):

    $ make compile

This will generate `out/cgrade` and `out/cgrade_test` which are compiled, runnabel C programs. The first one being the cgrade program and the second one the unit tests.

Run tests:

    $ make test

Clean the `out` directory:

    $ make clean


