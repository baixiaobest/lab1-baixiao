// UCLA CS 111 Lab 1 main program

// Copyright 2012-2014 Paul Eggert.

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

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <string.h>

#include "command.h"

static char const *program_name;
static char const *script_name;

static void
usage (void)
{
  error (1, 0, "usage: %s [-p PROF-FILE | -t] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int
main (int argc, char **argv)
{
  int command_number = 1;
  bool print_tree = false;
  char const *profile_name = 0;
  program_name = argv[0];

  for (;;)
    switch (getopt (argc, argv, "p:t"))
      {
      case 'p': profile_name = optarg; break;
      case 't': print_tree = true; break;
      default: usage (); break;
      case -1: goto options_exhausted;
      }
 options_exhausted:;

  // There must be exactly one file argument.
  if (optind != argc - 1)
    usage ();

  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
  command_stream_t command_stream =
    make_command_stream (get_next_byte, script_stream);
  int profiling = -1;
  if (profile_name)
    {
      profiling = prepare_profiling (profile_name);
      if (profiling < 0)
	error (1, errno, "%s: cannot open", profile_name);
    }

  command_t last_command = NULL;
  command_t command;

    //timer starts
    struct timespec absolute;
    double start_time, prev_user_CPU, prev_sys_CPU;
    get_timer(&start_time, &prev_user_CPU, &prev_sys_CPU);
  while ((command = read_command_stream (command_stream)))
    {
      if (print_tree)
	{
	  printf ("# %d\n", command_number++);
	  print_command (command);
	}
      else
	{
	  last_command = command;
	  execute_command (command, profiling);
	}
    }
    //timer stops
    clock_gettime(CLOCK_REALTIME, &absolute);
    double absolute_time = absolute.tv_sec + (double)absolute.tv_nsec/1000000000.0;
    double end_time, current_user_CPU, current_sys_CPU;
    get_timer(&end_time, &current_user_CPU, &current_sys_CPU);
    double real_time = end_time - start_time;
    double userCPU = current_user_CPU-prev_user_CPU;
    double sysCPU = current_sys_CPU-prev_sys_CPU;
    
    pid_t pid = getpid();
    char buffer[1024];
    sprintf(buffer, "%f %f %.3f %.3f %d\n", absolute_time, real_time, userCPU, sysCPU, (int)pid);
    write(profiling, buffer, strlen(buffer));
    

  return print_tree || !last_command ? 0 : command_status (last_command);
}
