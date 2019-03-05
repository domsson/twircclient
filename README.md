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

3. Go over to the `twircclient` directory and copy the compiled library files into its `inc` directory:

```
cd ../twircclient/
cp ../libtwirc/lib/* ./inc/
```

4. Create a token file and place your [Twitch oauth token](https://twitchapps.com/tmi/) in there:

```
touch token
echo "oauth:YOUR-TOKEN-HERE" > token
```

5. Open `src/bot.c` or `src/client.c` (depending on which one you want to run) and edit the `#define NICK` and `#define CHAN` lines to your needs.

6. Compile the bot and/or client:

```
./build-bot
./build-client
```

7. Tell your machine where to find the library (as they aren't in the standard directories is usually searches for libraries); this is temporary:

```
export LD_LIBRARY_PATH=$(pwd)/inc:$LD_LIBRARY_PATH`
```

8. Run the bot and or client:

```
./bin/bot
./bin/client
```
