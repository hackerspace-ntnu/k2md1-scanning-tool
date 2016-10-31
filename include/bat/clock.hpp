#include <sys/time.h>
#include <unistd.h>

class Clock {
public:
    timeval ta, tb;
    double realfps, maxfps;
    Clock() {
        gettimeofday(&ta, NULL);
    }
    double tick(double maxfps_) {
        double mintime = 1./maxfps_;

        gettimeofday(&tb, NULL);

        double deltime = (tb.tv_sec-ta.tv_sec+(tb.tv_usec-ta.tv_usec)*.000001);
        timeval cp = ta;
        if (deltime < mintime)
            usleep((mintime-deltime)*1000000);

        gettimeofday(&ta, NULL);

        maxfps = 1./deltime;
        realfps = 1./(ta.tv_sec-cp.tv_sec+(ta.tv_usec-cp.tv_usec)*.000001);
    }
    void print() {
        printf("fps: %.1f/%.1f           \r", realfps, maxfps);
        fflush(stdout);
    }
};
