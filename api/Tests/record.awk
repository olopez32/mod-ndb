# Copyright (C) 2006 - 2009 Sun Microsystems
# All rights reserved. Use is subject to license terms.
  
#  This is an auxilliary script in the mod_ndb test suite.
#
#  It takes a test result on input, and reformats the result 
#  for storage in an archive file.

BEGIN {  
      if(! obj) {
         print "usage(): awk -f record.awk -v obj=x"
         exit 1
      }      
      print "# _BEGIN_ " obj 
      print "r." obj "() {"
      printf("  cat <<'__%s__'\n", obj) 
} 

      {   print  }

END  {
      printf("__%s__\n", obj) 
      print "}"
      print "# __END__ " obj 
      printf("\n") 
}
