/*mmaptests.c*/
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

// This program is a simple test of mmap. It maps a page of memory at address 0, writes a value to it, and reads it back.
// With root privilege, we are able to map zero address.

int main()
{
    long* p = (long*) mmap(0, sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    printf("mmap result %p (errno %s)\n",p,strerror(errno));
    if (errno != 0)
    {
        return 1;
    }
    *p = 0x123;
    printf("the thing at 0x0 is %lx\n",*p);
    return 0;
}
