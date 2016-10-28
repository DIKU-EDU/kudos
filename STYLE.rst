KUDOS Style
===========

Coding style is a social construct. The following documents the conventions
we've sought to employ throughout the years. We kindly ask you to adhere to
this style.

We also try to give the reasons for our choices. If you disagree, you *can*
take it up with your TA, but your time is better spent working on the
assignment.

Indentation
-----------
2 spaces

Using spaces ensures to retain the hierarchical presentation of the code, as
intended by the author, regardless of the reader's system settings.

Configuration
~~~~~~~~~~~~~

**vim**

Add the following line to your ``~/.vimrc``:

.. code:: vim
    au BufNewFile,BufRead /path/to/kudos/* set expandtab tabstop=2

If you like this as a global setting, you can simply write this instead:

.. code:: vim
    set expandtab tabstop=2

Naming
------
Variables and functions are all lower-cased with words separated by underscores.

Macros
------
The body of a macro must always be wrapped in parentheses. Macros are replaced
*verbatim*, so forgetting parentheses might have a very unintended effect.

Do not use macros to define custom control structures.

``#define`` your magic constants.

Braces
------
The opening brace is not placed on a line by itself.

The closing brace is always on a line by itself or the same line as the opening brace.

This is also known as K&R braces.

Braces are always required. Even around one-line blocks.

Spaces
------
Around keywords, apart from ``sizeof``, ``typeof``, ``alignof``, and ``__attribute__``

Around binary/ternary operators (not unary)

Avoid trailing whitespace.

Comments
--------
Use ``//`` to allow easy commenting out of code sections using ``/* */``.

Line length
-----------
Stick to under 80 characters.

You can't assume that the maintainers of your code will dedicate all of their
screen real-estate to your code.

Humans often have a hard time comprehending long horizontal lines of text.

Use line breaks and indentation to show structure.

Functions
---------
A function should do one thing well. Preferably in no more than 10 lines.
