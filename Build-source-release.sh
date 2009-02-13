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
     fail "Cannot use unclean working copy -- $REVISION"
   echo "Using svn revision $REVISION"
fi


# Now move into the Releases directory
test -d Releases || mkdir Releases
cd Releases
RELDIR="mod_ndb-$RELEASE-$TAG-r$REVISION"
echo "Creating Releases/$RELDIR"
svn export -q -r $REVISION $SVNURL $RELDIR  || fail "svn export -r $REVISION $SVNURL"

# Build the parsers
cd "$RELDIR"
COCO=`which Coco`
test $? != "0" && fail "Cannot find Coco."
COCO="$COCO" make -f make_file_tail prep
echo
test $? != "0" && fail "Building parsers with Coco."



