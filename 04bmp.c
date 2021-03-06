
#include <errno.h>
#include "04bmp.h"

/*获取bmp图片的rgb数据*/
char * load_bmp(const char *bmpfile, struct image_info *minfo)
{
	//打开bmp图片
	int fd = open(bmpfile, O_RDONLY);
	if(fd == -1)
	{
		fprintf(stderr, "opening \"%s\" failed: %s\n",
					bmpfile, strerror(errno));
		exit(0);
	}

	// 获得文件大小，并分配内存
	struct stat fileinfo;
	fstat(fd, &fileinfo);

	int   rgb_size = fileinfo.st_size;
	char *rgb_buf  = calloc(1, rgb_size);

	// 读取BMP内容到内存中
	struct bitmap_header header;
	struct bitmap_info info;
	struct rgb_quad quad;
	//读取文件头
	read(fd, &header, sizeof(header));
	//读取信息头
	read(fd, &info, sizeof(info));
	if(info.compression != 0)
	{
		read(fd, &quad, sizeof(quad));
		fprintf(stderr, "read quad! \n");
	}
	//读取rgb数据
	read(fd, rgb_buf, rgb_size);
	//将bmp图片的宽度、高度和图片的色彩深度信息保存下来
	minfo->width = info.width;
	minfo->height= info.height;
	minfo->pixel_size = info.bit_count/8;

	close(fd);
	return rgb_buf;
}

/*显示bmp图片*/
void display(char *bmpfile, //图片路径
				  char *FB, //映射内存的地址
			struct fb_var_screeninfo *vinfo,//lcd硬件参数
			int xoffset,    //x轴的偏移量（相对于坐标原点）
			int yoffset     //y轴的偏移量
			)
{
	//初始化保存图片信息的结构体
	struct image_info *minfo = calloc(1, sizeof(struct image_info));
	//获取图片的rgb数据
	char *rgb_buf = load_bmp(bmpfile, minfo);
	char *tmp = rgb_buf;

	
	//清除屏幕（全屏显示白色）
	int x, y, k = 0x00FFFFFFFF;
	
	for(x=0; x<vinfo->yres; x++)
	{
		for(y=0; y<vinfo->xres; y++)
		{

			memcpy(FB+(4*y)+((vinfo->xres)*4*x), &k, 4);
		}
	}
	
	// 从最后一行开始显示BMP图像
	int pad = ((4-( minfo->width * minfo->pixel_size ) % 4)) % 4; // 0-3
	rgb_buf += (minfo->width * minfo->pixel_size + pad) * (minfo->height-1);	
	
	//偏移映射区域（相对于坐标原点（0，0））
	FB += (yoffset * vinfo->xres + xoffset) * 4;
	//每一行显示的最大值
	int lcd_w = vinfo->xres - xoffset;
	//没列显示的最大值
	int lcd_h = vinfo->yres - yoffset;
	//开始显示
	for(x=0; x<lcd_h && x<minfo->height; x++)
	{
		for(y=0; y<lcd_w && y<minfo->width; y++)
		{
			unsigned long lcd_offset = (vinfo->xres*x + y) * 4;

			memcpy(FB + lcd_offset + vinfo->red.offset/8,   rgb_buf + 2, 1);
			memcpy(FB + lcd_offset + vinfo->green.offset/8, rgb_buf + 1, 1);
			memcpy(FB + lcd_offset + vinfo->blue.offset/8,  rgb_buf + 0, 1);
			
			rgb_buf += minfo->pixel_size;
		}

		rgb_buf += pad;
		rgb_buf -= (minfo->width * minfo->pixel_size + pad) * 2;
	}

	free(tmp);
}
