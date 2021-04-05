# ElementaryCLI
An elementary Command Line Interface for C embedded projects

## Purpose

I was looking for a simple command line interface in order to configure an embedded device using serial communication (IP configuration, traces, and more). This [website](http://www.dalescott.net/an-embedded-command-line-interface/) lists some popular CLI but none of them was fitting my needs.

Here are a list of those "needs":

- Be able to execute commands
- Command should support arguments
- Command should be created at run time
- API should be stupidely simple
- The amount of space and processing should be compatible with embedded devices
- Must compile with C99 (Support for older compiler like armcc)

So here I am introducing ElementaryCLI !

## Build

The CLI can be built as a library or be included in a project as sources.
A CMake project allows you to test ElementaryCLI with an exemple :

``` 
mkdir build
cd build
cmake ../
cmake --build .
./cliExemple
```

## Philosophy

ElementaryCLI allows you to link a callback function to a defined command. When user types the command, the callback is called to execute an function. A command is a set of __tokens__, __arguments__ and __options__.

### Tokens

Tokens are organized as a tree. A parent can have several childrens. If a token has no children, it is concidered as a leaf. Here is an exemple:

```
.               // root
 \ show_config  // leaf
 \ set_config   // parent
    \ ip        // leaf
    \ gateway   // leaf
```

> There is always a `root` token initiated by the library. It should be the only token without parent.

### Arguments

A leaf can live without any argument (Ex: `show_config`). But some commands requier arguments (Ex: `set_config ip <address>` where `address` is an entry of the user)

There is 2 types of arguments:

    - Madatory (Noted `<arg>`)
    - Optional (Noted `[arg]`) (No yet supported)

Arguments are passed to callback as an array of `char *`. ElementaryCLI does not manage argument type (integer, boolean, strings, ...). It is up to the callback to parse the data.

### Options (No yet supported)

Options can modify a command behaviour. For the moment, only binary options will be available.
Options are optional by nature.

Exemple: Set verbosity with `-vvv`

### Callbacks 

Callbacks are defined by user. They are the interface between the commands and the user code function to call.
There prototype must match this one :

```C
int callback(int argc, char * argv[]);      // see cli_callback_t
```

The returned value tells if the command was successfull.

Callbacks have to check argument integrity and do the type conversion (Ex: String to int using `atoi()`)

## Usage

The recommanded way for creating the commands is the one below. We use brackets to highlight the actual tree we are creating.

```C
cli_token * tokLan;
cli_token * tokIpSet;
cli_token * curTok;

cli_init();
// 1st is the token text, 2nd is the description
tokLan = cli_add_token("lan", "LAN configuration");
{
    curTok = cli_add_token("show_config", "Show configuration");
    cli_set_callback(curTok, &show_config_callback);
    cli_add_children(tokLan, curTok);

    tokIpSet = cli_add_token("set_config", "Define new configuration");
    {
        curTok = cli_add_token("ip", "<address> Set IP adress");
        cli_set_callback(curTok, &set_ip_adress_callback);
        cli_set_argc(curTok, 1);
        cli_add_children(tokIpSet, curTok);
    }
    cli_add_children(tokLan, tokIpSet);
}
cli_add_children(cli_get_root_token(), tokLan);
```

This code gives the following tree:

```
.               
 \ show_config  // Call show_config_callback()
 \ set_config   
    \ ip        // Call set_ip_adress_callback(1, <address>)
```