
# This takes a JSON formatted rows of data on input
# and generates curl statements on output 
#
# Usage:  awk -v url="http://localhost..." -f load_json_data.awk

 BEGIN {
     cmd = "curl -isS" 
     flags = "-H 'Content-Type: application/jsonrequest'" 
     if(! url) {
        print "run this script with ``awk -v url=http://...'' " 
        exit 1;
    }
  } 
           
  {  
     data = "--data-binary '" $0 "'"
     printf("%s %s %s %s \n", cmd, data, flags, url) 
  }
