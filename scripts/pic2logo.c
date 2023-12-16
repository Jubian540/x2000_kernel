#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP

#include "../include/stb_image/stb_image.h"
#include "../include/stb_image/stb_image_write.h"
/* #include <stb_image/stb_image_resize.h> */

struct _logo_info {
	int width;
	int height;
	int bpp;
	unsigned int p8;
	unsigned int background_color;
} __attribute__ ((packed));

static void gen_info(int w, int h, int bpp, int backcolor)
{
	printf("/*\n"
			" * Automatically generated by \"mklogo\"\n"
			" *\n"
			" * DO NOT EDIT\n"
			" *\n"
			" */\n\n"
			"#include <linux/init.h>\n"
			"#include <video/ingenic_logo.h>\n\n"
			"extern unsigned char logo_buf_initdata[];\n\n"
			"struct _logo_info logo_info = {\n"
			"\t%d,\n"
			"\t%d,\n"
			"\t%d,\n"
			"\tNULL,\n"
			"\t0x%08x,\n"
			"};\n\n"
			"unsigned char logo_buf_initdata[] __initdata = {\n",
			w, h, bpp, backcolor);
}

static int ReadPic(const char *pic_file, int real_bit_dep ,int backcolor, const char *output)
{
	int ret;
	int i, j, k = 0;
	int width, height, channels;
	unsigned char *data, *p;

	ret = stbi_info(pic_file, &width, &height, &channels);
	if (ret == 0) {
		return -1;
	}

	data = stbi_load(pic_file, &width, &height, &channels, 4);

	if(output) {
		unsigned int tmp = 0;
		unsigned int *xdata = data;
		struct _logo_info info;
		info.width = width;
		info.height = height;
		info.bpp = real_bit_dep;
		info.background_color = backcolor;
		info.p8 = 0;

		int fd = open(output, O_RDWR | O_CREAT | O_TRUNC, 0644);
		write(fd, &info, sizeof(info));

		for(i = 0; i < width * height * real_bit_dep / 32; i++) {
			tmp = ((xdata[i] & 0x000000ff) << 16) |
				((xdata[i] & 0x00ff0000) >> 16) |
				(xdata[i] & 0xff000000)|
				(xdata[i] & 0x0000ff00);
			xdata[i] = tmp;
		}

		write(fd, data, width * height * real_bit_dep / 8);
		close(fd);

		return 0;
	}

	// gen head file.
	gen_info(width, height, real_bit_dep, backcolor);

	if(real_bit_dep == 32) {
		for(i = 0; i < height; i++) {
			for(j = 0; j < ( 4 * width); j += 4 ) {
				p = data + i * (4 * width) + j;

				printf (" 0x%02X, 0x%02X, 0x%02X, 0x%02X,%s",
						*(p + 2), *(p + 1),
						*(p + 0), *(p + 3),
						((k++%8) == 7) ? "\n" : "");
			}
		}

	} else if(real_bit_dep == 16) {
		unsigned short s16;
		for(i = 0; i < height; i++) {
			for(j = 0; j < ( 4 * width); j += 4 ) {
				p = data + i * (4 * width) + j;
				s16 = ((*(p + 0) & 0xf8) << 8)
					| ((*(p + 1) & 0xfc) << 3)
					| ((*(p + 2) & 0xf8) >> 3);
				unsigned char *p8 = (unsigned char*)&s16;
				printf (" 0x%02X, 0x%02X,%s", p8[0], p8[1], ((k++%8) == 7) ? "\n" : "");
			}
		}
	}

	printf("};\n");

	return 0;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int backcolor = 0x00000000;
	unsigned char *output = NULL;

	if (argc >= 4)
		sscanf(argv[3], "%x", &backcolor);

	if (argc >= 5) {
		output = argv[4];
	}

	if (argc >= 3) {
		ret = ReadPic(argv[1], atoi(argv[2]), backcolor, output);
	} else {
		printf("usage: pic2logo file bpp<32|16> [backcolor] [output]\n");
		printf("       e.x.: pic2logo logo.png 32 0x000000ff > logo.c\n");
		printf("       e.x.: pic2logo logo.png 32 0x000000ff  logo.out\n");
		ret = -1;
	}

	return ret;
}
