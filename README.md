# termimg

ANSI escape codes to PNG images.

## What?

termimg uses [libvterm] and [cairo] to render text containing terminal
escape codes, which it reads from `stdin`, to a PNG image, which it
writes to stdout. It is very useful for converting typescript files
created with the `script` tool into images for easy embedding in web
pages and documents.

## Usage

A typical usage of termimg to generate a PNG image from a typescript file:

```shell
termimg <typescript >img.png
```

Colourful grep to an image:

```shell
grep --color=always regex file | termimg >img.png
```

In combination with feh, you can view the images immediately, without
writing to a file (not sure why you'd want to, but you can!):

```shell
ls --color=always | termimg | feh -
```

## Building

termimg depends on libvterm and cairo. On Debian, they can be installed
with `apt install libvterm-dev libcairo2-dev`. Building is then a simple
matter of running `make`. To install, symlink or copy `termimg` into a 
directory in your `PATH`.
