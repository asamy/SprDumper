/*
 * Copyright (c) 2013 Ahmed Samy  <f.fallen45@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "buffer.h"
#include "asprintf.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <png.h>

#define min(a, b)				\
	__extension__ ({			\
		__typeof__(a) _a = (a);		\
		__typeof__(b) _b = (b);		\
		_a < _b ? _a : _b;		\
	})
#define max(a, b)				\
	__extension__ ({			\
		__typeof__(a) _a = (a);		\
		__typeof__(b) _b = (b);		\
		_a > _b ? _a : _b;		\
	})

typedef enum itemtype {
	Item,
	Creature
} itemtype_t;

typedef struct item {
	uint16_t id;
	uint8_t width, height;

	uint16_t *spriteIds;
	uint32_t spriteCount;
	itemtype_t iType;

	struct item *next;
} item_t;

typedef item_t *itemlist_t;
#define foreach_item(list, i)	\
	for ((i) = *(list); (i); (i) = (i)->next)

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
} pixel_t;

typedef struct {
	pixel_t *pixels;
	size_t width;
	size_t height;
} bitmap_t;

static pixel_t *pixel_at(bitmap_t *bitmap, int x, int y)
{
	return bitmap->pixels + bitmap->width * x + y;
}

static
#ifdef __GNUC__
	__attribute__((__noreturn__, __format__(__printf__, 1, 2)))
#endif
void critical(const char *errmsg, ...)
{
	va_list va;
	int len;
	char *outmsg;

	va_start(va, errmsg);
	len = vasprintf(&outmsg, errmsg, va);
	va_end(va);

	if (len < 0)
		abort ();

	puts(outmsg);
	free(outmsg);
	puts("Error is not recoverable, aborting now...");
	exit(EXIT_FAILURE);
}

static item_t *item_new(uint16_t id)
{
	item_t *n;
	if (!(n = malloc(sizeof(*n))))
		return NULL;

	n->id = id;
	n->next = NULL;
	n->spriteIds = NULL;
	return n;
}

static __inline void item_destroy(item_t *i)
{
	free(i->spriteIds);
	free(i);
}

static itemlist_t *itemlist_init(void)
{
	itemlist_t *ret;

	ret = calloc(1, sizeof(*ret));
	if (!ret)
		return NULL;

	return ret;
}

static void itemlist_destroy(itemlist_t *list)
{
	item_t *tmp, *p;

	tmp = *list;
	while (tmp) {
		p = tmp->next;
		item_destroy(tmp);
		tmp = p;
	}

	free(list);
}

static void itemlist_append(itemlist_t *list, item_t *newItem)
{
	newItem->next = *list;
	*list = newItem;
}

static item_t *item_unserialize(FILE *fp, uint16_t id)
{
	item_t *ret;
	uint8_t byte;

	if (!(ret = item_new(id))) {
		fclose(fp);
		critical("Can't allocate new item of size %d (id %d)", sizeof(*ret), id);
	}

	for (;;) {
		if ((byte = fgetc(fp)) == 0xFF)
			break;

		switch (byte) {
		case 0x1C:
		case 0x00:
		case 0x19:
		case 0x1D:
		case 0x20:
		case 0x08:
		case 0x09:
			fseek(fp, 2, SEEK_CUR);
			break;
		case 0x12:
		case 0x13:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x0A:
		case 0x0B:
		case 0x0C:
		case 0x0D:
		case 0x0E:
		case 0x0F:
		case 0x10:
		case 0x11:
		case 0x14:
		case 0x16:
		case 0x17:
		case 0x1A:
		case 0x1B:
		case 0x1E:
		case 0x1F:
			break;
		case 0x15:
		case 0x18:
			fseek(fp, 4, SEEK_CUR);
			break;
		default:
			critical("Failed to load item of id %d; Unknown byte 0x%x.\n", id, byte);
		}
	}

	ret->width  = fgetc(fp);
	ret->height = fgetc(fp);
	if (ret->width > 1 || ret->height > 1)
		fgetc(fp);

	ret->spriteCount = ret->width * ret->height * (int)fgetc(fp) * (int)fgetc(fp) * (int)fgetc(fp) * (int)fgetc(fp) * (int)fgetc(fp);
	if (!(ret->spriteIds = calloc(ret->spriteCount, sizeof(uint16_t))))
		critical("Cannot allocate sprite ids of count %d (item id %d)", ret->spriteCount, ret->id);

	int i;
	uint16_t spriteId;
	size_t n;
	for (i = 0; i < ret->spriteCount; ++i) {
		n = fread(&spriteId, 2, 1, fp);
		if (n != 1)
			continue;

		ret->spriteIds[i] = spriteId;
	}

	return ret;
}

static itemlist_t *load_dat(const char *f, int *maxCount)
{
	FILE *fp;
	item_t *n;
	itemlist_t *list;
	uint16_t id, creaturesCount, itemsCount;

	fp = fopen(f, "rb");
	if (!fp)
		return NULL;

	if (!(list = itemlist_init())) {
		fclose(fp);
		return NULL;
	}

	fseek(fp, 4, SEEK_CUR);  /* signature */
	fread(&itemsCount, 2, 1, fp);
	fread(&creaturesCount, 2, 1, fp);
	fseek(fp, 2, SEEK_CUR);   /* effects?  */
	fseek(fp, 2, SEEK_CUR);   /* missles?  */

	printf("Creatures: %d Items: %d Total: %d\n",
			creaturesCount, itemsCount, creaturesCount + itemsCount);
	for (id = 100; id < itemsCount + creaturesCount; ++id) {
		n = item_unserialize(fp, id);
		if (!n) {
			fprintf(stderr, "Failed to unserialize dat item %hd.\n", id);
			goto out;
		}

		if (id > itemsCount)
			n->iType = Creature;
		else
			n->iType = Item;
		itemlist_append(list, n);
		++(*maxCount);
	}

