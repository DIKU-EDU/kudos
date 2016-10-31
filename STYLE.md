# KUDOS Style

Coding style is a social construct. The following documents the conventions
we've sought to employ with KUDOS throughout the years. We kindly ask you to
adhere to this style.

We also try to give the reasons for our choices. If you disagree, you *can*
take it up with your TA, but your time is better spent working on the
assignment.

For more on (C) style guides, see Chapter 9 in [Modern
C](http://icube-icps.unistra.fr/index.php/File:ModernC.pdf).

Here are some examples of *other* style guides for C programmers (for your
musing only):

* [Linux kernel coding style](https://www.kernel.org/doc/Documentation/CodingStyle)
* [NASA C Style Guide](http://homepages.inf.ed.ac.uk/dts/pm/Papers/nasa-c-style.pdf)
* [GNU Coding Standards](https://www.gnu.org/prep/standards/standards.html)


## Indentation

2 spaces

Using spaces ensures to retain the hierarchical presentation of the code, as
intended by the author, regardless of the reader's system settings.


### Configuration

**vim**

Add the following line to your `~/.vimrc`:

```vim
au BufNewFile,BufRead /path/to/kudos/* set expandtab tabstop=2
```

If you like this as a global setting, you can simply write this instead:

```vim
set expandtab tabstop=2
```


## Naming

Variables and functions are all lower-cased with words separated by underscores.


## Macros

The body of a macro must always be wrapped in parentheses. Macros are replaced
*verbatim*, so forgetting parentheses might have a very unintended effect.

Do not use macros to define custom control structures.

`#define` your magic constants.


## Braces

The opening brace is not placed on a line by itself.

The closing brace is always on a line by itself or the same line as the opening brace.

This is also known as K&R braces.

Braces are always required. Even around one-line blocks.

## Spaces

Around keywords, apart from `sizeof`, `typeof`, `alignof`, and `__attribute__`

Around binary/ternary operators (not unary)

Avoid trailing whitespace.

## Comments

In general, you can use either multi-line (`/* */`) or single-line (`//`)
comments. We recommend single-line comments for everything. Some functions have
[Doxygen](http://www.stack.nl/~dimitri/doxygen/index.html)-style docstrings,
but this has not been used consistently.

For commenting out code, always use `//`. This is a great chance for you to get
acquainted with your text-editor.  Please also _consider removing commented out
code completely_, or add a comment as to why the code is commented out, if you
want someone else to sort it out.

**vim**

One option is to search and replace the beginnings of given lines with `// `.
For instance, to comment out lines 10-20:

```vim
:10,20s/^/\/\/ /
```

To comment the code back in:

```vim
:10,20s/^\/\/ //
```

Another option is to use [visual block
editing](https://mkrmr.wordpress.com/2010/05/14/vim-tip-visual-block-editing/).


## Line length

Stick to under 80 characters.

You can't assume that the maintainers of your code will dedicate all of their
screen real-estate to your code.

Humans often have a hard time comprehending long horizontal lines of text.

Use line breaks and indentation to show structure.

## Functions

A function should do one thing well. Preferably in no more than 10 lines.


## Include Guards

All header files are to be protected by include guards. Include guards ensure
that a header file is not included twice (the `#include` preprocessor directive
doesn't do this for you). Hence, it is important to use unique names for our
include guards.

A header file should follow this format:

```C
#ifndef <name>
#define <name>

// Code here

#endif // <name>
```

Where `<name>` , begins with `KUDOS_` for files in the `kudos` directory, and
`KUDOS_USERLAND_` for files in the `userland` directory. This is followed by
the path to the header file within the given directory, in ALL CAPS, with all
non-alphanumeric characters replaced by underscores. The `#endif` should be
followed by a comment with the include guard name for the sake of humans.

For instance, here is an include guard for
[`kudos/kernel/thread.h`](kudos/kernel/thread.h):

```C
#ifndef KUDOS_KERNEL_THREAD_H
#define KUDOS_KERNEL_THREAD_H

// Code here

#endif // KUDOS_KERNEL_THREAD_H
```

As another example, here is an include guard for
[`userland/lib.h`](userland/lib.h):

```C
#ifndef KUDOS_USERLAND_LIB_H
#define KUDOS_USERLAND_LIB_H
  
// Code here

#endif // KUDOS_USERLAND_LIB_H
```

You can use the [check_include_guards](tools/check_include_guards.py) tool to
check conformance of your include guards with this style guide.
