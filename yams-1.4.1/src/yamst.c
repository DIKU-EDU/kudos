/* yamst.c -- yams terminal emulator for unix and tcp sockets.

   Copyright (C) 2002--2003 Juha Aatrokoski, Timo Lilja, Leena Salmela,
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

   $Id: yamst.c,v 1.4 2005/04/16 11:03:21 jaatroko Exp $
*/

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/un.h>

struct termios terminal_state_stored;
struct termios terminal_state_raw;

/* what kind of socket: */
int LISTEN = 0;
int UNIX_SOCKET = 0;
int INET_SOCKET = 0;
int EXIT_ON_CLOSE = 0;

char *remote_name = NULL;
uint16_t remote_port = 0;

void console();
void handle_disconnect();

void terminal_raw();
void terminal_restore();
void clean_exit(int code);

int connect_inet(char *name, unsigned short port);
int listen_inet(unsigned short port);
int connect_unix(char *name);
int listen_unix(char *name);

void usage();


/* the socket file descriptor */
int fd;



int main(int argc, char **argv) {
    char *arg1 = NULL, *arg2 = NULL, *arg3 = NULL;
    char localhost[] = "localhost";
    int i;

    /* parse arguments */
    i = 1;
    while (i < argc) {
	if (argv[i][0] == '-') {
	    int j = 1;

	    while (argv[i][j] != '\0') {
		switch(argv[i][j]) {
		case 'u':
		    UNIX_SOCKET = 1;
		    break;
		case 'i':
		    INET_SOCKET = 1;
		    break;
		case 'l':
		    LISTEN = 1;
		    break;
		case 'e':
                    EXIT_ON_CLOSE = 1;
		    break;

		default:
		    usage();
		}
		j++;
	    }

	    i++;
	    continue;
	}

	/* a bit of a kludge, but it works: */
	if (arg1 == NULL)
	    arg1 = argv[i];
	else if (arg2 == NULL)
	    arg2 = argv[i];
	else
	    arg3 = argv[i];
	i++;
    }

    /* must have 1 or 2 nonswitch arguments */
    if (arg1 == NULL || arg3 != NULL) usage();

    if (UNIX_SOCKET && INET_SOCKET) usage();

    /* unix socket, argument is file name */
    if (UNIX_SOCKET) {
	if (arg2 != NULL) usage();
	remote_name = arg1;
    }

    /* inet socket, argument is port (listen) or hostname and port */
    if (INET_SOCKET) {
	if (LISTEN) {
	    if (arg2 != NULL) usage();
	    remote_port = atoi(arg1);
	    if (remote_port == 0) usage();
	} else {
	    if (arg2 == NULL) usage();
	    remote_port = atoi(arg2);
	    if (remote_port == 0) usage();
	    remote_name = arg1;
	}
    }


    /* only when interactive */
    if (isatty(0))
	tcgetattr(0,&terminal_state_stored);
    memcpy(&terminal_state_raw, &terminal_state_stored,sizeof(struct termios));

    /* Disable canonical mode, and set buffer size to 1 byte */
    terminal_state_raw.c_lflag &= (~(ECHO | ICANON | ISIG));
    terminal_state_raw.c_iflag = 0; /* disable XON XOFF etc. */
    terminal_state_raw.c_cc[VTIME] = 0;
    terminal_state_raw.c_cc[VMIN] = 1;


    /* open the socket */
    if (UNIX_SOCKET) {
	if (LISTEN) {
	    fd = listen_unix(remote_name);
	    if (fd < 0) {
		fprintf(stderr, "\n*** Error listening unix socket '%s': ",
			remote_name);
		perror("");
		clean_exit(1);
	    }
	} else {
	    fd = connect_unix(remote_name);
	    if (fd < 0) {
		fprintf(stderr, "\n*** Error connecting to unix socket '%s': ",
			remote_name);
		perror("");
		clean_exit(1);
	    }
	}
    } else if (INET_SOCKET) {
	if (LISTEN) {
	    fd = listen_inet(remote_port);
	    if (fd < 0) {
		fprintf(stderr, "\n*** Error listening inet port %u: ",
			remote_port);
		perror("");
		clean_exit(1);
	    }
	} else {
	    fd = connect_inet(remote_name, remote_port);
	    if (fd < 0) {
		fprintf(stderr, "\n*** Error connecting to %s:%u: ",
			remote_name, remote_port);
		perror("");
		clean_exit(1);
	    }
	}
    } else { /* unix/inet not specified, try guessing what is wanted */
	if (LISTEN) {
	    if (arg2 != NULL) usage(); /* only one arg for listen */
	    fd = -1;
	    /* first try inet port, if argument is integer 1..65535 */
	    remote_port = atoi(arg1);
	    remote_name = arg1;
	    if (remote_port > 0) {
		fd = listen_inet(remote_port);
		if (fd < 0)
		    printf("  Failed\n");
		else
		    INET_SOCKET = 1;
	    }
	    /* then try unix socket */
	    if (!INET_SOCKET) {
		fd = listen_unix(remote_name);
		if (fd < 0) {
		    printf("  Failed");
		    fflush(stdout);
		    usage();
		}
		UNIX_SOCKET = 1;
	    }
	} else {
	    /* two arguments => inet connect */
	    if (arg2 != NULL) {
		INET_SOCKET = 1;
		remote_name = arg1;
		remote_port = atoi(arg2);
		fd = connect_inet(remote_name, remote_port);
		if (fd < 0) {
		    fprintf(stderr, "\n*** Error connecting to %s:%u: ",
			    remote_name, remote_port);
		    perror("");
		    clean_exit(1);
		}
	    } else {
		/* one argument, connect to localhost or unix socket */
		remote_port = atoi(arg1);
		if (remote_port > 0) { /* implicit localhost */
		    remote_name = localhost;
		    fd = connect_inet(remote_name, remote_port);
		    if (fd < 0)
			printf("  Failed\n");
		    else
			INET_SOCKET = 1;
		}
		/* then try unix socket */
		if (!INET_SOCKET) {
		    remote_name = arg1;
		    fd = connect_unix(remote_name);
		    if (fd < 0) {
			printf("  Failed\n");
			fflush(stdout);
			usage();
		    }
		    UNIX_SOCKET = 1;
		}
	    }
	}
    }
    printf("  Connected\n");


    printf(">>> Escape character is '^]'\n");


    terminal_raw();

    while(1) {
	fd_set rfds;
	int retval;
	unsigned char b;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	FD_SET(0, &rfds);

	retval = select(fd + 1, &rfds, NULL, NULL, NULL);

	if (retval) {

	    /* input from the socket */
	    if (FD_ISSET(fd, &rfds)) {
		retval = read(fd, &b, 1);

		if (retval == 0) { /* disconnected */
		    handle_disconnect();
		} else if (retval != 1) { /* error */
		    perror("*** Read failed from socket");
		    clean_exit(2);
		}

		/* everything is printed as is */
		fputc(b, stdout);fflush(stdout);
	    }

	    /* input from keyboard */
	    if (FD_ISSET(0, &rfds)) {
		read(0, &b, 1);

		if (isatty(0) && b == 29) console(); /* ^] */

		/* the '^]' will also go through (do we want this?) */
		if (write(fd, &b, 1) != 1) {
		    perror("*** Write failed to socket");
		    clean_exit(2);
		}

	    }

	}

    }

    /* we should never get here, but just in case: */
    terminal_restore();
    return 0;
}







