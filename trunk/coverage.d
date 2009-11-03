#!/usr/sbin/dtrace -F -s 

/* Copyright (C) 2006 - 2009 Sun Microsystems
 All rights reserved. Use is subject to license terms.
 
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
*/


/* Usage: coverage.d APACHE_PID  
   (and see the discussion below) 
*/


modndb$1:::coverage     { @cov[copyinstr(arg0), copyinstr(arg1)] = count(); }

END                     
   {
      printf("\n");
      printa("  { \"file\":\"%s\" , \"name\":\"%s\" , \"count\":%@u } \n ", 
             @cov);
   } 


/* How to use the code coverage framework:

  To create a named code coverage point in a source file add a line like:
    COV_point("my name");
  You can also create a coverage point that uses the line number as its name:
    COV_line(); 

  Code coverage testing is implemented using dtrace.
  In normal builds, COV_point() and COV_line() are defined as null and will do
  nothing.  ONLY if you run configure with "--dtrace" or "--debug", and run on
  Mac OS X, will code coverage testing be enabled.   While other dtrace 
  platforms such as Solaris could be supported, the current build system works 
  only with Apple's implementation of static user-defined tracing. 

  Coverage points are indexed by file name and point name.  COV_line() points
  will have the name "Line:xxx".  

  PRELIMINARY STEP
  The "lib.test.sh" file defines all of the t.* and cov.* functions
  needed to run the functional test suite and the code coverage test
   % cd Tests ; . lib.test.sh ; cd ..
  
  Step 1 is to create (or drop and recreate) the code_cov table in MySQL
  Cluster, where code coverage results will be stored.  Do this using the 
  command "t.sql covtab" from the test framework.
   % t.sql covtab 
   
  Step 2 is to run the test suite, collect code coverage results from dtrace,
  and load them into the database.  This requires some concurrent work in 
  four different shell windows.
   [ window 1: monitor the log file to get the Apache PID] 
   % tail -f logs/ndb_error_log
   
   [ window 2: start mod_ndb in a single process]
   % make stop
   % make single
   
   [ window 3: use the dtrace script to collect coverage statistics
     (get the PID from the log file) ] 
   % sudo -s
   # ./coverage.d APACHE_PID 
   dtrace: script './coverage.d' matched 8 probes

   [ window 4: run the test suite ]
   % t.test 
   Wait for all tests to finish
   
   [ back to window 3: ]
   Hit ctrl-c
   Copy the output from dtrace, and pipe it into cov.load 
   %  pbpaste | cov.load
   You should see a set of "200 OK" responses as the JSON records go into
   the database.

  Step 3 is to run a script that examines the code to find all defined trace 
  points and also load them into the table.  This helps reveal any 
  coverage points that were not hit in the test suite.
   % make start
   % sh Tests/all_coverage_points.sh | cov.load
  You will see some "409 Conflict" responses and perhaps some "200 OK" as well.
  
  FINAL STEP: Examine the results!
   % cov.all      # see everything
   % cov.missed   # see just the missed points



*/