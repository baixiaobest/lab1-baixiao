// UCLA CS 111 Lab 1 command execution

// Copyright 2012-2014 Paul Eggert.
// Copyleft  2014-2017 Baixiao Huang

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "command.h"
#include "command-internals.h"

#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <stdint.h>

const int STANDARD_INPUT = 0;
const int STANDARD_OUTPUT = 1;
const int STANDARD_ERROR = 2;


void get_command(struct command *command, char* buffer)
{
    if (command==NULL) {
        return;
    }
    if (command->type==SIMPLE_COMMAND) {
        char** ptr = command->u.word;
        while ((*ptr)!=NULL) {
            strcat(buffer, *ptr);
            strcat(buffer, " ");
            ptr++;
        }
        if (command->input!=NULL) {
            strcat(buffer, "< ");
            strcat(buffer, command->input);
            strcat(buffer, " ");
        }
        if (command->output!=NULL) {
            strcat(buffer, "> ");
            strcat(buffer, command->output);
            strcat(buffer, " ");
        }
    }
    else if (command->type==SEQUENCE_COMMAND){
        get_command(command->u.command[0], buffer);
        strcat(buffer, "; ");
        get_command(command->u.command[1], buffer);
    }
    else if(command->type==PIPE_COMMAND){
        get_command(command->u.command[0], buffer);
        strcat(buffer, "| ");
        get_command(command->u.command[1], buffer);
    }
    else if(command->type==SUBSHELL_COMMAND){
        strcat(buffer, "( ");
        get_command(command->u.command[0], buffer);
        strcat(buffer, ") ");
    }
    else if(command->type==IF_COMMAND){
        strcat(buffer, " if ");
        get_command(command->u.command[0], buffer);
        if(command->u.command[1]!=NULL){
            strcat(buffer, "then ");
            get_command(command->u.command[1], buffer);
        }
        if (command->u.command[2]!=NULL) {
            strcat(buffer, "else ");
            get_command(command->u.command[2], buffer);
        }
        strcat(buffer, "fi ");
    }
    else if(command->type==WHILE_COMMAND || command->type==UNTIL_COMMAND){
        command->type==WHILE_COMMAND ? strcat(buffer, "while ") : strcat(buffer, "until ");
        get_command(command->u.command[0], buffer);
        strcat(buffer, "do ");
        get_command(command->u.command[1], buffer);
        strcat(buffer, "done");
    }
}

void get_timer(double *real_time, double *user_CPU, double *sys_cpu)
{
    struct timespec start;
    struct rusage self_usage, children_usage;
    if(clock_gettime(CLOCK_MONOTONIC, &start)==-1){error(1, 0, "cannot get clock");}
    getrusage(RUSAGE_SELF, &self_usage);
    getrusage(RUSAGE_CHILDREN, &children_usage);
    *real_time = start.tv_sec + start.tv_nsec/1000000000.0;
    *user_CPU = self_usage.ru_utime.tv_sec + children_usage.ru_utime.tv_sec + (self_usage.ru_utime.tv_usec + children_usage.ru_utime.tv_usec)/1000000.0;
    *sys_cpu = self_usage.ru_stime.tv_sec + children_usage.ru_stime.tv_sec + (self_usage.ru_stime.tv_usec + children_usage.ru_stime.tv_usec)/1000000.0;
}

void execute_simple_command(struct command *command, int profiling)
{
    
    struct timespec absolute;
    double start_time, prev_user_CPU, prev_sys_CPU;
    get_timer(&start_time, &prev_user_CPU, &prev_sys_CPU);
    
    int status;
    pid_t pid = fork();
    
    if(pid<0){
        error(1, 0, "Error system cannot get pregnent! *o* in command %s", command->u.word[0]);
    }else if(pid == 0){
        if (strcmp(command->u.word[0], "exec")==0) {
            command->u.word++;
        }
        execvp(command->u.word[0], command->u.word);
        _exit(127);
    }else{
        waitpid(pid, &status, 0);
        command->status = WEXITSTATUS(status);
        
    }
}

void execute_pipe_command(struct command *command, int profiling)
{
    pid_t sender_pid;
    pid_t receiver_pid;
    pid_t first_return_child;
    int status;
    int pipe_descriptors[2];
    
    if (pipe(pipe_descriptors) < 0) {
        error(1, 0, "Error creating pipe -_-||");
    }
    sender_pid = fork();
    if (sender_pid<0) {
        error(1, 0, "Error system cannot bear a child! ");
    }else if(sender_pid==0){
        close(pipe_descriptors[0]); //close read end
        dup2(pipe_descriptors[1], STANDARD_OUTPUT);
        execute_command_tree(command->u.command[0], profiling);
        close(pipe_descriptors[1]);
        _exit(command->u.command[0]->status);
    
    }else{
        receiver_pid = fork();
        if (receiver_pid<0) {
            error(1, 0, "Error system fail to get a child! =_=||");
        }else if(receiver_pid==0){
            close(pipe_descriptors[1]); //close write end
            dup2(pipe_descriptors[0], STANDARD_INPUT);
            execute_command_tree(command->u.command[1], profiling);
            _exit(command->u.command[1]->status);
        }else{
            close(pipe_descriptors[0]);
            close(pipe_descriptors[1]);
            first_return_child = waitpid(-1, &status, 0);
                if (first_return_child==sender_pid) {
                    waitpid(receiver_pid, &status, 0);
                }else{
                    waitpid(sender_pid, &status, 0);
                }
                command->status = WEXITSTATUS(status);
        }
    }
}

