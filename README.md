# SourceNormalizer

Tool to normalize whitespaces and line endings in source files.

I like my sources to have:

- Only 7-bit ASCII characters
- Only Unix style `'\n'` line endings, never `'\r'` or `"\r\n"`
- No tabs `'\t'`
- Only good old spaces `' '` for spacing
- No trailing spaces at the ends of lines
- A line feed `'\n'` as the last character of the file

A properly configured text editor can automatically take care
of all these requirements, but I wanted to have a tool
to quickly and easily fix all these issues in a big bunch
of source files.

It turned out, that this was the easy part.

## Other Goals added along the way

It started as a very simple program, but then got the bright idea
to use this as an opportunity to experiment with some techniques I have lately wanted to try.

Here's the list:

- Use the `recursive_directory_iterator` to scan directories
- Handle command line parsing in a reusable way
- Follow GNU and Posix conventions
- Release under `GPL-3.0-or-later`
- Reliably recognize UTF-16 encoded source text

### Use the `recursive_directory_iterator`

I traditionally use a piece of my own code for iterating within directories.
In Posix compatible systems it uses `opedir`, `readdir`, and `closedir` to do the trick.
Under windows it emulates those functions with `FindFirstFile`, `FindNextFile`, and `FindClose`.

That piece of code has served me well all these years,
but now I wanted to try the new and exciting `recursive_directory_iterator` instead.

To get the proper `<filesystem>` headers I had to upgrade my g++ compiler to level 8,
and I had to add linker option `-lstdc++fs` to link the binary without errors.

I'm quite happy with the result, and almost ready to throw my
own implementation into the garbage heap of history.

Handling filesystem errors could probably use some more work,
but for my private use the messages in errors thrown by the library seem sufficient.


### Handle command line parsing in a reusable way

I usually structure my command line parsing around the GNU `getopt_long`.

It's described in here: https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Options.html

It's easy to use, well documented, and as might be expected,
follows the GNU command line syntax very closely.

Unfortunately it needs quite a lot of additional coding around it to properly handle even
the most essential standards and conventions of GNU command line interface,
such as the `--help` and `--version` options.

I wrote the `Options` class to encapsulate much of that extra coding into an easily
reusable package. It also helps me to keep the `main` function shorter and cleaner
than before, because the usually huge `switch(opt_char)` statement is nicely tucked away in another file.


### Follow GNU and Posix conventions

Some info about the requirements: https://www.gnu.org/prep/standards/html_node/Command_002dLine-Interfaces.html

My new `Options` command line handler is one step towards this goal.
It handles the `--help` and `--version` options, and makes it quite easy to
implement other options in the genuine GNU style.

This is also a good exercise for me in preparation for releasing a larger project
with the GPL3 license.

### Release under `GPL-3.0-or-later`

As you may have noticed, I'm using a lot of code and getting major influences from the GNU project.
Therefore I'm releasing the program under GPL 3 (or later) license.

### Reliably recognize UTF-16 encoded source text

In the past, I did quite a lot of programming in the Windows world
and I'm now re-using some of the
code in my Linux, Unix, and Posix projects.

Some probably well-intentioned tool from the Microsoft world has saved many of my source
files in UTF-16 encoding. Using a good editor it's easy to convert them to UTF-8 or even
plain old 7-bit ASCII, but I'd like my tool to handle it automatically.

I now have a quite good UTF-16 recognizer, the `Utf16Checker`,
but conversion to UTF-8 is still waiting to be written.
