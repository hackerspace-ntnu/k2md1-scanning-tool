int*    pstart;
double* pose;

struct Point {
  Color col;
  float nx, ny, nz;
  union {
    struct {float x, y, z;};
    float v[3];
  };
  Point() {}
  Point(float x_, float y_, float z_, Color col) : x(x_),y(y_),z(z_),col(col) {}
};

void getPose(int ind, double&x, double&y, double&z) {
  int i = 0;
  while (ind >= pstart[i+1]) i++;
  x = pose[i*3];
  y = pose[i*3+1];
  z = pose[i*3+2];
}
