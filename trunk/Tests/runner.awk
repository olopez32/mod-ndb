#  (C) 2007 MySQL , 2009 Sun Microsystems
#  
#  This is the main engine of the test suite.  It takes a test.list file 
#  on input, and writes shell commands to output. 
#
#  Command line arguments, supplied through awk using -v, control the details.
#

BEGIN { if(!host) host = "localhost:3080" 
        server = "http://" host 
        if(test ~ /^-/) test = ""
        echo = (mode == "compare") ? "echo -n" : "echo";
       
        recorder = "awk -f record.awk -v obj="
      }

/^#/  { next; }
/^$/  { next; }

{ if( (!test) || ($1 ~ test)) {
     sub(/#.*/, "")  # strip comments

     flag_SQL = flag_JR = 0  
     filter = args = sorter = ""
     
     # Get the flags
     split($2, flags, "|")
     for(i in flags) {
       if(flags[i] == "SQL") flag_SQL = 1
       else if(flags[i] == "JR") flag_JR = 1
       else if(flags[i] ~ /^f/) filter = "sed -f " flags[i] ".sed"
       else if(flags[i] == "sort") sorter = "| sort -t: -k1n"
     }
  
     if(mode == "sql" && flag_SQL) {
        printf("mysql --defaults-file=my.cnf < SQL/%s \n\n",$3)
        next
     }
     else if(mode == "list") { 
        print 
        next 
     }
     else if(mode == "config") {
        qmark = index($3, "?")                 # e.g. "/item?q=1"   
        pathinf = match($3,/\/[0-9]+$/)        # e.g. "/item/1"

        if(qmark) base = substr($3, 1, qmark - 1)
        else if(pathinf) base = substr($3, 1, pathinf - 1)    
        else base = $3                       

        base = base ">"      # closing bracket of <Location ...> section         
        printf("awk -f config.awk -v obj=%s -v 'cfpat=/ndb/test/%s' httpd.conf\n", 
               $1, base)
        next
     }

     if(flag_SQL || mode == "sql") next;
     
     prefix = substr($1,1,3)
     archive_file = "results/" prefix ".archive"
     
     if(mode == "compare")      outfile = "> current/" $1
     else if(mode == "record")  outfile = "| " recorder $1 " | tee -a " archive_file
     else outfile = " | tee current/" $1
          
     if(NF > 3) for(i=4; i <= NF; i++) args = args $i " ";
     if(flag_JR) args = args " -H 'Content-Type: application/jsonrequest' "

     cmd = sprintf("curl -isS %s '%s/ndb/test/%s'", args, server, $3)

     printf("%s '==== %s '\n", echo, $1)
     printf("%s | %s %s %s \n", cmd, filter, sorter, outfile)

     if((diff || mode == "compare") && ! have_source[prefix]) {
       print "source " archive_file
       have_source[prefix] = 1
     }
     
     if(mode == "compare") 
       printf("r.%s | cmp -s - current/%s && echo OK || echo Fail\n", $1, $1)
     if(diff)
       printf("r.%s |diff -C 10 - current/%s \n",$1, $1)
   }
}
