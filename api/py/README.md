# Python 3 extension example

## Build

    python3 setup.py build

Output: `build/lib.macosx-10.11-x86_64-3.5/bslext.cpython-35m-darwin.so`

## Run

    $ cd build/lib.macosx-10.11-x86_64-3.5
    $ python3
    >>> import bslext
    >>> bslext.getNumberOfCards()

Tested on Ubuntu-16.04, Python *3.5.2*.