void execute_sequence_command(struct command *command, int profiling)
{
    execute_command_tree(command->u.command[0], profiling);
    execute_command_tree(command->u.command[1], profiling);
    command->status = command->u.command[1]->status;
    return;
}

void execute_subshell_command(struct command *command, int profiling)
{
    execute_command_tree(command->u.command[0], profiling);
    command->status = command->u.command[0]->status;
}

void execute_if_until_while_command(struct command *command, int profiling)
{
    int status;
    if (command->type == IF_COMMAND) {
        status = execute_command_tree(command->u.command[0], profiling);
        if (status==0) {
            execute_command_tree(command->u.command[1], profiling);
            command->status = command->u.command[1]->status;
        }else{
            execute_command_tree(command->u.command[2], profiling);
            command->status = command->u.command[2]->status;
        }
    }
    if (command->type == WHILE_COMMAND) {
        execute_command_tree(command->u.command[0], profiling);
        while (command->u.command[0]->status == 0) {
            execute_command_tree(command->u.command[1], profiling);
            execute_command_tree(command->u.command[0], profiling);
        }
        command->status = command->u.command[1]->status;
    }
    if (command->type == UNTIL_COMMAND) {
        execute_command_tree(command->u.command[0], profiling);
        while (command->u.command[0]->status!=0) {
            execute_command_tree(command->u.command[1], profiling);
            execute_command_tree(command->u.command[0], profiling);
        }
        command->status = command->u.command[1]->status;
    }
}

int execute_command_tree(struct command *command, int profiling)
{
    //timer starts
    struct timespec absolute;
    double start_time, prev_user_CPU, prev_sys_CPU;
    get_timer(&start_time, &prev_user_CPU, &prev_sys_CPU);
    
    int status;
    int standard_input = dup(STANDARD_INPUT);
    int standard_output = dup(STANDARD_OUTPUT);
 
    if (command->input!=NULL){
        int inputFd = open(command->input, O_RDONLY);
        if (inputFd < 0) {
            error(1, 0, "Error inputing file");
        }
        dup2(inputFd, STANDARD_INPUT);
        close(inputFd);
    }
    if (command->output!=NULL) {
        int outputFd = open(command->output, O_CREAT | O_WRONLY | O_TRUNC, 5555);
        if (outputFd < 0) {
            error(1, 0, "Error outputing file");
        }
        dup2(outputFd, STANDARD_OUTPUT);
        close(outputFd);
    }
    switch (command->type) {
        case SIMPLE_COMMAND:
            execute_simple_command(command, profiling);
            break;
        case PIPE_COMMAND:
            execute_pipe_command(command, profiling);
            break;
        case SEQUENCE_COMMAND:
            execute_sequence_command(command, profiling);
            break;
        case SUBSHELL_COMMAND:
            execute_subshell_command(command, profiling);
            break;
        case IF_COMMAND:
        case UNTIL_COMMAND:
        case WHILE_COMMAND:
            execute_if_until_while_command(command, profiling);
            break;
        default:
            break;
    }
    status = command->status;
    
    //timer stops
    clock_gettime(CLOCK_REALTIME, &absolute);
    double absolute_time = absolute.tv_sec + (double)absolute.tv_nsec/1000000000.0;
    
    double end_time, current_user_CPU, current_sys_CPU;
    get_timer(&end_time, &current_user_CPU, &current_sys_CPU);
    double real_time = end_time - start_time;
    double userCPU = current_user_CPU-prev_user_CPU;
    double sysCPU = current_sys_CPU-prev_sys_CPU;
    
    
    char buffer[1024];
    char *command_name = (char*)malloc(1024*sizeof(char));
    command_name[0]='\0';
    get_command(command, command_name);
    sprintf(buffer, "%f %f %.3f %.3f %s\n", absolute_time, real_time, userCPU, sysCPU, command_name);
    write(profiling, buffer, strlen(buffer));

    dup2(standard_input, STANDARD_INPUT);
    dup2(standard_output, STANDARD_OUTPUT);

    return status;
}


int
prepare_profiling (char const *name)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  //error (0, 0, "warning: profiling not yet implemented");
    
    return open(name, O_CREAT | O_WRONLY, 5555);
}

int
command_status (command_t c)
{
  return c->status;
}

void
execute_command (command_t c, int profiling)
{
  /* FIXME: Replace this with your implementation, like 'prepare_profiling'.  */
  //error (1, 0, "command execution not yet implemented");
    execute_command_tree(c, profiling);
}