out:
	fclose(fp);
	return list;
}

static buffer_t *load_spr(const char *f, uint32_t *off, uint16_t *total)
{
	buffer_t *bp;

	bp = balloc(f);
	if (!bp)
		return NULL;

	bseek(bp, 4);
	*total  = bget16(bp);
	*off = btell(bp);
	return bp;
}

/** Based on OTClient's sprite loading.  Writes pixel into @bitmap  */
#define SPRITE_DATA_SIZE 32 * 32 * 4
static bool spr_write_pixels(uint16_t spriteId,
				buffer_t *sp, uint32_t offset, uint16_t total, bitmap_t *bitmap)
{
	if (spriteId == 0)
		return false;

	bseek(sp, ((spriteId - 1) * 4) + offset);
	uint32_t address  = bget32(sp);
	if (address == 0)
		return false;
	bseek(sp, address + 3);

	uint16_t pixelSize = bget16(sp);
	uint16_t x = 0, y = 0;
	int read = 0, writePos = 0, i;
	pixel_t *pixel;

	while (read < pixelSize && writePos < SPRITE_DATA_SIZE) {
		uint16_t transparentPixels = bget16(sp);
		uint16_t colorizedPixels   = bget16(sp);

		for (i = 0; i < transparentPixels && writePos < SPRITE_DATA_SIZE; ++i) {
			pixel = pixel_at(bitmap, x, y);
			pixel->red = pixel->green = pixel->blue = pixel->alpha = 0x00;

			if (x < 31)
				++x;
			else {
				x = 0;
				++y;
			}

			writePos += 4;
		}

		for (i = 0; i < colorizedPixels && writePos < SPRITE_DATA_SIZE; ++i) {
			pixel = pixel_at(bitmap, x, y);
			pixel->red   = bgetc(sp);
			pixel->green = bgetc(sp);
			pixel->blue  = bgetc(sp);
			pixel->alpha = 0xFF;

			if (x < 31)
				++x;
			else {
				x = 0;
				++y;
			}

			writePos += 4;
		}

		read += 4 + (3 * colorizedPixels);
	}

	while (writePos < SPRITE_DATA_SIZE) {
		pixel = pixel_at(bitmap, x, y);
		pixel->red = pixel->green = pixel->blue = pixel->alpha = 0x00;

		if (x < 31)
			++x;
		else {
			x = 0;
			++y;
		}

		writePos += 4;
	}

	return true;
}

static bool save_png(bitmap_t *bitmap, const char *path)
{
	FILE *fp;
	png_structp png = NULL;
	png_infop info = NULL;
	size_t x, y;
	png_byte **rows = NULL;

	int pixelSize = 4;	/* rgba  */
	int depth = 8;

	fp = fopen(path, "wb");
	if (!fp)
		return false;

	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png)
		return false;

	info = png_create_info_struct(png);
	if (!info) {
		png_destroy_write_struct(&png, &info);
		return false;
	}

	if (setjmp(png_jmpbuf(png))) {
		png_destroy_write_struct(&png, &info);
		return false;
	}

	png_set_IHDR(png,
		     info,
		     bitmap->width,
		     bitmap->height,
		     depth,
		     PNG_COLOR_TYPE_RGBA,
		     PNG_INTERLACE_NONE,
		     PNG_COMPRESSION_TYPE_DEFAULT,
		     PNG_FILTER_TYPE_DEFAULT
		    );

	rows = png_malloc(png, bitmap->height * sizeof(png_byte *));
	for (y = 0; y < bitmap->height; ++y) {
		png_byte *row =
			png_malloc(png, sizeof(uint8_t) * bitmap->width * pixelSize);
		rows[y] = row;
		for (x = 0; x < bitmap->width; ++x) {
			pixel_t *pixel = pixel_at(bitmap, x, y);
			*row++ = pixel->red;
			*row++ = pixel->green;
			*row++ = pixel->blue;
			*row++ = pixel->alpha;
		}
	}

	png_init_io(png, fp);
	png_set_rows(png, info, rows);
	png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);

	for (y = 0; y < bitmap->height; ++y)
		png_free(png, rows[y]);
	png_free(png, rows);

	png_destroy_write_struct(&png, &info);
	fclose(fp);
	return true;
}

