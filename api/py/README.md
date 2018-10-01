# Python 3 extension example

## Build

    python3 setup.py build

Output: `build/lib.linux-x86_64-3.6/bslext.cpython-36m-x86_64-linux-gnu.so

## Run

    $ cd build/lib.linux-x86_64-3.6
    $ python3
    >>> import bslext
    >>> card = 0
	>>> port = 0
	>>> bslext.getLinkStats(card, port)

Tested on Ubuntu-16.04, Python *3.5.2*.
Tested on CentOS-7, Python *3.6.3*.
