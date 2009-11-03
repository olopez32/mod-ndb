

(
  grep -H COV_point *.cc | awk ' 
   
   {   len = length($1) 
       if(substr($1, len) == ":") filename = substr($1, 1, len - 1)
       else filename = $1 
       
       openquote = index($0, "\"")
       namelen = index(substr($0, openquote+1) , "\"") - 1 
       pointname = substr($0, openquote + 1, namelen) 
       
       printf(" { \"file\":\"%s\" , \"name\":\"%s\" }\n",
              filename, pointname)              
    
   }

'

  grep -H -n COV_line *.cc | awk -F: ' 
   {  printf(" { \"file\":\"%s\" , \"name\":\"Line:%d\" }\n", $1, $2) 
   }
'

) 

