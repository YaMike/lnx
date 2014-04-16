/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  This code reads and prints mega88 firmware via SPI interface (mega is
 *									directly connected to SPI port).
 *
 *        Version:  1.0
 *        Created:  03/01/2013 06:00:46 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  "YaMike" <m.likholet@ya.ru>
 *        Company:  -
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#define BUFF_SIZE 240
char data[BUFF_SIZE];

int main(int argc, char *args[]) {
	int fd = open("/dev/mega", O_RDONLY), i;

	if (0 > fd) {
		fprintf(stderr,"Cannot open device file\n");
		return -1;
	}

	if (BUFF_SIZE != (i = read(fd, data, BUFF_SIZE))) {
		fprintf(stderr, "Read only %d bytes!\n", i);
	}

	printf("Read data:\n");
	for (i = 0; i < BUFF_SIZE/16; i++) {
		printf("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", 
						data[16*i],		data[16*i+1],		data[16*i+2],		data[16*i+3],
						data[16*i+4], data[16*i+5],		data[16*i+6],		data[16*i+7],
						data[16*i+8], data[16*i+9],		data[16*i+10],	data[16*i+11],
						data[16*i+12],data[16*i+13],	data[16*i+14],	data[16*i+15]);
	}
	printf("Finished.\n");

	close(fd);

	return 0;
}
