# Project 2: Concurrent UNIX Processes and Shared Memory
This program reads in a list of strings and writes them to two separate output files based on whether or not the string is a palindrome. The output files are `palin.out` for palindromes, and `nopalin.out` for strings that aren't palindromes. 

The program reads the strings into **shared memory** and forks off multiple processes to concurrently process the strings using a version of the [Peterson's algorithm](https://en.wikipedia.org/wiki/Peterson%27s_algorithm).

## How to Build and Run
To build run `make`. Then run `./master < strings`.

**Important:** The program reads in a maximum of **256** strings of **256** length.

For more information read `cs4760Assignment2Fall2017Hauschild.pdf`.
