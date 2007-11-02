#
# Take test result on stdin 
# Reformat for archive 

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
