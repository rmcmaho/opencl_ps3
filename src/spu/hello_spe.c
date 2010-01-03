#include <stdio.h>
#include <unistd.h>


#ifndef __kernel
#define __kernel
#endif

__kernel int main(unsigned long long spe, unsigned long long argp,
	 unsigned long long envp)
  
{

  printf("Hello world!\n");
  printf("SPE ID: %llu\n", spe);
  printf("Argp: %llu\n", argp);
  printf("Envp: %llu\n", envp);
  //  sleep(15);
  
   
  return 0;
  
}
