DITo - Disk Image Tools
=======================

A c static library and a set of tools to handle disk image files of various file formats.

Installing
----------

	make
	make install

**With Homebrew**

	make
	PREFIX=/usr/local/Cellar/dito/0.1.0 make install
	brew link dito

Path format
-----------
The tools may require a path to a file or directory in an image file as
argument(s). The paths are given in the following format:

	filesystem:imagefile:partition:path

`filesystem` is the filesystem in use. This far, `ext2` is the only
implemented option.
`imagefile` is the path to your disk image file.
`partition` is the partition number you want. 1-4.
`path` is the path to the file from the root of the filesystem.

**Examples**

	ext2:imagefile.img:1:/boot/grub.conf

	ext2:codedrives.img:4:/src/main.c


The tools
---------
The following tools are implemented this far. Support for ext2 only.

**dito-generate**

	dito-generate imagefile size1 [size2] [size3] [size4]

Creates a hard drive image file with partition sizes as close to
size1..4 as possible. Sizes are in bytes but can be given with suffixes
e.g. 20M to create a partition with size 20 Mb. The first partition is
set as primary.

**dito-extract**

	dito-extract imagefile:partition output

Extracts one partition from a hard drive image file to a separate file.

**dito-format**

	dito-format imagefile:partition filesystem

Formats a partition of the image file with a filesystem. Valid file
system choices for now are:

	ext2


**dito-ls**

	dito-ls [-l] <path>

Show directory listing.

**dito-mkdir**

	dito-mkdir <path>

Creates an empty directory.

**dito-cp**

	dito-cp <source_path> <target_path>

Copies a file.
`<source_path>` and/or `<target_path>` can be replaced by paths to
local files to copy files into or out of image files.
Example:

	dito-cp directory/myfile.txt ext2:image.img:1:/path/to/dest.txt

	dito-cp ext2:image.img:1:/datefile.dat output_data.dat

Either could also be replaces by a dash `-` to read from or write to
stdin or stdout respectively.
Example:

	dito-cp ext2:image.img:1:/essay.txt - | wc -l

	ls -l | dito-cp - ext2:image.img:1:/directory_listing.txt

Important note
--------------
This software is provided as-is and I will take no responsibility for
any damage or data loss caused by them. I do not guarantee backwards
compatibility in future updates.

Why use these tool
-----------------
I created these tools for two reasons.

The first was that I wanted to learn more about filesystems in general
and ext2 in particular. 

The second was that I needed a tool for editing ext2 disk images for my
osdev project.

Why not loopback? I'm on a mac. No loopback devices. Doesn't support
ext2.

Why not native mounting? I'm on a mac. Doesn't support ext2. Also
horribly convoluted to do from a shell script. Also osx adds those
darned .DS_store files everywhere.

Why not osxfuse? Makes my computer freeze up irrevokably. Don't know
why.
