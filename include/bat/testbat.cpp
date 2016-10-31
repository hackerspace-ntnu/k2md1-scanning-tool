#include "bat.hpp"

int main() {
	int sw = 800, sh = 600;
	MyScreen screen(sw, sh);
	Surface sf(sw, sh);
	Clock clock;
	while (1) {
		while (screen.gotEvent()) {
			Event e = screen.getEvent();
			if (e.type == KeyPress and e.key == K_ESCAPE) return 0;
		}
		for (int i = 0; i < sf.w*sf.h; i++) 
			sf.pixels[i] = 0xff0000ff;
		int mx, my;
		screen.getCursorPos(mx, my);
		int w = 10;
		mx = max(min(mx, sw-w), 0);
		my = max(min(my, sh-w), 0);
		for (int j = my; j < my+10; j++)
			for (int i = mx; i < mx+10; i++) 
				sf.pixels[i+j*sf.w] = 0;

		screen.putSurface(sf);
		clock.tick(30);
	}
}
