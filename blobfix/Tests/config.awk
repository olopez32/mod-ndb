#  (C) 2007 MySQL 
#  
#  This is an auxilliary script in the mod_ndb test suite.
#  It takes a pattern on the command line, with e.g. "-v cfpat=col101",
#  and reads an Apache httpd.conf file from input.
#
#  On output, it prints the <Location> section of the config file 
#  matching cfpat

BEGIN { printing = 0  }
        
/<Location/     {  if( $2 ~ cfpat) {
                        printf("\n##   %s  at line %d \n", obj, NR)
                        printing = 1 
                    }
                }
                { if(printing) print }
/<\/Location>/  { printing = 0 }
