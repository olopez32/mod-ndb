#!/bin/sh

if svn info
  then
    svn info | awk ' /Last Changed Rev/ { printf("#define REVISION %d\n",$NF) } ' > revision.h
  else
    echo \#define REVISION 0 > revision.h
fi


