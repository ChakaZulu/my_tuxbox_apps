/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <asm/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include <linux/input.h>


int main (int argc, char **argv) {

    int fd = -1;        /* the file descriptor for the device */
    int yalv;           /* loop counter */
    size_t read_bytes;  /* how many bytes were read */
    struct input_event ev[64]; /* the events (up to 64 at once) */

    /* read() requires a file descriptor, so we check for one, and then open it */
    if (argc != 2) {
        fprintf(stderr, "usage: %s event-device - probably /dev/input/evdev0\n", argv[0]);
        exit(1);
    }
    if ((fd = open(argv[1], O_RDONLY)) < 0) {
        perror("evdev open");
        exit(1);
    }

    while (1)
        {
        read_bytes = read(fd, ev, sizeof(struct input_event) * 64);

        if (read_bytes < (int) sizeof(struct input_event)) {
            perror("evtest: short read");
            exit (1);
        }

        for (yalv = 0; yalv < (int) (read_bytes / sizeof(struct input_event)); yalv++)
            {
                printf("Event: time %ld.%06ld, type %d, code %d, value %d\n",
                       ev[yalv].time.tv_sec, ev[yalv].time.tv_usec, ev[yalv].type,
                       ev[yalv].code, ev[yalv].value);

            }
        }

    close(fd);

    exit(0);
}
