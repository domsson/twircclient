#include <stdio.h>      // NULL, fprintf(), perror()
#include <string.h>     // strcmp()
#include <stdlib.h>     // NULL, EXIT_FAILURE, EXIT_SUCCESS
#include <errno.h>      // errno
#include <unistd.h>     // getopt() et al.
#include <sys/types.h>  // ssize_t
#include <signal.h>
#include <time.h>
#include "libtwirc.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_BUILD 0

#define PROJECT_URL "https://github.com/domsson/twircclient"

#define DEFAULT_HOST "irc.chat.twitch.tv"
#define DEFAULT_PORT "6667"
#define DEFAULT_TIMESTAMP "[%H:%M:%S]"
#define TIMESTAMP_BUFFER 64

static volatile int running; // Used to stop main loop in case of SIGINT etc
static volatile int handled; // The last signal that has been handled

struct metadata
{
	char *chan;           // Channel to join
	char *timestamp;      // Timestamp format
	int   verbose;        // Print additional info
};

/*
 * Constructs a timestamp according to the timestamp format saved in the metadata
 * that should be in the state's context and adds a space to it, so it can be 
 * directly used as a prefix for chat messages. If there is no timestamp format,
 * it simply puts an empty string into the provided buffer.
 */
char *timestamp_prefix(twirc_state_t *s, char *buf, size_t len)
{
	// Initialize to empty string
	buf[0] = '\0';

	// Get the metadata from the state context
	struct metadata *meta = twirc_get_context(s);

	// Stop if we're not supposed to show a timestamp
	if (!meta->timestamp)
	{
		return buf;
	}

	// Let's get the current time for a nice timestamp
	time_t t = time(NULL);
	struct tm lt = *localtime(&t);

	// Let's run the time through strftime() for format
	strftime(buf, len - 1, meta->timestamp, &lt);

	// Let's make sure there is a space after the string
	size_t actual_len = strlen(buf);
	buf[actual_len] = ' '; // Add a space, overwriting null terminator
	buf[actual_len+1] = 0; // Add null terminator again

	return buf;
}


/*
 * Called once the connection has been established. This does not mean we're
 * authenticated yet, hence we should not attempt to join channels yet etc.
 */
void handle_connect(twirc_state_t *s, twirc_event_t *evt)
{
	struct metadata *meta = twirc_get_context(s);
	if (meta->verbose)
	{
		fprintf(stderr, "*** Connected\n");
	}
}

/*
 * Called once we're authenticated. This is where we can join channels etc.
 */
void handle_welcome(twirc_state_t *s, twirc_event_t *evt)
{
	struct metadata *meta = twirc_get_context(s);
	if (meta->verbose)
	{
		fprintf(stderr, "*** Authenticated\n");
	}

	// Let's join the specified channel
	twirc_cmd_join(s, meta->chan);
}

/*
 * Called once we see a user join a channel we're in. This also fires for when
 * we join a channel, in which case 'evt->origin' will be our own username.
 */
void handle_join(twirc_state_t *s, twirc_event_t *evt)
{
	twirc_login_t *login = twirc_get_login(s);

	if (!evt->origin)
	{
		return;
	}
	if (!login->nick)
	{
		return;
	}
	if (strcmp(evt->origin, login->nick) != 0)
	{
		return;
	}

	struct metadata *meta = twirc_get_context(s);
	if (meta->verbose)
	{
		fprintf(stderr, "*** Joined %s\n", evt->channel);
	}
}

/*
 * Called when a user sends a message to a channel. In other words, chat!
 * 'evt->origin' will contain the username of the person who sent the message,
 * 'evt->channel' will contain the channel the message was sent to,
 * 'evt->message' will contain the actual chat message.
 */
void handle_privmsg(twirc_state_t *s, twirc_event_t *evt)
{
	char buf[TIMESTAMP_BUFFER];
	fprintf(stdout, "%s%s: %s\n",
				timestamp_prefix(s, buf, TIMESTAMP_BUFFER),
				evt->origin,
				evt->message);
}

/*
 * Called when a user uses the "/me" command in a channel.
 * This is pretty much the same as the privmsg, just that the output is usually
 * stylized to differentiate it from regular chat messages.
 */
void handle_action(twirc_state_t *s, twirc_event_t *evt)
{
	char buf[TIMESTAMP_BUFFER];
	fprintf(stdout, "%s* %s %s\n",
				timestamp_prefix(s, buf, TIMESTAMP_BUFFER),
				evt->origin,
				evt->message);
}

