/*-
 * Copyright 2017 Shivansh Rai
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <array>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

#include "utils.h"

#define READ 0  	/* Pipe descriptor: read end. */
#define WRITE 1 	/* Pipe descriptor: write end. */
#define BUFSIZE 128 	/* Buffer size (used for buffering output
			 * from a utility's execution).
			 */
#define TIMEOUT 2 	/* threshold (seconds) for a function call to return. */

/*
 * Insert a list of user-defined option definitions
 * into a hashmap. These specific option definitions
 * are the ones which one can be easily tested.
 */
void
utils::OptDefinition::InsertOpts()
{
	/* Option definitions. */
	OptRelation h_def;        /* '-h' */
	h_def.type    = 's';
	h_def.value   = "h";
	h_def.keyword = "help";

	OptRelation v_def;        /* '-v' */
	v_def.type    = 's';
	v_def.value   = "v";
	v_def.keyword = "version";

	/*
	 * "opt_map" contains all the options
	 * which can be easily tested.
	 */
	opt_map.insert(std::make_pair<std::string, OptRelation>
		      ("h", (OptRelation)h_def));
	opt_map.insert(std::make_pair<std::string, OptRelation>
		      ("v", (OptRelation)v_def));
};

/*
 * For the utility under test, find the supported options
 * present in the hashmap generated by InsertOpts()
 * and return them in a form of list of option relations.
 */
std::list<utils::OptRelation *>
utils::OptDefinition::CheckOpts(std::string utility)
{
	std::string line;                       /* An individual line in a man-page. */
	std::string opt_name;                   /* Name of the option. */
	std::string opt_identifier = ".It Fl";  /* Identifier for an option in man page. */
	std::string buffer;                     /* Option description extracted from man-page. */
	std::string opt_string;                 /* Identified option names. */
	int opt_position;                       /* Starting index of the (identified) option. */
	int space_index;                        /* First occurrence of space character
					         * in a multi-word option definition.
					         */
	std::list<OptRelation *> identified_opt_list;  /* List of identified option relations. */
	std::list<std::string> supported_sections = { "1", "8" };

	/* Generate the hashmap opt_map. */
	InsertOpts();

	for (const auto &section : supported_sections) {
		std::ifstream infile("groff/" + utility + "." + section);

		/*
		 * Search for all the options accepted by the
		 * utility and collect those present in "opt_map".
		 */
		while (std::getline(infile, line)) {
			if ((opt_position = line.find(opt_identifier)) != std::string::npos) {
				/* Locate the position of option name. */
				opt_position += opt_identifier.length() + 1;

				if (opt_position > line.length()) {
					/*
					 * This condition will trigger when a utility
					 * supports an empty argument, e.g. tset(issue #9)
					 */
					continue;
				}

				/*
				 * Check for long options ; While here, also sanitize
				 * multi-word option definitions in a man page to properly
				 * extract short options from option definitions such as:
				 * 	.It Fl r Ar seconds (taken from date(1)).
				 */
				if ((space_index = line.find(" ", opt_position + 1, 1))
						!= std::string::npos)
					opt_name = line.substr(opt_position, space_index - opt_position);
				else
					opt_name = line.substr(opt_position);

				/*
				 * Check if the identified option matches the identifier.
				 * "opt_list.back()" is the previously checked option, the
				 * description of which is now stored in "buffer".
				 */
				if (!opt_list.empty() &&
						(opt_map_iter = opt_map.find(opt_list.back()))
						!= opt_map.end() &&
						buffer.find((opt_map_iter->second).keyword) != std::string::npos) {
					identified_opt_list.push_back(&(opt_map_iter->second));

					/*
					 * Since the usage of the option under test
					 * is known, we remove it from "opt_list".
					 */
					opt_list.pop_back();
				}

				/* Update the list of valid options. */
				opt_list.push_back(opt_name);

				/* Empty the buffer for next option's description. */
				buffer.clear();
			} else {
				/*
				 * Collect the option description until next
				 * valid option definition is encountered.
				 */
				buffer.append(line);
			}
		}
	}

	return identified_opt_list;
}

