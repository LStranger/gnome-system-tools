#!/usr/bin/env perl
#-*- Mode: perl; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-

# This is a general script to manipulate the GLADE files. It keeps track of 
# the location of the read process using a stack. subst is called every
# time a tag is opened, passing a refference to the stack, the string
# from where the parsing stopped to the eol, and the whole string.

# The first argument must be $(pixmapsdir).

sub subst {
  my $stack = $_[0];
  my $input = $_[1];
  my $input2 = $_[2];
  my ($tmp, $pre, $max, $ret);
  
  $input2 =~ /\Q$input\E/;
  $pre = $`;
  $ret = $input2;
  $max = $#stack;
  
  if (($$stack[$max - 1] eq "project") &&
	 ($$stack[$max] eq "pixmaps_directory")) {
    $input =~ s/^[^<]*//;
    $ret = $pre . $PIXMAPSDIR . $input;
  } elsif (($$stack[$max - 1] eq "widget") &&
		 ($$stack[$max] eq "filename")) {
    $ret = $pre . "../pixmaps/" . $input;
  } elsif (($$stack[$max] eq "watermark_image") ||
		 ($$stack[$max] eq "logo_image")) {
    $ret = $pre . "../pixmaps/" . $input;
  }
  
  return $ret;
}

my @stack = ();
my $line_no = 0;
my $input;
my $input2;
my $tag;

$PIXMAPSDIR = $ARGV[0];

while ($input = <STDIN>) {
  
  $input2 = $input;
  $line_no ++;
  
  while ($input =~ s/^[^<]*<([^>]+)>//) {
    $tag = $1;
    
    next if ($tag =~ /\/$/); # stand-alone tags. Ignore.
    next if ($tag =~ /\?$/);
    
    if ($tag =~ /^([^\/][^ ]*).*/) { # opening tag.
	 push @stack, $1;
	 
	 # OK, change whatever we may want to change.
	 $input2 = &subst (\@stack, $input, $input2);
	 
    } elsif ($tag =~ /^\/([^ ]*).*/) { # closing tag.
	 if ($1 eq $stack[$#stack]) { # Check sane nesting.
	   pop @stack;
	 } else {	
	   print STDERR "Excess closing tag \"$1\" at line $line_no.\n";
	 }
    }
  }
  
  print $input2;
}

foreach $i (@stack) {
  print STDERR "Unclosed tag \"$i\" at end of file.\n";
}
