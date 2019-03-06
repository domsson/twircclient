# twircclient

Example code that shows how to use [`libtwirc`](https://github.com/domsson/libtwirc)

## `bot.c`

A simple bot that connects to Twitch IRC and joins a channel. The source is well-commented, check it out.

## `client.c`

A simple client that connectes to Twitch IRC and outputs all incoming messages on the console. Reads user input and sends it to the IRC server.

# How to

1. Clone `twircclient` and [`libtwirc`](https://github.com/domsson/libtwirc):

````
git clone https://github.com/domsson/twircclient
git clone https://github.com/domsson/libtwirc
````

2. Build `libtwirc` with the provided `build-shared` script. It will output the `.so` and `.h` file into the `lib` subdirectory:

```
cd libtwirc
chmod +x build-shared
./build-shared
```

3. Install `libtwirc`. This will vary depending on your distro and machine, as the include paths aren't the same for all. You'll have to look up where your distro wants shared library and header files. In any case, after copying the files to their appropriate directories, you should call `ldconfig` so that your system learns about the new library. For my amd64 Debian install, this does the trick (requires super user permissions):

```
cp lib/libtwirc.so /usr/lib/x86_64-linux-gnu/
cp lib/libtwirc.h /usr/include/
ldconfig -v -n /usr/lib
```

4. Go over to the `twircclient` directory, create a token file and place your [Twitch oauth token](https://twitchapps.com/tmi/) in there:

```
cd ../twircclient
touch token
echo "oauth:YOUR-TOKEN-HERE" > token
```

5. Open `src/bot.c` or `src/client.c` (depending on which one you want to run) and edit the `#define`s to meet your needs:

```
#define NICK "your-twitch-username-here"
#define CHAN "#your-channel-here"
```

6. Compile the bot and/or client:

```
chmod +x build-bot
chmod +x build-client
./build-bot
./build-client
```

8. Run the bot and/or client:

```
./bin/bot
./bin/client
```