/* set terminal settings to raw mode */
void terminal_raw() {
    if (isatty(0))
	tcsetattr(0,TCSANOW,&terminal_state_raw);
}

/* restore terminal settings to what they were at startup */
void terminal_restore() {
    if (isatty(0))
	tcsetattr(0,TCSANOW,&terminal_state_stored);
}

void clean_exit(int code) {
    terminal_restore();
    exit(code);
}






void usage() {
    fprintf(stderr,
	    "Usage: yamst [-lui] [<hostname>] [<port>]\n"
	    "\t-u  Use unix domain sockets\n"
	    "\t-i  Use TCP sockets\n"
	    "\t-l  Listen the socket instead of connecting\n"
            "\t-e  Exit after connection is lost instead of retrying\n"
	    "Examples:\n"
	    "\tyamst -lu s0\n"
	    "\tyamst -li 10000\n"
	    "\tyamst -i host.domain.tld 10000\n"
	    "\tyamst -u s0\n");
    exit(-1);
}






/* the internal console for controlling yamst */
void console() {
    char command[20];
    int i;

    /* normal terminal for console */
    terminal_restore();

    printf("\n");
    while (1) {
	printf("yamst>");fflush(stdout);
	if (fgets(command, 20, stdin) == NULL)
	    exit(0);

	i = strlen(command);
	/* remove trailing spaces */
	while (i > 0 && command[i-1] <= ' ') command[--i] = 0;

	if (i == 0) break; /* empty command continues operation */

	if (!strcmp(command, "?") || !strcmp(command, "h")
	    || !strcmp(command, "help")) {
	    printf("\tcr\tCR (enter) is sent as is (default mode)\n");
	    printf("\tnl\tCR is sent as NL\n");
	    printf("\tquit\tQuit yamst\n");
	    continue;
	}

	if (!strcmp(command, "cr")) {
	    printf("CR will be sent as CR\n");
	    terminal_state_raw.c_iflag = 0;
	    continue;
	}

	if (!strcmp(command, "nl")) {
	    printf("CR will be sent as NL\n");
	    terminal_state_raw.c_iflag = ICRNL;
	    continue;
	}

	if (!strcmp(command, "q") || !strcmp(command, "quit")) {
	    exit(0);
	}

	printf("Invalid command: '%s'\n", command);
    }

    terminal_raw();
}




