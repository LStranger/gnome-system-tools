#!/usr/bin/env perl
#-*- Mode: perl; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-

# This is a general script to manipulate the GLADE files. It keeps track of 
# the location of the read process using a stack. subst is called every
# time a tag is opened, passing a refference to the stack, the string
# from where the parsing stopped to the eol, and the whole string.

# The first argument must be $(pixmapsdir).

$IN_AA_IMAGE_WIDGET_HACK = 0;
$NO_AA_IMAGE_WIDGET_HACK = 0;

sub image_widget_hack
{
  my ($tag_name, $input) = @_;

     if ($tag_name eq "child" || $tag_name eq "widget") { $IN_AA_IMAGE_WIDGET_HACK = 0; }
  elsif ($tag_name eq "class") { $input =~ s/GnomePixmap/Custom/; }
  elsif ($tag_name eq "name")
  {
    chomp $input;
    $input =~ s/^([ \t]*)(.*)/\1\2\n\1/;
    $input .= "<creation_function>xst_ui_create_image_widget</creation_function>\n";
  }
  elsif ($tag_name eq "filename")
  {
    chomp $input;
    $input =~ s/^([ \t]*)(.*)/\1\2\n\1/;
    $input =~ s/filename>/string1>/g;
    $input .= "<string2></string2><int1>0</int1><int2>0</int2>\n";
  }

  return $input;
}
    

sub subst
{
  my ($stack, $input, $input2) = @_;
  my ($tmp, $pre, $max, $ret);
  
  $input2 =~ /\Q$input\E/;
  $pre = $`;
  $ret = $input2;
  $max = $#stack;

  if (($$stack[$max] eq "name") &&
      ($input =~ /^_[0-9]+/))
  {
    $NO_AA_IMAGE_WIDGET_HACK = 1;
  }

  if ($NO_AA_IMAGE_WIDGET_HACK)
  {
    if (($$stack[$max] eq "class") &&
        ($input =~ /^GnomePixmap/))
    {
      $NO_AA_IMAGE_WIDGET_HACK = 0;
    }
  }
  elsif (($$stack[$max] eq "class") &&
         ($input =~ /^GnomePixmap/)) {
    $IN_AA_IMAGE_WIDGET_HACK = 1;
  }

  if (($$stack[$max - 1] eq "project") &&
	 ($$stack[$max] eq "pixmaps_directory"))
  {
    $input =~ s/^[^<]*//;
    $ret = $pre . $PIXMAPSDIR . $input;
  }
  elsif (($$stack[$max - 1] eq "widget") &&
         ($$stack[$max] eq "filename"))
  {
    $ret = "$pre$PIXMAPSDIR/$input";
  }
  elsif (($$stack[$max] eq "watermark_image") ||
         ($$stack[$max] eq "logo_image"))
  {
    $ret = $pre . "../pixmaps/" . $input;
  }
  
  return &image_widget_hack ($$stack[$max], $ret) if $IN_AA_IMAGE_WIDGET_HACK;
  
  return $ret;
}

my @stack = ();
my $line_no = 0;
my $input;
my $input2;
my $tag;

$PIXMAPSDIR = $ARGV[0];

while ($input = <STDIN>)
{
  
  $input2 = $input;
  $line_no ++;
  
  while ($input =~ s/^[^<]*<([^>]+)>//)
  {
    $tag = $1;
    
    next if ($tag =~ /\/$/); # stand-alone tags. Ignore.
    next if ($tag =~ /\?$/);
    
    if ($tag =~ /^([^\/][^ ]*).*/) # opening tag.
    { 
      push @stack, $1;
      
      # OK, change whatever we may want to change.
      $input2 = &subst (\@stack, $input, $input2);
      
    }
    elsif ($tag =~ /^\/([^ ]*).*/) # closing tag.
    {
      if ($1 eq $stack[$#stack]) # Check sane nesting.
      {
        pop @stack;
      }
      else
      {	
        print STDERR "Excess closing tag \"$1\" at line $line_no.\n";
      }
    }
  }
  
  print $input2;
}

foreach $i (@stack)
{
  print STDERR "Unclosed tag \"$i\" at end of file.\n";
}

