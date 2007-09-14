
BEGIN { if(!host) host = "localhost:3080" 
        server = "http://" host 
        if(test ~ /^-/) test = ""
        echo = (mode == "compare") ? "echo -n" : "echo";
      }

/^#/  { next; }
/^$/  { next; }

{ if( (!test) || ($1 ~ test)) {
     if(mode == "sql") {
       if($2 == "SQL") printf("mysql --defaults-file=my.cnf < SQL/%s \n\n",$3)
       next
     }

     if(mode == "list") { print ; next }
     if($2 == "SQL") next;

     if(mode == "config") {
        qm = index($3, "?") ; 
        if (qm) base = substr($3, 0, qm - 1) ">"
        else base = $3 ">"
        printf("awk -f config.awk -v 'cfpat=/ndb/test/%s' httpd.conf\n", base)
        next
     }
     
     if(mode == "compare")      outfile = "> current/" $1
     else if(mode == "record")  outfile = "| tee results/" $1
     else outfile = " | tee current/" $1
          
     args=""
     if(NF > 3) for(i=4; i <= NF && $i !~ /^#/; i++) args = args $i " ";

     cmd = sprintf("curl -isS %s '%s/ndb/test/%s'", args, server, $3)
     filter = "sed -f " $2 ".sed"

     printf("%s '==== %s '\n", echo, $1)
     printf("%s | %s %s \n", cmd, filter, outfile)
     if(mode == "compare") 
       printf("cmp -s results/%s current/%s && echo OK || echo Fail\n",$1,$1)
     if(diff)
       printf("diff -C 10 results/%s current/%s \n",$1, $1)
   }
}