/* called when the socket was disconnected. will wait for or try to
 * make a new connection
 */
void handle_disconnect() {
    char b;

    close(fd);

    /* listening socket, wait for the next connection */
    if (LISTEN) {
	printf("\n>>> Remote disconnect\n");
	terminal_restore(); /* in normal mode so CTRL-C works */
	if (EXIT_ON_CLOSE)
           clean_exit(0);

	if (UNIX_SOCKET) {
	    fd = listen_unix(remote_name);
	    if (fd < 0) {
		fprintf(stderr, "\n*** Error listening unix socket '%s': ",
			remote_name);
		perror("");
		clean_exit(1);
	    }
	} else {
	    fd = listen_inet(remote_port);
	    if (fd < 0) {
		fprintf(stderr, "\n*** Error listening inet port %u: ",
			remote_port);
		perror("");
		clean_exit(1);
	    }
	}
	printf("  Connected\n");
	terminal_raw();
    } else { /* connecting socket, wait for input before reconnecting */
	printf("\n>>> Remote disconnect. ESC or q to quit, other to reconnect\n");
	read(0, &b, 1);

	if (b == 'q' || b == 'Q' || b == 27)
	    clean_exit(0);

	terminal_restore(); /* normal mode for CTRL-C */
	if (UNIX_SOCKET) {
	    fd = connect_unix(remote_name);
	    if (fd < 0) {
		fprintf(stderr, "\n*** Error connecting to unix socket'%s': ",
			remote_name);
		perror("");
		clean_exit(1);
	    }
	} else { /* inet socket */
	    fd = connect_inet(remote_name, remote_port);
	    if (fd < 0) {
		fprintf(stderr, "\n*** Error connecting %s:%u: ",
			remote_name, remote_port);
		perror("");
		clean_exit(1);
	    }
	}
	printf("  Connected\n");
	terminal_raw();
    }

}



/* connect to a inet socket at host 'name', port 'port' and return the
 * file descriptor for the established connection
 */
