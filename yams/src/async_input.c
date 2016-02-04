/* yams -- Yet Another Machine Simulator
   Copyright (C) 2002-2005 Juha Aatrokoski, Timo Lilja, Leena Salmela,
   Teemu Takanen, Aleksi Virtanen

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.

   $Id: async_input.c,v 1.6 2010/11/24 16:52:19 jaatroko Exp $
*/

#include "includes.h"
#include "cfg.h"
#include "async_input.h"
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

/* How often does the asynchronous input subsystem call select().
 * Every 100th clock cycle is used as a default, although it is quite
 * often for most usage. This is a compromise between efficiency and
 * responsiveness. To avoid the need to make this compromise is why
 * POSIX threads should be used for polling the input. This can be
 * overridden at compile time with the configure option
 * '--with-polling-interval'.
 */
#ifndef ASYNC_INPUT_POLL_INTERVAL
#define ASYNC_INPUT_POLL_INTERVAL 100
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
#include <sys/time.h>
#endif

#ifdef POLLING_PTHREAD
#include <semaphore.h>
#include <pthread.h>
#endif

typedef struct {
    int fd;
    int input;
} fdlist_t;

static fdlist_t fdlist[CFG_MAX_DEVICES];
static int numfd = 0;

static int async_input_started = 0;

/* These variables are used for inter-thread communication: */

/* Read/set/reset by main thread, not used by I/O thread */
static int input_available = 0;
/* Read and reset by main thread, set by I/O thread */
static volatile int input_available_poller = 0;


#ifdef POLLING_PTHREAD
static sem_t async_semaphore;
static pthread_t async_thread;

/* The function doing all the polling when using threads */
static void *async_input_poller(void *dummy) {
    fd_set rfds;
    int maxfd = -1, i;

    /* If there are no input fds, just wait forever on the
     * semaphore. 
     */
    if (numfd == 0)
	sem_wait(&async_semaphore);


    /* find the maximum fd */
    for (i=0; i<numfd; i++)
	if (fdlist[i].fd > maxfd)
	    maxfd = fdlist[i].fd;
    maxfd++;

    while(1) {
	FD_ZERO(&rfds);
	for (i=0; i<numfd; i++)
	    FD_SET(fdlist[i].fd, &rfds);
    
	/* Wait until there really is some input available. */
	while (select(maxfd, &rfds, NULL, NULL, NULL) <= 0);

	/* Set the input availability flags. */
	for (i=0; i<numfd; i++)
	    if (FD_ISSET(fdlist[i].fd, &rfds))
		fdlist[i].input = 1;

	/* Only after this will async_input_unlock() do anything,
	 * since it depends on the latched value of this variable.
	 * This means that the main thread will not touch the shared
	 * variables until we get here, and we won't continue from
	 * here until the main thread has reset this variable. Thus
	 * there is no problem with concurrent access.
	 */
	input_available_poller = 1;
	sem_wait(&async_semaphore);
    }

    return NULL;
}
#endif

#ifdef POLLING_FORK
extern void *yams_original_brk;
static pid_t async_input_sigpid = -1;

/* polling signal handler */
static void async_input_sigusr1(int signum) {
    input_available_poller = 1;
}

static void async_input_sigchld(int signum) {
    fprintf(stderr, "\nThe input poller process died unexpectedly, exiting.\n");
    exit(-1);
}

static void async_input_cleanup() {
    /* kill the poller when program exits */
    kill(async_input_sigpid, SIGTERM);
}

/* handling all fatal signals: kill child & exit */
static void async_input_sighandler(int signum) {
    kill(async_input_sigpid, SIGTERM);
    exit(-1);
}

/* The function doing all the polling  */
static void async_input_poller() {
    fd_set rfds;
    int maxfd = -1, i;


    /* find the maximum fd */
    for (i=0; i<numfd; i++)
	if (fdlist[i].fd > maxfd)
	    maxfd = fdlist[i].fd;
    maxfd++;

    while(1) {
	FD_ZERO(&rfds);
	for (i=0; i<numfd; i++)
	    FD_SET(fdlist[i].fd, &rfds);
    
	/* Wait until there really is some input available. */
	while (select(maxfd, &rfds, NULL, NULL, NULL) <= 0);

	/* Set the input availability flags. */
	for (i=0; i<numfd; i++)
	    if (FD_ISSET(fdlist[i].fd, &rfds))
		fdlist[i].input = 1;

	input_available_poller = 0;
	/* signal main process */
	kill(async_input_sigpid, SIGUSR1);
	/* wait until signaled */
	while(!input_available_poller);
    }
}
#endif


/* Start the asynchronous input subsystem. */
int async_input_start() {
    printf("Starting the asynchronous input subsystem.\n");

#ifdef POLLING_PTHREAD
    if (sem_init(&async_semaphore, 0, 0) == -1) {
	fprintf(stderr, "Cannot initialize a semaphore.\n");
	return 1;
    }

    if (pthread_create(&async_thread, NULL, async_input_poller, NULL)) {
	fprintf(stderr, "Cannot create input polling thread.\n");
	return 1;
    }
#endif

#ifdef POLLING_FORK
    signal(SIGUSR1, async_input_sigusr1);

    async_input_sigpid = fork();

    if (async_input_sigpid == -1) {
	fprintf(stderr, "Cannot fork input polling process.\n");
	return 1;
    }

    if (async_input_sigpid == 0) {
	/* we are the child: enter the poller function */
	async_input_sigpid = getppid();
	close(0); /* stdin */
	/* don't want the ctrl-c to kill our poller: */
	signal(SIGINT, SIG_IGN);
#ifdef HAVE_BRK
	/* This is a desperate attempt to release all dynamically
	 * allocated memory. It migth work and it might not, might as
	 * well try it. The worst it can do is to totally mess up the
	 * dynamic memory allocation, but we won't use dynamic memory
	 * anymore in this poller process.
	 */
	brk(yams_original_brk);
#endif
	async_input_poller();
    }
    /* if we got here, we are the parent */
    atexit(async_input_cleanup);
    signal(SIGHUP, async_input_sighandler);
    signal(SIGINT, async_input_sighandler);
    signal(SIGPIPE, async_input_sighandler);
    signal(SIGTERM, async_input_sighandler);
    signal(SIGUSR2, async_input_sighandler);
    signal(SIGCHLD, async_input_sigchld);
#endif

    async_input_started = 1;
    return 0;
}

