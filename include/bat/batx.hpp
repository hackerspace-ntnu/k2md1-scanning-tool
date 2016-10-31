#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>


struct Event {
	int type, key;
	Event() {}
	Event(int a, int b) {
		type = a;
		key = b;
	}
};

static int code2key(Display*d, int kc) {
  int tmp;
  KeySym *keysym = XGetKeyboardMapping(d,kc,1,&tmp);
  int r = keysym[0];
  XFree(keysym);
  return r;
}

static int warned = 0;
class MyScreen {
public:
  Display* display;
  Window   win;
  GC       gc;
  Visual*  vis;
  Colormap cmap;
  unsigned int w, h;
  XImage * pixmapImage;
  MyScreen() {}
  inline MyScreen(unsigned int, unsigned int);
  inline MyScreen(Display*, unsigned int, unsigned int);
  inline void update();
  inline void flush();
  inline void putSurface(Surface);
  inline void getCursorPos(int&x, int&y);
  inline void hideCursor();
  inline Event getEvent();
  inline int gotEvent();
  inline void setCursorPos(int, int);
  inline void init(Display*, unsigned int, unsigned int);
};

void MyScreen::hideCursor() {
  Cursor no_ptr;
  Pixmap bm_no;
  XColor black, dummy;
  Colormap colormap;
  static char no_data[] = { 0,0,0,0,0,0,0,0 };

  colormap = DefaultColormap(display, DefaultScreen(display));
  XAllocNamedColor(display, colormap, "black", &black, &dummy);
  bm_no = XCreateBitmapFromData(display, win, no_data, 8, 8);
  no_ptr = XCreatePixmapCursor(display, bm_no, bm_no, &black, &black, 0, 0);

  XDefineCursor(display, win, no_ptr);
  XFreeCursor(display, no_ptr);
}

void MyScreen::init(Display*disp, unsigned int sw, unsigned int sh) {
  w = sw;
  h = sh;
  display = disp;
  win = XCreateSimpleWindow(display, RootWindow(display, 0), 500, 100, w, h, 2, BlackPixel(display, 0), BlackPixel(display, 0));
  XMapWindow(display, win);
  cmap = DefaultColormap(display, 0);
  gc = XCreateGC(display, win, 0, 0);
  vis = DefaultVisual(display, 0);//DefaultDepth(display, 0)
  pixmapImage = XCreateImage(display, vis, 24, ZPixmap, 0, NULL, w, h, 8, 0);
  XSelectInput(display, win, ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | ExposureMask | StructureNotifyMask);
}

MyScreen::MyScreen(Display*disp, unsigned int sw, unsigned int sh) {
  init(disp, sw, sh);
}

MyScreen::MyScreen(unsigned int sw, unsigned int sh) {
  init(XOpenDisplay(NULL), sw, sh);
}

int MyScreen::gotEvent() {
  return XPending(display);
}

#undef KeyPress
#undef KeyRelease
#undef ButtonPress
#undef ButtonRelease

#define KeyPress 1
#define KeyRelease 2
#define ButtonPress 3
#define ButtonRelease 4

Event MyScreen::getEvent() {
	XEvent e;
	XNextEvent(display, &e);
	switch (e.type) {
	case 2:
		return Event(KeyPress, code2key(display, e.xkey.keycode));
	case 3:
		return Event(KeyRelease, code2key(display, e.xkey.keycode));
	case 4:
		return Event(ButtonPress, e.xbutton.button);
	case 5:
		return Event(ButtonRelease, e.xbutton.button);
	default:
		return Event(-1, -1);
	}
}

void MyScreen::flush() {
  XFlush(display);
}

void MyScreen::update() {
  XPutImage(display, win, gc, pixmapImage, 0, 0, 0, 0, w, h);
  XFlush(display);
}

