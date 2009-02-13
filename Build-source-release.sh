#
# (C) 2009 Sun Microsystems
#

# Build a source release of mod_ndb 
#
# usage: Build-source-release BRANCH QUALITY REVISION

fail() {  echo "Failure: $1" && exit 
}

# Where is the svn tree?  REPO can be anything that "svn export" will accept:
# Either a local path to a working copy, or a URL to a repository:
CURDIR=`pwd`
REPO=$CURDIR

# Usage
if test -z "$1" || test -z "$2"  
 then
   echo "usage: Build-source-release RELEASE TAG [REVISION] "
   echo "  ex.: ./Build-source-release 1.1 beta 551"
   exit
fi

RELEASE="$1"
BRANCH="r$1"
SVNURL=$REPO/$BRANCH

TAG=$2
REVISION=$3

# If REVISION is not supplied, and svnversion can obtain it from the working copy,
# and it is a clean, unmodified version, then it will be used as the default.
if test -z "$REVISION"
 then
   REVISION=`svnversion $SVNURL`
   test $? != "0" && \
     fail "REVISION required; cannot obtain it using svnversion $SVNURL"
   echo "$REVISION" | grep -q '[:MSPe]'
   test $? = "0" && \
     fail "Cannot use unclean working copy $REVISION in $BRANCH"
   echo "Using svn revision $REVISION"
fi

# The release name is, e.g., "1.1-beta-r551"
# So the release directory is "mod_ndb-1.1-beta-r551" 
# And the release file is "mod_ndb-1.1-beta-r551.tar.gz"
RELNAME="$RELEASE-$TAG-r$REVISION"
RELDIR="mod_ndb-$RELNAME"

# Now move into the Releases directory
test -d Releases || mkdir Releases
cd Releases
echo "Creating Releases/$RELDIR"
svn export -q -r $REVISION $SVNURL $RELDIR  || fail "svn export -r $REVISION $SVNURL"

# Move on in to the release directory
cd "$RELDIR"

# Build the parsers
COCO=`which Coco`
test $? != "0" && fail "Cannot find Coco."
COCO="$COCO" make -f make_file_tail prep
echo
test $? != "0" && fail "Building parsers with Coco."

# In defaults.h, replace __SVN_DEV__ with the actual release name
sed -e "/^#define/s/__SVN_DEV__/$RELNAME/" < defaults.h > defaults.h.tmp
mv defaults.h.tmp defaults.h
test -f defaults.h || fail "something is wrong with defaults.h"
test -r defaults.h || fail "something is wrong with defaults.h"
test -s defaults.h || fail "something is wrong with defaults.h"
grep -q $RELNAME defaults.h || fail "something is wrong with defaults.h"

# Run the Architecture.graffle file through bzip2
bzip2 Architecture.graffle

# All done up in the release directory.
cd ..

# Create the tar file
tar cf $RELDIR.tar $RELDIR

# GZIP it
gzip $RELDIR.tar

# All done!
echo 
echo "All done!"
ls -l $RELDIR.tar.gz

# Back to where we started
cd $CURDIR

