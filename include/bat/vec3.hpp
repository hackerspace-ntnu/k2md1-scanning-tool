class vec3 {
public:
  float x, y, z;
  inline vec3(float, float, float);
  inline vec3();
  inline vec3 operator+(vec3);
  inline vec3 operator-(vec3);
  inline vec3 operator*(float);
  inline vec3 operator/(float);
  inline vec3 operator^(vec3);
  inline float operator*(vec3);
  inline void operator|=(float);
  inline void operator+=(vec3);
  inline void operator-=(vec3);
  inline void operator*=(float);
  inline void operator/=(float);
  inline unsigned char operator==(vec3 v) {
    return (v.x == x && v.y == y && v.z == z);
  }
  inline float&operator[](int);
  inline vec3 normalize(float);
  inline void rotate(vec3, vec3, vec3);
  inline void rotateR(vec3, vec3, vec3);
  inline void rotatePlane(vec3, float);
};

vec3::vec3(float vx, float vy, float vz) {
  x = vx;
  y = vy;
  z = vz;
}

vec3::vec3() {}

vec3 vec3::operator+(vec3 v) {
  return vec3(x+v.x, y+v.y, z+v.z);
}

vec3 vec3::operator-(vec3 v) {
  return vec3(x-v.x, y-v.y, z-v.z);
}

vec3 vec3::operator*(float v) {
  return vec3(x*v, y*v, z*v);
}

vec3 vec3::operator/(float v) {
  v = 1./v;
  return vec3(x*v, y*v, z*v);
}

float vec3::operator*(vec3 v) {
  return x*v.x+y*v.y+z*v.z;
}

vec3 vec3::operator^(vec3 v) {
  return vec3(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x);
}

void vec3::operator|=(float v) {
  float f = v/sqrt(x*x+y*y+z*z);
  x *= f;
  y *= f;
  z *= f;
}

void vec3::operator+=(vec3 v) {
  x += v.x;
  y += v.y;
  z += v.z;
}

void vec3::operator-=(vec3 v) {
  x -= v.x;
  y -= v.y;
  z -= v.z;
}

void vec3::operator*=(float v) {
  x *= v;
  y *= v;
  z *= v;
}

void vec3::operator/=(float v) {
  v = 1./v;
  x *= v;
  y *= v;
  z *= v;
}

void vec3::rotatePlane(vec3 u, float a) {
  u |= 1;
  float c = cos(M_PI*a/180.), s = sin(M_PI*a/180.);
  rotate(vec3(u.x*u.x+(1-u.x*u.x)*c, u.x*u.y*(1-c)-u.z*s, u.x*u.z*(1-c)+u.y*s), 
	 vec3(u.x*u.y*(1-c)+u.z*s, u.y*u.y+(1-u.y*u.y)*c, u.y*u.z*(1-c)-u.x*s), 
	 vec3(u.x*u.z*(1-c)-u.y*s, u.y*u.z*(1-c)+u.x*s, u.z*u.z+(1-u.z*u.z)*c));
}

void vec3::rotate(vec3 r0, vec3 r1, vec3 r2) {
  *this = vec3(r0**this, r1**this, r2**this);
}

void vec3::rotateR(vec3 r0, vec3 r1, vec3 r2) {
  *this = vec3(vec3(r0.x, r1.x, r2.x)**this, vec3(r0.y, r1.y, r2.y)**this, vec3(r0.z, r1.z, r2.z)**this);
}

float&vec3::operator[](int i) {
  if (i == 0) return x;
  if (i == 1) return y;
  if (i == 2) return z;
  return x;
}

vec3 vec3::normalize(float v = 1) {
  if (x == 0 && y == 0 && z == 0) return vec3(0, 0, 0);
  return *this/sqrt(x*x+y*y+z*z);
}

static float pers = 1;
static vec3*lights, camera(0, 0, 0), viewdir(0, 0, 1), viewing(0, 0, 0), up(0, 1, 0);
static int numlights = 0;

static void Trackball(float sx, float sy, float ex, float ey) {
  float f;
  if (sx*sx+sy*sy > 1) {
    f = sqrt(sx*sx+sy*sy)+.000001;
    sx /= f;
    sy /= f;
  }
  if (ex*ex+ey*ey > 1) {
    f = sqrt(ex*ex+ey*ey)+.000001;
    ex /= f;
    ey /= f;
  }
  if (sx == ex && sy == ey) return;
  vec3 sv(sx, sy, -sqrt(1-sx*sx-sy*sy)), ev(ex, ey, -sqrt(1-ex*ex-ey*ey)), u = ev^sv;
  vec3 r[3] = {
    vec3((up^viewdir).x, up.x, viewdir.x), 
    vec3((up^viewdir).y, up.y, viewdir.y), 
    vec3((up^viewdir).z, up.z, viewdir.z)};
  u = vec3(u*r[0], u*r[1], u*r[2]);
  u |= 1;
  float c = sv*ev, s = sqrt(1-c*c);
  //vec3 r[3] = {
  r[0] = vec3(u.x*u.x+(1-u.x*u.x)*c, u.x*u.y*(1-c)-u.z*s, u.x*u.z*(1-c)+u.y*s); 
  r[1] = vec3(u.x*u.y*(1-c)+u.z*s, u.y*u.y+(1-u.y*u.y)*c, u.y*u.z*(1-c)-u.x*s);
  r[2] = vec3(u.x*u.z*(1-c)-u.y*s, u.y*u.z*(1-c)+u.x*s, u.z*u.z+(1-u.z*u.z)*c);

  camera -= viewing;
  camera = vec3(r[0]*camera, r[1]*camera, r[2]*camera);
  camera += viewing;
  up = vec3(r[0]*up, r[1]*up, r[2]*up);
  viewdir = vec3(r[0]*viewdir, r[1]*viewdir, r[2]*viewdir);
}
