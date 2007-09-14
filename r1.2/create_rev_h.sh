#!/bin/sh

if svnversion .
  then
    echo \#define REVISION \"`svnversion .`\" > revision.h
  else
    echo \#define REVISION \"0\" > revision.h
fi


