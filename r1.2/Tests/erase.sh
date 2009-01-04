
obj=$1
archive=`perl -e 'print substr("'$obj'",0,3) . ".archive"'` 

cat results/$archive | awk -v obj=$obj '
  BEGIN { printing = 1  }
        
  /^#/  {   if($3 == obj && $2 == "_BEGIN_") printing = 0 }
        {   if(printing) print   }
  /^#/  {   if($3 == obj && $2 == "__END__") printing = 1 }
' > current/$archive

mv current/$archive results/$archive
