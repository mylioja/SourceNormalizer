# SourceNormalizer

Tool to normalize whitespaces and line endings in source files.

I like my sources to have:

- Only 7-bit ASCII characters
- Only Unix style `'\n'` line endings, never `'\r'` or `"\r\n"`
- No tabs `'\t'`
- Only good old spaces `' '` for spacing
- No trailing spaces at the ends of lines
- A line feed `'\n'` as the last character of the file

So, as a hobby project, I started writing a small tool
to detect and optionally fix these issues
in my source files. [Read more about the history here.](#some-history)

## Usage

Name of the tool is `source_normalizer`, and the anticipated normal
use-case is to scan all source files in a given directory and report
all the whitespace problems found. This is done as follows:

    source_normalizer path_to_examine

If the code is in several subdirectories, it's easy to scan them
all in one go using the `--recursive` or `-r` option.

    source_normalizer -r path_to_examine

The recursive scan will skip all sub directories that have a name
starting with a period, typically `.git`, `.vscode`, etc...
You can ask it to skip also other subdirectories with a `--skip` or `-s` option.
To skip more than one, the option can be repeated several times.
For example, the next command will skip also directories `bin` and `build`:

    source_normalizer -r -s bin -s build path_to_examine

If you'd like the tool to automatically fix the found issues,
add the `--fix` or `-f` option to your command line.

    source_normalizer -rf -s bin path_to_examine

There are plenty of other options.
With the `--help` option you can get an up to date overview.

Instead of a directory name, you can also specify file names.
This might be useful if you want to check or fix a file
that doesn't end with an extension that the tool considers
to indicate a source file.

The current list is `.c`, `.cc`, `.cpp`, `.h`, and `.hpp`.
Files with other extensions are ignored unless you
specifically mention the file on the command line.

## Options

*  `-f, --fix ` Fix detected easily fixable errors.
*  `-h, --help ` Display this help text and exit
*  `-r, --recursive ` Recurse to subdirectories
*  `-s, --skip=name ` Skip the given subdirectory when recursing
*  `-v, --verbose ` Display lots of messages
*  `-V, --version ` Display program version and exit

All whitespace problems are easily fixable, but only if the file has no strange or unprintable characters. Binary content and strange encodings, UTF-16 included, are considered unfixable at the moment.


## Some history

A properly configured text editor can automatically take care
of all the listed requirements, but I wanted to have a tool
to quickly and easily fix all these issues in a big bunch
of source files.

It turned out, that this was the easy part.

Along the way a lot more got added, and some of it is
still work in progress.

## Other Goals - Added along the way

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

### Release under `GPL-3.0-or-later`

As you may have noticed, I'm using a lot of code and getting major influences from the GNU project.
Therefore I'm releasing the program under GPL 3 (or later) license.

This is also a good exercise for me in preparation for releasing a larger project
with the GPL3 license.

### Reliably recognize UTF-16 encoded source text

In the past, I did quite a lot of programming in the Windows world
and I'm now re-using some of the
code in my Linux, Unix, and Posix projects.

Some probably well-intentioned tool from the Microsoft world has saved many of my source
files in UTF-16 encoding. Using a good editor it's easy to convert them to UTF-8 or even
plain old 7-bit ASCII, but I'd like my tool to handle it automatically.

I now have a quite good UTF-16 recognizer, the `Utf16Checker`,
but conversion to UTF-8 is still waiting to be written.
