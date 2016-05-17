# libMonomeCpp

A C++ wrapper around libmonome (https://github.com/monome/libmonome), hiding some of the complexity of libmonome's C Interface. Offers a C++11, callback-based solution.

The std::function called TouchCallback is called whenever a button is pressed,
released, or a long press is detected (default time = 0.5s). In this function
is possible to change something in the monome itself, for example enabling the
corresponding LED by calling one of the setXXX methods.

The std::function GridRefreshed is called each time the grid is refreshed, that
is ~20 ms. It runs on the monome thread. One example of using this callback
is readin the MIDI clock of a sequencer, and updating the LEDs accordingly.
Note that there's an internal thread that changes all the LEDs every 20 ms,
so you are't supposed to do anything that is very time consuming here.

All the SetXXX methods are thread safe, so for example you can call them
from an audio or MIDI callback without problems: they use the lock-free 
TPCircularBuffer implementation. Please see the (few) examples.

Note that multiple calld to SetXXX with the same value (ON, OFF, BLINK) don't
afect the performance. So feel free to clear the grid how many times you want.

Always consider the monome as made of two different things:
- a MxN input matrix of push buttons (you decide what to do when they are pushed, through a callback)
- a MxN input matrix of LEDs (you decide when to light them up, calling SetXXX from any thread you want)

### Building
libmonomec++ requires Cmake to run

First you update the dependency on the awesome [TPCircularBuffer](https://github.com/michaeltyson/TPCircularBuffer)

```sh
$ git submodule update
```
then 
```sh
$ mkdir build && cd build
```
```sh
$ cmake ../
```

You can pass the agruments to build with your favorite IDE, for example -G Xcode

### License
libMonomeCpp is Public license

Uses libmonome (https://github.com/monome/libmonome):

Copyright (c) 2010 William Light <wrl@illest.net>
Copyright (c) 2013 Nedko Arnaudov <nedko@arnaudov.name>


And TPCircularBuffer
Copyright (C) 2012-2013 A Tasty Pixel

See the relevant README and Licenses

### Author

Alessandro Saccoia, 2016 (http://www.alsc.co)

