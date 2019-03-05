#include <stdio.h>      // NULL, fprintf(), perror()
#include <stdlib.h>     // NULL, EXIT_FAILURE, EXIT_SUCCESS
#include <string.h>     //
#include <errno.h>      // errno
#include <sys/types.h>  // ssize_t
#include <signal.h>
#include <time.h>
#include "../inc/libtwirc.h"

#define NICK "kaulmate"
#define CHAN "#domsson"
#define HOST "irc.chat.twitch.tv"
#define PORT "6667"

static volatile int running; // Used to stop main loop in case of SIGINT etc
static volatile int handled; // The last signal that has been handled

/*
 * Read a file called 'token' (in the same directory as the code is run)
 * and read it into the buffer pointed to by buf. The file is exptected to
 * contain one line, and one line only: the Twitch oauth token, which should
 * begin with "oauth:". This is used as your IRC password on Twitch IRC.
 */
int read_token(char *buf, size_t len)
{
	FILE *fp;
	fp = fopen ("token", "r");
	if (fp == NULL)
	{
		return 0;
	}
	char *res = fgets(buf, len, fp);
	if (res == NULL)
	{
		fclose(fp);
		return 0;
	}
	size_t res_len = strlen(buf);
	if (buf[res_len-1] == '\n')
	{
		buf[res_len-1] = '\0';
	}
	fclose (fp);
	return 1;
}

/*
 * Called once the connection has been established. This does not mean we're
 * authenticated yet, hence we should not attempt to join channels yet etc.
 */
void handle_connect(struct twirc_state *s, struct twirc_event *evt)
{
	fprintf(stdout, "*** connected!\n");
}

/*
 * Called once we're authenticated. This is where we can join channels etc.
 */
void handle_welcome(struct twirc_state *s, struct twirc_event *evt)
{
	fprintf(stdout, "*** logged in!\n");

	// Let's join a channel
	fprintf(stdout, "*** joining %s\n", CHAN);
	twirc_cmd_join(s, CHAN);
}

/*
 * Called once we see a user join a channel we're in. This also fires for when
 * we join a channel, in which case 'evt->origin' will be our own username.
 */
void handle_join(struct twirc_state *s, struct twirc_event *evt)
{
	// Check if we just saw ourself joining our channel
	if (evt->origin && strcmp(evt->origin, NICK) == 0
	    && strcmp(evt->params[0], CHAN) == 0)
	{
		// privmsg actually sends a message to a channel
		twirc_cmd_privmsg(s, CHAN, "I'm alive!");
		// action performs the "/me" command
		twirc_cmd_action(s, CHAN, "might be the coolest bot ever");
	}
}

/*
 * Called when a user sends a message to a channel. In other words, chat!
 * 'evt->origin' will contain the username of the person who sent the message,
 * 'evt->channel' will contain the channel the message was sent to,
 * 'evt->message' will contain the actual chat message.
 */
void handle_privmsg(struct twirc_state *s, struct twirc_event *evt)
{
	// Let's get the current time for a nice timestamp
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	// Chat messages usually contain a 'color' tag which is the color that 
	// the user selected for their Twitch account, but could also be NULL
	char *color = twirc_tag_by_key(evt->tags, "color");

	// Let's print the chat message to the console!
	fprintf(stdout, "[%02d:%02d:%02d] [%s] (%s) %s: %s\n", 
			tm.tm_hour, tm.tm_min, tm.tm_sec, 
			color ? color : "default",	
			evt->channel, evt->origin, evt->message);
}

/*
 * Called when a user uses the "/me" command in a channel.
 * This is pretty much the same as the privmsg, just that the output is usually
 * stylized to differentiate it from regular chat messages.
 */
void handle_action(struct twirc_state *s, struct twirc_event *evt)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	char *color = twirc_tag_by_key(evt->tags, "color");
	
	fprintf(stdout, "[%02d:%02d:%02d] [%s] (%s) * %s %s\n",
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			color ? color : "default",
			evt->channel, evt->origin, evt->message);
}

/*
 * Called when we receive a whisper (private message).
 */
void handle_whisper(struct twirc_state *s, struct twirc_event *evt)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	fprintf(stdout, "[%02d:%02d:%02d] *** whisper from %s: %s\n",
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			evt->origin, evt->message);
	twirc_cmd_whisper(s, evt->origin, "Thanks, but I'm only a bot :-(");
}

/*
 * Called when a loss of connection has been detected. This could be due to 
 * a connection error or because Twitch closed the connection on us.
 */
void handle_disconnect(struct twirc_state *s, struct twirc_event *evt)
{
	fprintf(stdout, "*** connection lost\n");
}

/*
 * Let's handle CTRL+C by stopping our main loop and tidying up.
 */
void sigint_handler(int sig)
{
	fprintf(stderr, "*** received signal, exiting\n");
	running = 0;
	handled = sig;
}

/*
 * Main - this is where we make things happen!
 */
int main(void)
{
	// Output libtwirc version
	fprintf(stderr, "Starting up libtwirc test bot...");
	
	// Make sure we still do clean-up on SIGINT (ctrl+c)
	// and similar signals that indicate we should quit.
	struct sigaction sa_int = {
		.sa_handler = &sigint_handler
	};
	if (sigaction(SIGINT, &sa_int, NULL) == -1)
	{
		fprintf(stderr, "Failed to register SIGINT handler\n");
	}
	if (sigaction(SIGQUIT, &sa_int, NULL) == -1)
	{
		fprintf(stderr, "Failed to register SIGQUIT handler\n");
	}
	if (sigaction (SIGTERM, &sa_int, NULL) == -1)
	{
		fprintf(stderr, "Failed to register SIGTERM handler\n");
	}

	// Create libtwirc state instance
	struct twirc_state *s = twirc_init();
	
	if (s == NULL)
	{
		fprintf(stderr, "Could not init twirc state\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "Successfully initialized twirc state...\n");

	// We get the callback struct from the libtwirc state
	struct twirc_callbacks *cbs = twirc_get_callbacks(s);

	// We assign our handlers to the events we are interested int
	cbs->connect    = handle_connect;
	cbs->welcome    = handle_welcome;
	cbs->join       = handle_join;
	cbs->action     = handle_action;
	cbs->privmsg    = handle_privmsg;
	cbs->whisper    = handle_whisper;
	cbs->disconnect = handle_disconnect;
	
	// Read in the token file (oauth token / IRC password)
	char token[128];
	int token_success = read_token(token, 128);
	if (token_success == 0)
	{
		fprintf(stderr, "Could not read token file\n");
		return EXIT_FAILURE;
	}

	// Connect to the IRC server
	if (twirc_connect(s, HOST, PORT, NICK, token) != 0)
	{
		fprintf(stderr, "Could not connect to Twitch IRC\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "Connection initiated...\n");

	// Main loop - we call twirc_tick() every go-around, as that's what 
	// makes the magic happen. The 1000 is a timeout in milliseconds that
	// we grant twirc_tick() to do its work - in other words, we'll give 
	// it 1 second do wait for and process IRC messages, then it will hand 
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
	fprintf(stderr, "Bye!\n");
	return EXIT_SUCCESS;
}