/*
 * Called when a loss of connection has been detected. This could be due to 
 * a connection error or because Twitch closed the connection on us.
 */
void handle_disconnect(twirc_state_t *s, twirc_event_t *evt)
{
	struct metadata *meta = twirc_get_context(s);
	if (meta->verbose)
	{
		fprintf(stderr, "*** Disconnected\n");
	}
}

/*
 * Let's handle CTRL+C by stopping our main loop and tidying up.
 */
void sigint_handler(int sig)
{
	running = 0;
	handled = sig;
}

void version()
{
	fprintf(stdout, "twitch-dump version %d.%d.%d - %s\n",
				VERSION_MAJOR,
				VERSION_MINOR,
				VERSION_BUILD,
				PROJECT_URL);
}

void help(char *invocation)
{
	fprintf(stdout, "Usage:\n");
	fprintf(stdout, "\t%s [OPTION...] -c CHANNEL\n", invocation);
	fprintf(stdout, "\t Note: the channel should start with '#'\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "Options:\n");
	fprintf(stdout, "\t-h Print this help text and exit.\n");
	fprintf(stdout, "\t-s Print additional status information to stderr.\n");
	fprintf(stdout, "\t-t FORMAT Enable timestamps, optionally specifying the format.\n");
	fprintf(stdout, "\t-v Print version information and exit.\n");
	fprintf(stdout, "\n");
	version();
}

/*
 * Main - this is where we make things happen!
 */
int main(int argc, char **argv)
{
	// Get a metadata struct	
	struct metadata m = { 0 };

	// Process command line options
	opterr = 0;
	int o;
	while ((o = getopt(argc, argv, "c:t:svh")) != -1)
	{
		switch(o)
		{
			case 'c':
				m.chan = optarg;
				break;
			case 't':
				m.timestamp = optarg;
				break;
			case 's':
				m.verbose = 1;
				break;
			case 'v':
				version();
				return EXIT_SUCCESS;
			case 'h':
				help(argv[0]);
				return EXIT_SUCCESS;
		}
	}

	// Abort if no channel name was given	
	if (m.chan == NULL)
	{
		fprintf(stderr, "No channel specified, exiting\n");
		return EXIT_FAILURE;
	}

	if (m.verbose)
	{
		fprintf(stderr, "*** Initializing\n");
	}
	
	// Make sure we still do clean-up on SIGINT (ctrl+c)
	// and similar signals that indicate we should quit.
	struct sigaction sa_int = {
		.sa_handler = &sigint_handler
	};
	
	// These might return -1 on error, but we'll ignore that for now
	sigaction(SIGINT, &sa_int, NULL);
	sigaction(SIGQUIT, &sa_int, NULL);
	sigaction (SIGTERM, &sa_int, NULL);

	// Create libtwirc state instance
	twirc_state_t *s = twirc_init();

	if (s == NULL)
	{
		fprintf(stderr, "Error initializing, exiting\n");
		return EXIT_FAILURE;
	}
	
	// Save the metadata in the state
	twirc_set_context(s, &m);

	// We get the callback struct from the libtwirc state
	twirc_callbacks_t *cbs = twirc_get_callbacks(s);

	// We assign our handlers to the events we are interested int
	cbs->connect         = handle_connect;
	cbs->welcome         = handle_welcome;
	cbs->join            = handle_join;
	cbs->action          = handle_action;
	cbs->privmsg         = handle_privmsg;
	cbs->disconnect      = handle_disconnect;
	
	// Connect to the IRC server
	if (twirc_connect_anon(s, DEFAULT_HOST, DEFAULT_PORT) != 0)
	{
		fprintf(stderr, "Error connecting, exiting\n");
		return EXIT_FAILURE;
	}

	// Main loop - we call twirc_tick() every go-around, as that's what 
	// makes the magic happen. The 1000 is a timeout in milliseconds that
	// we grant twirc_tick() to do its work - in other words, we'll give 
	// it 1 second to wait for and process IRC messages, then it will hand 
	// control back to us. If twirc_tick() detects a disconnect or error,
	// it will return -1, otherwise it will return 0 and we can go on!

	running = 1;
	while (twirc_tick(s, 1000) == 0 && running == 1)
	{
		// Nothing to do here - it's all done via the event handlers
	}

	// twirc_kill() is a convenience functions that calls two functions:
	// - twirc_disconnect(), which makes sure the connection was closed
	// - twirc_free(), which frees the libtwirc state, so we don't leak
	twirc_kill(s);

	// That's all, wave good-bye!
	return EXIT_SUCCESS;
}