/*
 * When pclose() is called on the stream returned by popen(),
 * it waits indefinitely for the created shell process to
 * terminate in cases where the shell command waits for the
 * user input via a blocking read (e.g. passwd(1)).
 * Hence, we define a custom function which alongside
 * returning the FILE* stream (as with popen()) also returns
 * the pid of the newly created (child) shell process. This
 * pid can later be used for sending a signal to the child.
 *
 * NOTE For the sake of completeness, we also specify actions
 * to be taken corresponding to the "w" (write) type. However,
 * in this context only the "r" (read) type will be required.
 */
FILE*
utils::POpen(const char *command, const char *type, pid_t& pid)
{
	int pdes[2];
	int pdes_unused_in_parent;
	char *argv[4];
	FILE *iop;
	pid_t child_pid;

	/* Create a pipe with ~
	 *   - pdes[READ]: read end
	 *   - pdes[WRITE]: write end
	 */
	if (pipe2(pdes, O_CLOEXEC) < 0)
		return NULL;

	if (*type == 'r') {
		iop = fdopen(pdes[READ], type);
		pdes_unused_in_parent = pdes[WRITE];
	} else if (*type == 'w') {
		iop = fdopen(pdes[WRITE], type);
		pdes_unused_in_parent = pdes[READ];
	} else
		return NULL;

	if (iop == NULL) {
		close(pdes[READ]);
		close(pdes[WRITE]);
		return NULL;
	}

	argv[0] = (char *)"sh"; 	/* Type-cast to avoid compiler warning [-Wwrite-strings]. */
	argv[1] = (char *)"-c";
	argv[2] = (char *)command;
	argv[3] = NULL;

	switch (child_pid = vfork()) {
		case -1: 		/* Error. */
			close(pdes_unused_in_parent);
			fclose(iop);
			return NULL;
		case 0: 		/* Child. */
			if (*type == 'r') {
				if (pdes[WRITE] != STDOUT_FILENO)
					dup2(pdes[WRITE], STDOUT_FILENO);
				else
					fcntl(pdes[WRITE], F_SETFD, 0);
			} else {
				if (pdes[READ] != STDIN_FILENO)
					dup2(pdes[READ], STDIN_FILENO);
				else
					fcntl(pdes[READ], F_SETFD, 0);
			}

			/*
			 * For current usecase, it might so happen that the child gets
			 * stuck on a blocking read (e.g. passwd(1)) waiting for user
			 * input. In that case the child will be killed via a signal.
			 * To avoid any effect on the parent's execution, we place the
			 * child in a separate process group with pgid set as "child_pid".
			 */
			setpgid(child_pid, child_pid);
			execve("/bin/sh", argv, NULL);
			exit(127);
	}

	/* Parent */
	close(pdes_unused_in_parent);
	pid = child_pid;

	return iop;
}

/*
 * Executes the passed argument "command" in a shell
 * and returns its output and the exit status.
 */
std::pair<std::string, int>
utils::Execute(std::string command)
{
	pid_t pid;
	int result;
	std::array<char, BUFSIZE> buffer;
	std::string usage_output;
	struct timeval tv;
	fd_set readfds;
	FILE *pipe = utils::POpen(command.c_str(), "r", pid);

	if (pipe == NULL) {
		perror ("popen()");
		exit(EXIT_FAILURE);
	}

	/*
	 * Set a timeout value for the spawned shell
	 * process to complete its execution.
	 */
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;

	FD_ZERO(&readfds);
	FD_SET(fileno(pipe), &readfds);
	result = select(fileno(pipe) + 1, &readfds, NULL, NULL, &tv);

	if (result > 0) {
		try {
			while (!feof(pipe))
				if (std::fgets(buffer.data(), BUFSIZE, pipe) != NULL)
					usage_output += buffer.data();
		} catch(...) {
			pclose(pipe);
			throw "Unable to execute the command: " + command;
		}

	} else if (result == -1) {
		perror("select()");
		kill(pid, SIGTERM);
		exit(EXIT_FAILURE);
	}

	/*
	 * We gave a relaxed value of 2 seconds for the shell process
	 * to complete it's execution. If at this point it is still
	 * alive, it (most probably) is stuck on a blocking read
	 * waiting for the user input. Since few of the utilities
	 * performing such blocking reads don't respond to SIGINT
	 * (e.g. pax(1)), we terminate the shell process via SIGTERM.
	 */
	if (kill(pid, SIGTERM) < 0)
		perror("kill()");

	return std::make_pair<std::string, int>
		((std::string)usage_output, WEXITSTATUS(pclose(pipe)));
}
