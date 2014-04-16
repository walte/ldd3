//#include </work/apue/ourhdr.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
	int fdin, fdout;
	void *src, *dst;
	struct stat statbuf;
	unsigned char sz[1024]={0};
	//if ((fdin = open("/dev/simple_nopage", O_RDONLY)) < 0)
	if ((fdin = open("/dev/simpler", O_RDONLY)) < 0)
		perror("can't open /dev/simple_nopage for reading");

	if ((src = mmap(0, 4096*2, PROT_READ, MAP_SHARED,
					fdin, 0)) == MAP_FAILED)
		perror("mmap error for simplen");

	memcpy(sz, src, 11);
	sz[10]='\0';
	printf("%x\n", src);
	printf("%s\n\n", sz);

	printf("%d\n", getpid());
	sleep(300);
	memcpy(sz, src+4096, 11);
	printf("%x\n", src+4096);
	printf("%s\n", sz);

	exit(0);
}