int connect_inet(char *name, unsigned short port) {
    struct sockaddr_in addr;
    struct hostent *he;
    int f;

    if ((he=gethostbyname(name)) == NULL) {
	perror("\n*** Error resolving host name");
	clean_exit(1);
    }
    if ((f = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("\n*** Error creating INET socket");
	clean_exit(1);
    }

    printf(">>> Connecting to %s:%d ...", name, port);
    fflush(stdout);

    addr.sin_family = AF_INET;      /* host byte order */
    addr.sin_port = htons(port);  /* short, network byte order */
    addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(addr.sin_zero), 0, 8);     /* zero the rest of the struct */

    if (connect(f, (struct sockaddr *)&addr, sizeof(struct sockaddr))) {
	close(f);
	return -1;
    }

    return f;
}

/* listen to a inet socket at port 'port' and return the file
 * descriptor for the accepted connection
 */
int listen_inet(unsigned short port) {
    struct sockaddr_in addr;
    int f, af;
    int opt_true = 1;

    if ((f = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("\n*** Error creating INET socket");
	clean_exit(1);
    }

    /* enable immediate reconnects: */
    if(setsockopt(f,
		  SOL_SOCKET,
		  SO_REUSEADDR,
		  &opt_true,
		  sizeof(opt_true))
       < 0) {
	perror("\n*** Error setting socket options");
	clean_exit(1);
    }

    printf(">>> Waiting for incoming connection at port %d ...", port);
    fflush(stdout);

    addr.sin_family = AF_INET;      /* host byte order */
    addr.sin_port = htons(port);  /* short, network byte order */
    addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(addr.sin_zero), 0, 8);     /* zero the rest of the struct */

    if (bind(f,
	     (struct sockaddr *)&addr,
	     sizeof(struct sockaddr))
	== -1) {
	close(f);
	return -1;
    }

    if (listen(f, 0)) {
	close(f);
	return -1;
    }


    if ((af = accept(f, NULL, NULL)) == 1) {
	perror("\n*** Error accepting connection");
	clean_exit(1);
    }

    close(f);
    return af;
}

/* connect to a unix socket at file 'name' and return the file
 * descriptor for the established connection
 */
int connect_unix(char *name) {
    struct sockaddr_un addr;
    int f;

    if ((f = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
	perror("\n*** Error creating UNIX socket");
	clean_exit(1);
    }

    printf(">>> Connecting to socket '%s' ...", name);
    fflush(stdout);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, name, sizeof(addr.sun_path));

    if (connect(f, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	close(f);
	return -1;
    }

    return f;
}

/* listen to a unix socket at file 'name' and return the file
 * descriptor for the accepted connection
 */
int listen_unix(char *name) {
    struct sockaddr_un addr;
    int f, af;
    int opt_true = 1;
    struct stat st;

    if ((f = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
	perror("\n*** Error creating UNIX socket");
	clean_exit(1);
    }

    /* enable immediate reconnects: */
    if(setsockopt(f,
		  SOL_SOCKET,
		  SO_REUSEADDR,
		  &opt_true,
		  sizeof(opt_true))
       < 0) {
	perror("\n*** Error setting socket options");
	clean_exit(1);
    }

    /* don't destroy any nonsocket files: */
    if (!stat(name, &st) && !S_ISSOCK(st.st_mode)) {
	fprintf(stderr, "\n*** Cannot listen, '%s' is not a socket file\n", name);
	clean_exit(1);
    }

    /* remove old socket file */
    if (unlink(name) == -1 && errno != ENOENT) {
	fprintf(stderr, "\n*** Error removing old socket file '%s': ", name);
	perror("");
	clean_exit(1);
    }

    printf(">>> Waiting for incoming connection on socket '%s' ...", name);
    fflush(stdout);

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, name, sizeof(addr.sun_path));

    if(bind(f, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	close(f);
	return -1;
    }

    if(listen(f, 1) < 0) {
	close(f);
	return -1;
    }

    af = accept(f, NULL, 0);

    if(af < 0) {
	perror("\n*** Error accepting connection");
	clean_exit(1);
    }

    close(f);
    return af;
}
