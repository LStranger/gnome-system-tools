#!/usr/bin/env perl
#-*- Mode: perl; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-

# This is a general script to manipulate the GLADE files. It keeps track of 
# the location of the read process using a stack. subst is called every
# time a tag is opened, passing a refference to the stack, the string
# from where the parsing stopped to the eol, and the whole string.

# The first argument must be $(pixmapsdir).

$PIXMAPSDIR = $ARGV[0];

while (<STDIN>) {
  if (/\s*<property\s+name\s*=\s*\"pixbuf\"\s*>(.+)<\/property>/ ||
      /\s*<property\s+name\s*=\s*\"watermark_image\"\s*>(.+)<\/property>/ ||
      /\s*<property\s+name\s*=\s*\"logo\"\s*>(.+)<\/property>/ ||
      /\s*<property\s+name\s*=\s*\"icon\"\s*>(.+)<\/property>/ ||
      /\s*<property\s+name\s*=\s*\"logo_image\"\s*>(.+)<\/property>/) {

    $filename = $1;
    @parts = split (/\//, $filename);
    $basename = $parts[scalar @parts - 1];

    $new_line = $_;
    $new_line =~ s@$filename@$PIXMAPSDIR/$basename@;

    print $new_line;
  } else {
    print $_;
  }
}
