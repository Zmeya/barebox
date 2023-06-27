/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <linux/kernel.h>

#include "../include/mach/sunxi/egon.h"

#include "compiler.h"
#include "common.h"
#include "common.c"

#define STAMP_VALUE 0x5f0a6c39

static void mkimage(char *infile, char *outfile)
{
       struct egon_header *hdr;
       uint32_t *p32;
       uint32_t sum;
       int i;
       size_t hdr_size = sizeof(*hdr);
       size_t bin_size;
       size_t img_size;
       void *bin;
       int fd, ret;

       bin = read_file(infile, &bin_size);
       if (!bin) {
               perror("read_file");
               exit(1);
       }

       /* test if the binary has reserved space for the header */
       hdr = bin;
       if (hdr->branch == EGON_HDR_BRANCH && memcmp(hdr->magic, "eGON", 4) == 0) {
               /* strip/skip existing header */
               bin += hdr_size;
               bin_size -= hdr_size;
       }

       hdr = calloc(1, hdr_size);
       if (!hdr) {
               perror("malloc");
               exit(1);
       }

       /* total image length must be a multiple of 4K bytes */
       img_size = ALIGN(hdr_size + bin_size, 4096);

       hdr->branch = EGON_HDR_BRANCH;
       hdr->length = cpu_to_le32(img_size);
       memcpy(hdr->magic, "eGON.BT0", 8);
       memcpy(hdr->spl_signature, "SPL", 3);
       hdr->spl_signature[3] = 0x03; /* version 0.3 */

       /* calculate header checksum: */
       sum = STAMP_VALUE;
       /*  - add the header checksum */
       for (p32 = (void *)hdr, i = 0; i < hdr_size / sizeof(uint32_t); i++)
               sum += le32_to_cpu(p32[i]);
       /*  - add the image checksum */
       for (p32 = bin, i = 0; i < bin_size / sizeof(uint32_t); i++)
               sum += le32_to_cpu(p32[i]);
       /*  - handle image size not aligned on 32-bits */
       if (bin_size % sizeof(uint32_t)) {
               uint32_t tmp = 0;
               size_t rem = bin_size % sizeof(uint32_t);
               memcpy(&tmp, bin + (bin_size - rem), rem);
               sum += le32_to_cpu(tmp);
       }
       /* final image will be padded with zeros: doesn't change the checksum */
       hdr->check_sum = cpu_to_le32(sum);

       fd = open(outfile, O_WRONLY | O_TRUNC | O_CREAT,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
       if (fd < 0) {
               fprintf(stderr, "Cannot open %s: %s\n", outfile, strerror(errno));
               exit(1);
       }
       /* write the header */
       ret = write_full(fd, hdr, hdr_size);
       if (ret < 0) {
               perror("write_full");
               exit(1);
       }
       /* write the binary */
       ret = write_full(fd, bin, bin_size);
       if (ret < 0) {
               perror("write_full");
               exit(1);
       }
       /* align the image file size on a 4K bytes multiple (img_size),
        * if neccessary ftruncate will pad the end of the file with zeros */
       ret = ftruncate(fd, img_size);
       if (ret < 0) {
               perror("ftruncate");
               exit(1);
       }
       close(fd);

       free(hdr);
}

static void usage(char *argv0)
{
       fprintf(stderr, "usage: %s <infile> <outfile>\n", argv0);
}

int main(int argc, char *argv[])
{
       if (argc != 3) {
               usage(argv[0]);
               return 1;
       }

       mkimage(argv[1], argv[2]);

       return 0;
}
