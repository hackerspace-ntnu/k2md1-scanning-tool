typedef unsigned int uint;
typedef unsigned char byte;

struct Color {
  union {
    uint v;
    struct {
      byte b, g, r, a;
    };
  };
  Color() {}
  Color(byte r_, byte g_, byte b_) {
    v = b_|(g_<<8)|(r_<<16);
  }
  Color(uint a) {
    v = a;
  }
  operator uint&() {
    return v;
  }
  
};

class Surface {
public:
  int w, h;
  Color*pixels;
  void fill(Color v) {
    for (int i = 0; i < w*h; i++) {
      pixels[i] = v;
    }
  }
  Surface(unsigned int ww, unsigned int hh) {
    pixels = new Color[ww*hh];
    w = ww;
    h = hh;
  }
  Surface() {}
  inline Surface(const char*img);
};

static int stringLen(const char*s1) {
	int i = 0;
	while (s1[i]) i++;
	return i;
}

static int sameString(const char*s1, char*s2, int len) {
  for (int i = 0; i < len; i++) 
    if (s1[i] != s2[i]) return 0;
  return 1;
}
static void fskip(FILE *fp, int n) {
  while (n--) fgetc(fp);
}

Surface::Surface(const char*img) {
  int i;
  int len = stringLen(img);
  if (sameString(&img[len-3], (char*)"ppm", 3)) {
    FILE*fp = fopen(img, "rb");
    unsigned char s;
    int cd;
    if (!fp) {
			std::cout << "No such file\n";
      return;
    }
    if (fgetc(fp) != 'P') {
      fclose(fp);
			std::cout << "Error: '" << img << "' is not a pixmap file\n";
      return;
    }
    s = fgetc(fp);
    if (s != '5' && s != '6') {
      fclose(fp);
			std::cout << "Error: '" << img << "' is not a supported pixmap format\n";
      return;
    }
    i = fscanf(fp, "%d", &w);
    i = fscanf(fp, "%d", &h);
    i = fscanf(fp, "%d", &cd);
    if (cd != 255) {
			std::cout << "Unsupported color\n";
      return;
    }
    fgetc(fp);
    pixels = new Color[w*h];
    if (s == '6') {
      for (i = 0; i < w*h; i++) {
	pixels[i].r = (unsigned char)fgetc(fp);
	pixels[i].g = (unsigned char)fgetc(fp);
	pixels[i].b = (unsigned char)fgetc(fp);
      }
    } else {
      for (i = 0; i < w*h; i++) {
	pixels[i].r = (unsigned char)fgetc(fp);
      }
    }
  } else if (sameString(&img[len-3], (char*)"bmp", 3)) {
    unsigned short cols;
    FILE*fp = fopen(img, "rb");
    if (!fp) return;
    if (fgetc(fp) != 'B' or fgetc(fp) != 'M') {
      fclose(fp);
			std::cout << "Error: '" << img << "' is not a bitmap file\n";
      return;
    }
    fskip(fp, 16);
    w = fgetc(fp)+fgetc(fp)*256;
    fskip(fp, 2);
    h = fgetc(fp)+fgetc(fp)*256;
    fskip(fp, 22);
    cols = fgetc(fp)+fgetc(fp)*256;
    fskip(fp, 6);

    //printf("%d, %d\n", w, h);

    pixels = new Color[w*h];
    fskip(fp, cols*4);
    for (int y = h-1; y >= 0; y--) {
			for (int x = 0; x < w; x++) {
				pixels[y*w+x].a = 0;
				pixels[y*w+x].b = (unsigned char)fgetc(fp);
				pixels[y*w+x].g = (unsigned char)fgetc(fp);
				pixels[y*w+x].r = (unsigned char)fgetc(fp);
      }
      int x = w*3;
      while (x++%4) fgetc(fp);
    }
    fclose(fp);
  } else {
		std::cout << "Unsupported image format\n";
  }
}
