/* ***	$URL: https://svn.uv-software.com/opensource/linux/socketCAN/utilities/can_open/trunk/main.c $  ***
 *
 *	project   :  CAN - Controller Area Network.
 *
 *	purpose   :  CANopen Commandline Tool (using berliOS socketCAN)
 *
 *	copyright :  (C) 2008-2009, UV Software, Friedrichshafen.
 *
 *	compiler  :  GCC - GNU C Compiler (Linux Kernel 2.6)
 *
 *	syntax    :  can_open <interface> [<option>...]
 *	             Options:
 *	               -g, --gateway=<port>         operate in gateway mode on <port>
 *	               -t, --timeout=<seconds>      time-out in seconds (client only)
 *	               -b, --baudrate=<baudrate>    bit timing in kbps (default=250)
 *	               -i, --id=<node-id>           node-id of CANopen Master (default=-1)
 *	                   --net=<network>          set default network number (default=1)
 *	                   --node=<node-id>         set default node-id (default=1)
 *	                   --echo                   echo input stream to output stream
 *	                   --prompt                 prefix input stream with a prompt
 *	                   --syntax                 show input syntax and exit
 *	               -h, --help                   display this help and exit
 *	                   --version                show version information and exit
 *
 *	libraries :  (none)
 *
 *	includes  :  default.h, cop_tcp.h, cop_api.h, can_defs.h
 *
 *	author    :  Uwe Vogt, UV Software, Friedrichshafen
 *
 *	e-mail    :  uwe.vogt@uv-software.de
 *
 *
 *	-----------  description  -----------------------------------------------
 *
 *	CANopen Commandline Tool (using berliOS socketCAN)
 *
 *	Enter option --syntax at the commandline to get a syntax summery.
 *	For information about socketCAN see "http://socketcan.berlios.de/"
 *
 *	See also:
 *	[DS301] CANopen application layer and communication profile, version 4.02
 *	[DS309] Interfacing CANopen with TCP/IP, part 1 and 3, version 1.1
 */

static const char* __copyright__ = "Copyright (C) 2008-2009 UV Software, Friedrichshafen";
static const char* __version__   = "0.2";
static const char  _rev[] = "$Rev: 34 $";

/* ***	includes  ***
 */

#include "can_defs.h"
#include "cop_api.h"
#include "cop_tcp.h"
#include "default.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <linux/can.h>


/* ***	options  ***
 */

#define __no_can_ioctl
#define __single_channel


/* ***	defines  ***
 */

#define NODE_ID			255
#define DEFAULT_NET		1
#define DEFAULT_NODE	1
#define MODE_LOCAL		0
#define MODE_REMOTE		1
#define MODE_GATEWAY	2
#define BUFFER_LENGTH	1025
#define SEQUENCE_NO		1
#define TIMEOUT			66


/* ***	types  ***
 */


/* ***	prototypes  ***
 */

void sigterm(int signo);
void usage(FILE *stream, char *program);
void version(FILE *stream, char *program);

void syntax(FILE *stream, char *program);
ssize_t readline(int fd, char *buf, size_t nbyte);

/* ***	variables  ***
 */

static int client = -1, server = -1;
static int running = 1;


/* ***	main  ***
 */

