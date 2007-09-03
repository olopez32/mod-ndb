
BEGIN { printing = 0  }
        
                { if(printing) print }
/<Location/     {  if( $2 ~ cfpat) {
                        printf("\n##      at line %d\n", NR)
                        print
                        printing = 1 
                    }
                }
/<\/Location>/  { printing = 0 }