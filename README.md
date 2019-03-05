# twircclient

Example code that shows how to use [`libtwirc`](https://github.com/domsson/libtwirc)

## `client.c`

A simple client that connectes to Twitch IRC and outputs all incoming messages on the console. Reads user input and sends it to the IRC server. A great little tool to play around with the available IRC commands and what the Twitch IRC messages look like, exactly.

## `kaul.c`

A simple bot that connects to Twitch IRC. Not yet ready. Stick with `client.c` for now.

# How to

- Clone [`libtwirc`](https://github.com/domsson/libtwirc) to somewhere
- Build `libtwirc` with the provided `build-shared` script
- Copy the resulting `.so` and `.h` files from the `lib` dir of libtwirc to the `inc` dir of twircclient (you can use twircclient's `./fetch` for this, if both repositories reside within the same parent directory)
- Create a `token` file within twircclient's directory and place your Twitch oauth-token in it
- Open `src/client.c` and change `"kaulmate"` (line 11 or somesuch) to your (or your bot's) Twitch username (the one that goes with the oauth token you are using)
- Run `./build`
- Run `export LD_LIBRARY_PATH=$(pwd)/inc:$LD_LIBRARY_PATH` so that your machine knows where to find the library files (alternatively, place the library files into the appropriate directories for your system)
- Run `./bin/client`
- Profit, maybe