void MyScreen::putSurface(Surface sf) {
  int bpp = pixmapImage->bits_per_pixel/8;
	if (bpp != 4) {
		if (!warned++) {
			printf("Warning: using inefficient screen blitting\n");
			pixmapImage->data = new char[h*pixmapImage->bytes_per_line];
		}
		for (int i = 0; i < sf.h*sf.w-1; i++) {
			*((uint*)&pixmapImage->data[i*bpp]) = sf.pixels[i]; //Unsafe stuff
		}
		pixmapImage->data[(sf.w*sf.h-1)*bpp] = sf.pixels[sf.w*sf.h-1].b;
		pixmapImage->data[(sf.w*sf.h-1)*bpp+1] = sf.pixels[sf.w*sf.h-1].g;
		pixmapImage->data[(sf.w*sf.h-1)*bpp+2] = sf.pixels[sf.w*sf.h-1].r;
	} else {
		pixmapImage->data = (char*)sf.pixels;
	}
	update();
}

void MyScreen::getCursorPos(int&x, int&y) {
  unsigned int r;
  int rx, ry;
  Window w;
  XQueryPointer(display, win, &w, &w, &rx, &ry, &x, &y, &r);
}

void MyScreen::setCursorPos(int x, int y) {
  XWarpPointer(display, None, win, 0, 0, 0, 0, x, y);
  XFlush(display);
}

//Shift, Caps lock, Ctrl, Alt, Num lock, (Scr lock), Menu, Alt Gr, Left MP, Middle MP, Right MP, (Scroll Up, Scroll Down)

#define Shift    1
#define Caps     2
#define CapsLock 2
#define Control  4
#define Ctrl     4
#define Alt      8
#define Num      0x10
#define NumLock  0x10
#define Scr      0x20
#define ScrLock  0x20
#define Menu     0x40
#define AltGr    0x80
#define LeftMouseButton   0x100
#define MiddleMouseButton 0x200
#define RightMouseButton  0x400
#define LeftButton   0x100
#define MiddleButton 0x200
#define RightButton  0x400

#define K_a XK_a
#define K_b XK_b
#define K_c XK_c
#define K_d XK_d
#define K_e XK_e
#define K_f XK_f
#define K_g XK_g
#define K_h XK_h
#define K_i XK_i
#define K_j XK_j
#define K_k XK_k
#define K_l XK_l
#define K_m XK_m
#define K_n XK_n
#define K_o XK_o
#define K_p XK_p
#define K_q XK_q
#define K_r XK_r
#define K_s XK_s
#define K_t XK_t
#define K_u XK_u
#define K_v XK_v
#define K_w XK_w
#define K_x XK_x
#define K_y XK_y
#define K_z XK_z

#define K_0 XK_0
#define K_1 XK_1
#define K_2 XK_2
#define K_3 XK_3
#define K_4 XK_4
#define K_5 XK_5
#define K_6 XK_6
#define K_7 XK_7
#define K_8 XK_8
#define K_9 XK_9

#define K_F1 XK_F1
#define K_F2 XK_F2
#define K_F3 XK_F3
#define K_F4 XK_F4
#define K_F5 XK_F5
#define K_F6 XK_F6
#define K_F7 XK_F7
#define K_F8 XK_F8
#define K_F9 XK_F9
#define K_F10 XK_F10
#define K_F11 XK_F11
#define K_F12 XK_F12

#define K_ESCAPE XK_Escape
#define K_TAB XK_Tab
#define K_LEFT XK_Left
#define K_RIGHT XK_Right
#define K_UP XK_Up
#define K_DOWN XK_Down
#define K_SPACE XK_space
#define K_RETURN XK_Return
#define K_ENTER XK_Return
#define K_BACKSPACE XK_BackSpace
#define K_CAPSLOCK XK_Caps_Lock
#define K_SHIFT XK_Shift_L

#define K_LSHIFT XK_Shift_L
#define K_RSHIFT XK_Shift_R

#define K_CONTROL XK_Control_L
#define K_CTRL XK_Control_L
#define K_PERIOD XK_period
#define K_COMMA XK_comma
#define K_LESS XK_less
#define K_PIPE XK_bar
#define K_BAR XK_bar
#define K_MINUS XK_minus
#define K_PLUS XK_plus
