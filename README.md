# potato

A web service

## About

potato is written in C.  It's a fast web server.

## Ports

potato is being developed on Debian GNU/Linux 8 and 9 and Ubuntu 16.04.

## Building

From the github repository: run in a bash shell

```
./bootscrap && make && make install
```

TODO: Add a ./configure step.  Currently not needed.


### Configure Make Options

#### Is this a debug build or not

In the make files, config.make or package.make, define DEBUG or not.


### If this is a debug build

Defining in the make files, config.make or package.make, SPEW_LEVEL_DEBUG
SPEW_LEVEL_INFO SPEW_LEVEL_WARN or SPEW_LEVEL_ERROR will set the default
spew level which at startup time environment variable PO_SPEW_LEVEL can
override.  Like for example:

```
PO_SPEW_LEVEL=warn ./my_potato_program
```

## Development Notes


ERROR(), ASSERT(), VASSERT() calls are always present in the compiled code.

Setting make variable DEBUG does not change the potato library ABI
(application binary interface).


