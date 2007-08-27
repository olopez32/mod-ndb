
BEGIN { if(!host) host = "localhost:3080" 
        server = "http://" host }

/^#/  { next; }
/^$/  { next; }

{ if( (!test) || ($1 ~ test)) {
     if(mode == "record") outfile = "> results/" $1
     else if(mode == "compare") outfile = "> current/" $1
     else outfile = ""
     filter = "sed -f " $2 ".sed"
     args=""
     if(NF > 3) for(i=4; i <= NF && $i !~ /^#/; i++) args = args $i " ";
     cmd = sprintf("curl -isS %s '%s/ndb/test/%s'", args, server, $3)
     printf("echo %s '==== %s '\n", mode=="compare" ? "-n" : "", $1)
     printf("%s | %s %s \n", cmd, filter, outfile)
     if(mode == "compare") 
       printf("cmp -s results/%s current/%s && echo Pass || echo Fail\n",$1,$1)
   }
}
