
BEGIN { printing = 0  }
        
/<Location/     {  if( $2 ~ cfpat) {
                        printf("\n##      at line %d\n", NR)
                        printing = 1 
                    }
                }
                { if(printing) print }
/<\/Location>/  { printing = 0 }
