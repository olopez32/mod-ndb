
/Fail$/  { print ; fail++ }
/OK$/    { pass++ } 

END {  if(fail) printf("\n")
       printf("Tests  Passed: %d \t Failed: %d \t Total: %d \n", 
              pass, fail, pass+fail)
    }
    