static void makedir(const char *__name)
{
	/* Check if the directory already exists.  */
	struct stat st;
	if (stat(__name, &st) == 0)
		return;

	printf("Creating directory %s... ", __name);
	if (mkdir(__name, 0777) != 0)
		critical("Failed!");
	printf("Success\n");
}

int main(int ac, char *av[])
{
	char *dumpFolder = av[1];
	itemlist_t *list;
	item_t *it;
	int maxCount = 0;

	if (ac != 2)
		critical("Usage: %s <folder to dump into>", av[0]);

	if (!(list = load_dat("./Tibia.dat", &maxCount)))
		critical("Failed to load Tibia.dat!");

	uint32_t sp_off;
	uint16_t sp_total;
	buffer_t *sp = load_spr("./Tibia.spr", &sp_off, &sp_total);
	if (!sp)
		critical("Failed to load Tibia.spr!");

	printf("Total sprites found in Tibia.spr: %hd\n", sp_total);
	{
		/* Make the directories:
		 * - dumpFolder
		 * - dumpFolder/Items
		 * - dumpFolder/Creatures  */
		makedir(dumpFolder);

		char tmpf[512];
		snprintf(tmpf, sizeof(tmpf), "%s/Items", dumpFolder);
		makedir(tmpf);
		snprintf(tmpf, sizeof(tmpf), "%s/Creatures", dumpFolder);
		makedir(tmpf);
	}

#define MAX_FAILED_SIZE (1<<13)
	int count = 0, countFailed = 0, i, num;
	uint32_t failedIds[MAX_FAILED_SIZE];
	char *type;

	printf("Now dumping sprites into %s (This may take some time)...\n", dumpFolder);
	fflush(stdout);

	foreach_item(list, it) {
		/* Loop each sprite in this item, an item can have several
		 * sprites, for example if it's a creature it will have all
		 * of the possible creature directions. For an item
		 * it perhaps can contain the other parts if it's large (citation needed).  */
		if (it->iType == Item)
			type = "Items";
		else
			type = "Creatures";

		for (num = 0, i = 0; i < it->spriteCount; ++i) {
			bitmap_t bmp;

			bmp.width = bmp.height = 32;
			bmp.pixels = calloc(sizeof(pixel_t), 32 * 32);
			if (!spr_write_pixels(it->spriteIds[i], sp, sp_off, sp_total, &bmp)) {
				/* FIXME: Usually when the spr_write_pixels returns false, all of the
				 * item sprites are corrupt, which is extremely odd.
				 * This happens when loading Tibia files of 8.6, haven't tested
				 * with any other Tibia version.  */
				assert(countFailed < MAX_FAILED_SIZE);
				failedIds[countFailed++] = it->id;
				free(bmp.pixels);
				break;
			}

			char *file;
			/* ./dumpFolder/ItemType/ItemId_sNumSprite/  */
			if (!asprintf(&file, "%s/%s/%d_s%d.png", dumpFolder, type, it->id, num++))
				abort();
			if (!save_png(&bmp, file))
				abort();

			free(file);
			free(bmp.pixels);
		}

		++count;
		if (!(count % 2))
			printf("\r[%3d%%]", 100 * count / maxCount);
		fflush(stdout);
	}

	printf("\n%d sprites were saved", count);
	if (countFailed) {
		FILE *out;
		printf(" and %d were corrupt", countFailed);

		out = fopen("corrupt_ids.txt", "w+");
		if (out) {
			fprintf(out, "Corrupt item ids:\n");
			fprintf(out, "A\tB\tC\tD\tE\n");
			for (i = 0, num = 0; i < countFailed; ++i) {
				fprintf(out, "%d\t", failedIds[i]);
				if (++num == 5) {
					fprintf(out, "\n");
					num = 0;
				}
			}

			fclose(out);
			printf(", successfully saved corrupt ids to corrupt_ids.txt");
		} else
			printf(", failed to save corrupt ids to corrupt_ids.txt");
	}
	putchar('\n');

	itemlist_destroy(list);
	bfree(sp);
	return 0;
}