/* Register a file descriptor for input polling. 
 * Must be called before async_input_start()
 */
int async_input_register_fd(int fd) {
    /* Check for numfd < CFG_MAX_DEVICES is redundant */

    if (async_input_started)
	return 1;

    fdlist[numfd].fd = fd;
    fdlist[numfd].input = 0;

    numfd++;

    return 0;
}

/* Check if the file decriptor has input available. 
 * Must be called when the input subsystem is locked with
 * async_input_lock()
 */
int async_input_check_fd(int fd) {
    int i;

    if (!input_available)
	return 0;

    for (i=0; i<numfd; i++)
	if (fdlist[i].fd == fd)
	    return fdlist[i].input;

    return 0;
}

/* Verify that the file descriptor really has input available. Must be
 * called only after async_input_check_fd() returned true. This is
 * only needed when using the pthread polling method with external I/O
 * devices, since they use the same fd for synchronous I/O. This makes
 * it possible that the fd is marked as having input but the input is
 * eaten by the synchronous I/O and there is nothing available even
 * though async_input_check_fd() returns true. This is why we need to
 * verify the availability when using pthreads. This is not a problem
 * with the select and fork methods, since they call select in
 * async_input_lock().
 */
int async_input_verify_fd(int fd) {
#ifdef POLLING_PTHREAD
    fd_set rfds;
    struct timeval timeout;
    
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    timeout.tv_sec  = 0;
    timeout.tv_usec = 0;

    if (select(fd+1, &rfds, NULL, NULL, &timeout) > 0) {
	if (FD_ISSET(fd, &rfds))
	    return 1;
	else 
	    return 0;
    }
#endif
    return 1;
}

/* Lock the input subsystem. 
 * This will make the 'input available' flags for all file descriptors
 * available to be used with async_input_check_fd()
 */
void async_input_lock() {
#if !defined POLLING_PTHREAD && !defined POLLING_FORK
    fd_set rfds;
    struct timeval timeout;
    int maxfd = -1, i;
    static int count = 0;

    if (numfd == 0)
	return;

    count = (count + 1) % ASYNC_INPUT_POLL_INTERVAL;
    if (count != 0)
	return;

    FD_ZERO(&rfds);
    for (i=0; i<numfd; i++) {
	FD_SET(fdlist[i].fd, &rfds);
	if (fdlist[i].fd > maxfd)
	    maxfd = fdlist[i].fd;
    }
    
    timeout.tv_sec  = 0;
    timeout.tv_usec = 0;
    
    input_available_poller = 0;
    if (select(maxfd+1, &rfds, NULL, NULL, &timeout) > 0) {
	for (i=0; i<numfd; i++)
	    if (FD_ISSET(fdlist[i].fd, &rfds)) {
		fdlist[i].input = 1;
		input_available_poller = 1;
	    }
    }
#endif

    /* Latch the input availability. */
    input_available = input_available_poller;

#ifdef POLLING_FORK
    if (input_available) {
	fd_set rfds;
	int maxfd = -1, i;
	struct timeval timeout;


	FD_ZERO(&rfds);
	for (i=0; i<numfd; i++) {
	    FD_SET(fdlist[i].fd, &rfds);
	    if (fdlist[i].fd > maxfd)
		maxfd = fdlist[i].fd;
	}
	maxfd++;
	
	timeout.tv_sec  = 0;
	timeout.tv_usec = 0;

	/* Check which fds have input. */
	if (select(maxfd, &rfds, NULL, NULL, &timeout) > 0) {
	    /* Set the input availability flags. */
	    for (i=0; i<numfd; i++)
		if (FD_ISSET(fdlist[i].fd, &rfds))
		    fdlist[i].input = 1;
	}
    }
#endif
}

/* Unlock the input subsystem.
 * This will clear the 'input available' flags for all file
 * descriptors.
 */
void async_input_unlock() {
    int i;

    if (input_available) {
	/* If we get here when using threads, then the I/O thread is
	 * either waiting on the semaphore or just about to, so it
	 * should be safe to do almost anything before signaling the
	 * semaphore. As for the poller process, the SIGUSR1
	 * signalling works like a semaphore.
	 */
	input_available = 0;
	/* Clear input availability flags. */
	for (i=0; i<numfd; i++)
	    fdlist[i].input = 0;

#ifdef POLLING_PTHREAD
	/* Reset to zero before signaling */
	input_available_poller = 0;
	sem_post(&async_semaphore);
#endif

#ifdef POLLING_FORK
	/* Reset to zero before signaling */
	input_available_poller = 0;
	kill(async_input_sigpid, SIGUSR1);
#endif
    }
}
