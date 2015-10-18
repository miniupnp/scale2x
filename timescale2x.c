#include <stdlib.h>
#include <stdio.h>
#include <sys/times.h>

#include <assert.h>

#include "scale2x.h"
#include "file.h"

/* number of iterations to run */
#define ITER_TIMES 10000

#define SCDST(i) (dst+(i)*dst_slice)
#define SCSRC(i) (src+(i)*src_slice)

void time_scale2x(void* void_dst, unsigned dst_slice, const void* void_src, unsigned src_slice, unsigned width, unsigned height)
{
	struct tms t[4];
	clock_t et[3];
	unsigned char* dst = (unsigned char*)void_dst;
	const unsigned char* src = (const unsigned char*)void_src;
	unsigned count;
	int i, j;

	assert(height >= 2);

	memset(t, 0, sizeof(t));
	memset(et, 0, sizeof(et));

	j = 0;
	times(&t[j++]);

	for (i = 0; i < ITER_TIMES; i++) {
		dst = (unsigned char*)void_dst;
		src = (const unsigned char*)void_src;
		scale2x_8_def(SCDST(0), SCDST(1), SCSRC(0), SCSRC(0), SCSRC(1), width);
		dst = SCDST(2);
		for (count = height - 2; count > 0; count--) {
			scale2x_8_def(SCDST(0), SCDST(1), SCSRC(0), SCSRC(1), SCSRC(2), width);
			dst = SCDST(2);
			src = SCSRC(1);
		}
		scale2x_8_def(SCDST(0), SCDST(1), SCSRC(0), SCSRC(1), SCSRC(1), width);
	}

	times(&t[j++]);

#if defined(__x86__) || defined(__x86_64__)
	for (i = 0; i < ITER_TIMES; i++) {
		dst = (unsigned char*)void_dst;
		src = (const unsigned char*)void_src;
		scale2x_8_sse2(SCDST(0), SCDST(1), SCSRC(0), SCSRC(0), SCSRC(1), width);
		dst = SCDST(2);
		for (count = height - 2; count > 0; count--) {
			scale2x_8_sse2(SCDST(0), SCDST(1), SCSRC(0), SCSRC(1), SCSRC(2), width);
			dst = SCDST(2);
			src = SCSRC(1);
		}
		scale2x_8_sse2(SCDST(0), SCDST(1), SCSRC(0), SCSRC(1), SCSRC(1), width);
	}

	times(&t[j++]);

	for (i = 0; i < ITER_TIMES; i++) {
		dst = (unsigned char*)void_dst;
		src = (const unsigned char*)void_src;
		scale2x_8_mmx(SCDST(0), SCDST(1), SCSRC(0), SCSRC(0), SCSRC(1), width);
		dst = SCDST(2);
		for (count = height - 2; count > 0; count--) {
			scale2x_8_mmx(SCDST(0), SCDST(1), SCSRC(0), SCSRC(1), SCSRC(2), width);
			dst = SCDST(2);
			src = SCSRC(1);
		}
		scale2x_8_mmx(SCDST(0), SCDST(1), SCSRC(0), SCSRC(1), SCSRC(1), width);
	}
#elif defined(__ALTIVEC__)
	for (i = 0; i < ITER_TIMES; i++) {
		dst = (unsigned char*)void_dst;
		src = (const unsigned char*)void_src;
		scale2x_8_altivec(SCDST(0), SCDST(1), SCSRC(0), SCSRC(0), SCSRC(1), width);
		dst = SCDST(2);
		for (count = height - 2; count > 0; count--) {
			scale2x_8_altivec(SCDST(0), SCDST(1), SCSRC(0), SCSRC(1), SCSRC(2), width);
			dst = SCDST(2);
			src = SCSRC(1);
		}
		scale2x_8_altivec(SCDST(0), SCDST(1), SCSRC(0), SCSRC(1), SCSRC(1), width);
	}
#endif

	times(&t[j++]);

	for (i = 0; i < j; i++) {
		et[i] = t[i+1].tms_utime - t[i].tms_utime;
	}

	printf("def implementation scale2x time =  %9lu\n", (unsigned long)et[0]);
#if defined(__x86__) || defined(__x86_64__)
	printf("sse2 implementation scale2x time = %9lu\n", (unsigned long)et[1]);
	printf("mmx implementation scale2x time =  %9lu\n", (unsigned long)et[2]);
#elif defined(__ALTIVEC__)
	printf("altivec implementation scale2x time = %9lu\n", (unsigned long)et[1]);
#endif
}

int file_process(const char* file)
{
	unsigned pixel;
	unsigned width;
	unsigned height;
	unsigned char* src_ptr;
	unsigned src_slice;
	unsigned char* dst_ptr;
	unsigned dst_slice;
	int type;
	int channel;
	png_color* palette;
	unsigned palette_size;

	if (file_read(file, &src_ptr, &src_slice, &pixel, &width, &height, &type, &channel, &palette, &palette_size, 0) != 0) {
		goto err;
	}
	printf("%s : %ux%u %ubpp type=%d channel=%d palette_size=%u\n", file, width, height, pixel*8, type, channel, palette_size);
	if (pixel != 1) {
		fprintf(stderr, "This is not a 8bpp file.\n");
		goto err_src;
	}

	dst_slice = width * pixel * 2;
	dst_ptr = malloc(dst_slice * height * 2);
	if (dst_ptr == NULL) {
		fprintf(stderr, "Memory allocation error\n");
		goto err_src;
	}

	time_scale2x(dst_ptr, dst_slice, src_ptr, src_slice, width, height);

	free(dst_ptr);
	free(src_ptr);
	free(palette);
	return 0;

err_src:
	free(src_ptr);
	free(palette);
err:
	return -1;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		/* usage */
		exit(EXIT_FAILURE);
	}
	if (file_process(argv[1]) != 0) {
		exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;
}

