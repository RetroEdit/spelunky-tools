# spelunky-tools
## Level gen sim
These are some tools I created between March and May of 2022. The code is messy and poorly written in parts, so have fun!

They re-create the bare bones process of Spelunky 2 level generation. They effectively streamline the process of exploring seeds for specific level generation features; I used them to find the smallest and largest CO level layouts. Currently, they only simulate some basic features of CO levels, but it's not necessarily to simulate a whole level's generation to get the seed for the next level.

For the C++ code, I compiled it using GCC with `g++ -Wall -Wextra -O3 level_gen_sim.cpp -o level_gen_sim`; the Python code will probably work for Python 3 versions later than 3.9. The Python code demonstrates the basic functionality in a more readable form, but it's far less efficient than the C++ code.

The code was designed to match Spelunky 2 version 1.25.2, so it will need to be adjusted slightly to match level generation changes in newer versions.