int main(int argc, char *argv[])
/*
 * function : main function of the application.
 *
 * parameter: argc - number of command line arguments.
 *            argv - command line arguments as string vector.
 *
 * result   : 0    - no error occured.
 */
{
	int    opt, echo = 0, prompt = 0; 
	long   baudrate = 3; int bd = 0;
	int    node_id = NODE_ID; int id = 0;
	int    default_net = DEFAULT_NET; int net = 0;
	int    default_node = DEFAULT_NODE; int node = 0;
	int    gateway = 0; int gw = 0;
	//long   timeout = TIMEOUT; int to = 0;
	int    mode = MODE_LOCAL;
	long   ip1 = 127, ip2 = 0, ip3 = 0, ip4 = 1;	
	long   port = 0;
	long   sequence = SEQUENCE_NO;
	long   request = 0, response = 0;
	char   buffer[BUFFER_LENGTH];
	long   rc; ssize_t n; int on;
	struct sockaddr_in addr;
	char  *device, *firmware, *software;
		
	struct option long_options[] = {
		{"baudrate", required_argument, 0, 'b'},
		{"id", required_argument, 0, 'i'},
		{"net", required_argument, 0, 'N'},
		{"node", required_argument, 0, 'n'},
		{"echo", no_argument, 0, 'e'},
		{"prompt", no_argument, 0, 'p'},
		{"syntax", no_argument, 0, 's'},
		{"gateway", required_argument, 0, 'g'},
		//{"timeout", required_argument, 0, 't'},
		{"help", no_argument, 0, 'h'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};
	struct _can_param can_param = {"can0", PF_CAN, SOCK_RAW, CAN_RAW};
	struct _cop_tcp_settings settings = {DEFAULT_NET, DEFAULT_NODE, NODE_ID};
	
	signal(SIGINT, sigterm);	
	signal(SIGHUP, sigterm);	
	signal(SIGTERM, sigterm);	

	while((opt = getopt_long(argc, argv, "b:i:g:h", long_options, NULL)) != -1) {
		switch(opt) {
			case 'b':
				if(bd++) {
					fprintf(stderr, "+++ error: conflict in option -- b\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if(optarg && optarg[0] == '-') {
					fprintf(stderr, "+++ error: missing argument in option -- b\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if(sscanf(optarg, "%li", &baudrate) != 1) {
					fprintf(stderr, "+++ error: illegal argument in option -- b\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				switch(baudrate) {
					case 1000: baudrate = 0; break;
					case 800:  baudrate = 1; break;
					case 500:  baudrate = 2; break;
					case 250:  baudrate = 3; break;
					case 125:  baudrate = 4; break;
					case 100:  baudrate = 5; break;
					case 50:   baudrate = 6; break;
					case 20:   baudrate = 7; break;
					case 10:   baudrate = 8; break;
				}
				if((baudrate < 0) || (8 < baudrate)) {
					fprintf(stderr, "+++ error: illegal value in option -- b\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
#ifdef __no_can_ioctl
 fprintf(stderr, "+++ sorry: unsupported option -- b\n");
 return 1;
#endif
				break;
			case 'i':
				if(id++) {
					fprintf(stderr, "+++ error: conflict in option -- i\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if(optarg && optarg[0] == '-') {
					fprintf(stderr, "+++ error: missing argument in option -- i\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if(sscanf(optarg, "%i", &node_id) != 1) {
					fprintf(stderr, "+++ error: illegal argument in option -- i\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if((node_id < 1) || (127 < node_id)) {
					fprintf(stderr, "+++ error: illegal argument in option -- i\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				settings.master = (unsigned char)node_id;
				break;
			case 'n':
				if(node++) {
					fprintf(stderr, "+++ error: conflict in option -- node\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if(optarg && optarg[0] == '-') {
					fprintf(stderr, "+++ error: missing argument in option -- node\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if(sscanf(optarg, "%i", &default_node) != 1) {
					fprintf(stderr, "+++ error: illegal argument in option -- node\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if((default_node < 1) || (127 < default_node)) {
					fprintf(stderr, "+++ error: illegal argument in option -- node\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				settings.node = (unsigned char)default_node;
				break;
			case 'N':
				if(net++) {
					fprintf(stderr, "+++ error: conflict in option -- net\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if(optarg && optarg[0] == '-') {
					fprintf(stderr, "+++ error: missing argument in option -- net\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if(sscanf(optarg, "%i", &default_node) != 1) {
					fprintf(stderr, "+++ error: illegal argument in option -- net\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
#ifndef __single_channel				
				if((default_net < 1) || (255 < default_net)) {
#else
				if((default_net != DEFAULT_NET)) {
#endif
					fprintf(stderr, "+++ error: illegal argument in option -- net\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				settings.net = (unsigned char)default_net;
				break;
			case 'g':
				if(gw++) {
					fprintf(stderr, "+++ error: conflict in option -- g\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if(optarg && optarg[0] == '-') {
					fprintf(stderr, "+++ error: missing argument in option -- g\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if(sscanf(optarg, "%li", &port) != 1) {
					fprintf(stderr, "+++ error: illegal argument in option -- g\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if((port < 0) || (65535 < port)) {
					fprintf(stderr, "+++ error: illegal argument in option -- g\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				gateway = 1;
				break;
			/* *** **
			case 't':
				if(to++) {
					fprintf(stderr, "+++ error: conflict in option -- t\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if(optarg && optarg[0] == '-') {
					fprintf(stderr, "+++ error: missing argument in option -- t\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if(sscanf(optarg, "%li", &timeout) != 1) {
					fprintf(stderr, "+++ error: illegal argument in option -- t\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if(timeout < 0) {
					fprintf(stderr, "+++ error: illegal argument in option -- t\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				break;
				** *** */
			case 'e':
				echo = 1;
				break;
			case 'p':
				prompt = 1;
				break;
			case 's':
				syntax(stdout, basename(argv[0]));
				return 0;
			case 'v':
				version(stdout, basename(argv[0]));
				return 0;
			case 'h':
			default:
				usage(stderr, basename(argv[0]));
				return 1;
		}
	}
	if(argc == optind) {
		fprintf(stderr, "+++ error: no interface given\n");
		usage(stderr, basename(argv[0]));
		return 1;
	}
	if(argc >= optind + 2) {
		fprintf(stderr, "+++ error: too many arguments\n");
		usage(stderr, basename(argv[0]));
		return 1;
	}
	else {
		if(!gateway) {
			if(sscanf(argv[optind], "%li.%li.%li.%li:%li", &ip1, &ip2, &ip3, &ip4, &port) == 5) {
				if((ip1 < 0) || (255 < ip1)) {
					fprintf(stderr, "+++ error: illegal ip address\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if((ip2 < 0) || (255 < ip2)) {
					fprintf(stderr, "+++ error: illegal ip address\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if((ip3 < 0) || (255 < ip3)) {
					fprintf(stderr, "+++ error: illegal ip address\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if((ip4 < 0) || (255 < ip4)) {
					fprintf(stderr, "+++ error: illegal ip address\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				if((port < 0) || (65535 < port)) {
					fprintf(stderr, "+++ error: illegal port number\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				mode = MODE_REMOTE;
			}
			else if(sscanf(argv[optind], "localhost:%li", &port) == 1) {
				if((port < 0) || (65535 < port)) {
					fprintf(stderr, "+++ error: illegal port number\n");
					usage(stderr, basename(argv[0]));
					return 1;
				}
				mode = MODE_REMOTE;
			}
			else {
				can_param.ifname = argv[optind];
				mode = MODE_LOCAL;
			}
		}
		else {
			can_param.ifname = argv[optind];
			mode = MODE_GATEWAY;
		}
	}
	if(mode == MODE_REMOTE && node) {
		fprintf(stderr, "+++ error: conflict in option -- node\n");
		usage(stderr, basename(argv[0]));
		return 1;
	}
	if(mode == MODE_REMOTE && net) {
		fprintf(stderr, "+++ error: conflict in option -- net\n");
		usage(stderr, basename(argv[0]));
		return 1;
	}
	if(mode == MODE_REMOTE && id) {
		fprintf(stderr, "+++ error: conflict in option -- id\n");
		usage(stderr, basename(argv[0]));
		return 1;
	}
	/* *** **
	if(mode != MODE_REMOTE && to) {
		fprintf(stderr, "+++ error: conflict in option -- t\n");
		usage(stderr, basename(argv[0]));
		return 1;
	}
	** *** */
	switch(mode) {
	case MODE_GATEWAY:
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons((unsigned short)port);
		if((server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("+++ error(socket)");
			return 1;
		}
		if(setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
			perror("+++ error(setsockopt)");
			return 1;
		}
		if(bind(server, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			perror("+++ error(bind)");
			close(server);
			return 1;
		}
		if(listen(server, 1) < 0) {
			perror("+++ error(listen)");
			close(server);
			return 1;
		}
		if((rc = cop_init(CAN_NETDEV, &can_param, (BYTE)baudrate)) != 0) {
			fprintf(stderr, "+++ error: cop_init = %li\n", rc);
			close(server);
			return 1;
		}
		fprintf(stderr, "Interfacing CANopen with TCP/IP acc. DS-309/3: port=%li\n", port);
		if(((device = cop_hardware()) != NULL) &&
		   ((firmware = cop_software()) != NULL) &&
		   ((software = cop_version()) != NULL)) {
			fprintf(stdout, "Hardware: %s\n" \
							"Firmware: %s\n" \
							"Software: %s\n" \
							"%s.\n",
					device, firmware, software,__copyright__);
		}
		fprintf(stderr, "\nPress ^C to abort.\n\n");
		
		while(running && (client = accept(server, NULL, NULL)) >= 0) {
			while(running && (n = readline(client, &buffer[0], BUFFER_LENGTH)) > 0) {
				if(echo) {
					fputs(&buffer[0], stdout);
				}
				if(strcmp(buffer, "\r\n") && strcmp(buffer, "\n")) {
					cop_tcp_parse(&buffer[0], &settings, &buffer[0], BUFFER_LENGTH);
					if(write(client, buffer, strlen(buffer)) < 0) {
						perror("+++ error(write)");
					}
					else if(echo) {
						fputs(&buffer[0], stdout);
					}
				}
			}
			if(running && n < 0 && errno != ECONNRESET) {
				perror("+++ error(read)");
			}
			close(client);
			client = -1;
		}
		close(server);
		cop_exit();
		fprintf(stderr, "Port %li closed.\n", port);
		break;
	case MODE_REMOTE:
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl((unsigned long)(ip1 * (256*256*256) + ip2 * (256*256) + ip3 * (256) + ip4));
		addr.sin_port = htons((unsigned short)port);
		if((client = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("+++ error(socket)");
			return 1;
		}
		if(connect(client, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			perror("+++ error(connect)");
			close(client);
			return 1;
		}
		while(running && !feof(stdin)) {
			if(prompt) {
				sprintf(buffer, "[%li] ", sequence++);
				fputs(&buffer[0], stdout);
				fflush(stdout);
			}
			else {
				buffer[0] = '\0';
			}
			if(fgets(&buffer[strlen(buffer)], BUFFER_LENGTH-strlen(buffer), stdin) != NULL) {
				if(echo) {
					fputs(&buffer[0], stdout);
				}
				if(write(client, buffer, strlen(buffer)) > 0) {
					if(sscanf(buffer, "[%li]", &request) != 1) {
						request = 0;
					}
					do {
						if(running && readline(client, buffer, BUFFER_LENGTH) > 0) {
							if(sscanf(buffer, "[%li]", &response) != 1) {
								response = 0;
							}
							fputs(&buffer[0], stdout);
						}
						else {
							break;
						}
					}	while (running && request != response);
				}
				else {
					perror("+++ error(write)");
				}
			}
			else if(ferror(stdin)) {
				perror("+++ error(stdin)");
			}
		}
		fprintf(stdout, "\n");
		close(client);
		break;
	case MODE_LOCAL:
		if((rc = cop_init(CAN_NETDEV, &can_param, (BYTE)baudrate)) != 0) {
			fprintf(stderr, "+++ error: cop_init = %li\n", rc);
			return 1;
		}
		while(running && !feof(stdin)) {
			if(prompt) {
				sprintf(buffer, "[%li] ", sequence++);
				fputs(&buffer[0], stdout);
				fflush(stdout);
			}
			else {
				buffer[0] = '\0';
			}
			if(fgets(&buffer[strlen(buffer)], BUFFER_LENGTH-strlen(buffer), stdin) != NULL) {
				if(echo) {
					fputs(&buffer[0], stdout);
				}
				if(strcmp(buffer, "\r\n") && strcmp(buffer, "\n")) {
					cop_tcp_parse(&buffer[0], &settings, &buffer[0], BUFFER_LENGTH);
					fputs(&buffer[0], stdout);
				}
			}
			else if(ferror(stdin)) {
				perror("+++ error(stdin)");
			}
		}
		fprintf(stdout, "\n");
		cop_exit();
		break;
	default:
		usage(stderr, basename(argv[0]));
		return 1;
	}
	return 0;
}

/* ***	functions  ***
 */

void sigterm(int signo)
{
	/*printf("got signal %d\n", signo);*/
	if(client != -1)
		close(client);
	if(server != -1)
		close(server);
	running = 0;
}

void version(FILE *stream, char *program)
{
	int rev; if(sscanf(_rev, "\044Rev: %i \044", &rev) != 1) rev = 0;
	
	fprintf(stream, "%s (CANopen using socketCAN), version %s.%i of %s\n%s.\n\n",
							     program,__version__, rev,__DATE__,__copyright__);
/* *** GPL *** **
	fprintf(stream, "This is free software. You may redistribute copies of it under the terms of\n");
	fprintf(stream, "the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n");
	fprintf(stream, "There is NO WARRANTY, to the extent permitted by law.\n\n");
** *** end *** */
	fprintf(stream, "This software is freeware without any warranty or support!\n\n");

	fprintf(stream, "Written by Uwe Vogt, UV Software <http://www.uv-software.de/>\n");
}

void usage(FILE *stream, char *program)
{
	fprintf(stream, "Usage: %s <interface> [<option>...]\n", program);
	fprintf(stream, "Options:\n");
	fprintf(stream, " -g, --gateway=<port>         operate in gateway mode on <port>\n");
	//fprintf(stream, " -t, --timeout=<seconds>      time-out in seconds (client only)\n");
#ifndef __no_can_ioctl
	fprintf(stream, " -b, --baudrate=<baudrate>    bit timing in kbps (default=250)\n");
#endif
	fprintf(stream, " -i, --id=<node-id>           node-id of CANopen Master (default=%i)\n",(char)NODE_ID);
	fprintf(stream, "     --net=<network>          set default network number (default=%u)\n",DEFAULT_NET);
	fprintf(stream, "     --node=<node-id>         set default node-id (default=%u)\n",DEFAULT_NODE);
	fprintf(stream, "     --echo                   echo input stream to output stream\n");
	fprintf(stream, "     --prompt                 prefix input stream with a prompt\n");
	fprintf(stream, "     --syntax                 show input syntax and exit\n");
	fprintf(stream, " -h, --help                   display this help and exit\n");
	fprintf(stream, "     --version                show version information and exit\n");
	fprintf(stream, "Press ^D to leave the interactive input mode.\n");
}

void syntax(FILE *stream, char *program)
{
	int rev; if(sscanf(_rev, "\044Rev: %i \044", &rev) != 1) rev = 0;
	
	fprintf(stream, "Syntax for %s (CANopen Commandline Tool):\n\n", program);
	cop_tcp_syntax(stream);
	fprintf(stream, "(%s)\n\n", cop_tcp_version());
	fprintf(stream, "%s (CANopen using socketCAN), version %s.%i of %s\n%s.\n\n",
							     program,__version__, rev,__DATE__,__copyright__);
}

ssize_t readline(int fd, char *buf, size_t nbyte)
{
	char chr; ssize_t pos = 0, res;
	while((res = read(fd, &chr, sizeof(chr))) > 0) {
		if(pos + 1 < nbyte) {
			buf[pos++] = chr;
			buf[pos] = 0;
		}
		if(chr == '\n') {
			//@ToDo: CRLF
			return pos;
		}
		if(!running) {
			return 0;
		}
	}
	if(res == 0)
		return pos;
	else
		return res;
}

/* ***	end of file  ***
 */
