#include <vector>
#include <cmath>
#include "bat/point.hpp"
#include <locale.h>

using std::max;

struct Node {
  int child[8], num;
  Node() {num = 0;}
};

struct Nearest {
  float r;
  int ind;
};
int c = 0;
struct Octree {
  vector<Node> node;
  Point*point;
  int points;
  Octree() {}
  Octree(Point*point_, int points_) {
    cout << "Loaded" << endl;
    point = point_;
    points = points_;
    scalePoints();
    cout << "Scaled" << endl;

    fillOctree();
    cout << "Filled" << endl;

    calcNormals();
    cout << "Done" << endl;
  }
  void scalePoints() {
    float minv[3], maxv[3];
    for (int i = 0; i < 3; i++) minv[i] = 1e9, maxv[i] = -1e9;
    for (int j = 0; j < points; j++) {
      for (int i = 0; i < 3; i++) {
	minv[i] = min(minv[i], point[j].v[i]);
	maxv[i] = max(maxv[i], point[j].v[i]);
      }
    }
    cout << "Naive bounding box:" << endl;
    for (int i = 0; i < 3; i++) {
      cout << minv[i] << " - " << maxv[i] << ", ";
    }
    cout << endl;
    float lmin[3], rmin[3], lmax[3], rmax[3];
    for (int i = 0; i < 3; i++) {
      lmin[i] = lmax[i] = minv[i];
      rmin[i] = rmax[i] = maxv[i];
    }
    for (int k = 0; k < 10; k++) {
      float mmin[3], mmax[3];
      int minfail[3], maxfail[3];
      for (int i = 0; i < 3; i++) {
	minfail[i] = maxfail[i] = 0;
	mmin[i] = (lmin[i]+rmin[i])*0.5;
	mmax[i] = (lmax[i]+rmax[i])*0.5;
      }
      for (int j = 0; j < points; j++) {
	for (int i = 0; i < 3; i++) {	
	  if (point[j].v[i] > mmax[i]) maxfail[i]++;
	  if (point[j].v[i] < mmin[i]) minfail[i]++;
	}
      }
      for (int i = 0; i < 3; i++) {
	(minfail[i] > points*0.001 ? rmin[i] : lmin[i]) = mmin[i];
	(maxfail[i] > points*0.001 ? lmax[i] : rmax[i]) = mmax[i];
      }
    }
    for (int i = 0; i < 3; i++) {
      minv[i] = (lmin[i]+rmin[i])*0.5;
      maxv[i] = (lmax[i]+rmax[i])*0.5;
    }
    cout << "Smart bounding box:" << endl;
    for (int i = 0; i < 3; i++) {
      cout << minv[i] << " - " << maxv[i] << ", ";
    }
    cout << endl;

    float maxw = 0, off[3];
    for (int i = 0; i < 3; i++) 
      maxw = max(maxw, (maxv[i]-minv[i]));
    float scale = 1./maxw;
    for (int i = 0; i < 3; i++) {
      off[i] = -(minv[i]+maxv[i])*0.5*scale+0.5;
    }

    for (int j = 0; j < points; j++) {
      for (int i = 0; i < 3; i++) {
	if (point[j].v[i] > maxv[i] || point[j].v[i] < minv[i]) point[j].col = 0x00ff00;
	point[j].v[i] = point[j].v[i]*scale+off[i];
      }
    }
    for (int j = 0; j < N_FRAMES; j++) {
      for (int i = 0; i < 3; i++) {
	pose[j*3+i] = pose[j*3+i]*scale+off[i];
      }
    }
  }
  void fillOctree() {
    node.clear();
    node.push_back(Node());
    const int maxdepth = 20;
    int stack[maxdepth], slen = 0;
    for (int i = 0; i < points; i++) {
      int ok = 1;
      for (int j = 0; j < 3; j++) {
	if (point[i].v[j] < 0 || point[i].v[j] >= 1) ok = 0;
      }
      if (!ok) continue;
      int ix = point[i].x*(1<<maxdepth);
      int iy = point[i].y*(1<<maxdepth);
      int iz = point[i].z*(1<<maxdepth);
      int shift = maxdepth-1, ind = 0;
      while (1) {
	int ci = (ix>>shift&1)+(iy>>shift&1)*2+(iz>>shift&1)*4;
	if (node[ind].num > -1 && node[ind].num < 8) {
	  node[ind].child[node[ind].num++] = i;
	  break;
	} else if (node[ind].num > -1) {
	  node[ind].num = -1;
	  int ni = node.size();
	  for (int k = 0; k < 8; k++) 
	    node.push_back(Node());
	  for (int k = 0; k < 8; k++) {
	    Point&p = point[node[ind].child[k]];
	    int mul = 1<<maxdepth-shift;
	    int ci = ((int(p.x*mul)&1)+
		      (int(p.y*mul)&1)*2+
		      (int(p.z*mul)&1)*4);
	    node[ni+ci].child[node[ni+ci].num++] = node[ind].child[k];
	    node[ind].child[k] = ni+k;
	  }
	}
	ind = node[ind].child[ci];
	shift--;
      }
    }
  }
  void calcNormals() {
    const int maxk = 64;
    loading_title("Estimating normals");
    recCalcNormals(maxk);
    loading_finish(points);
  }
  void recCalcNormals(const int&maxk, int ind = 0, float w = 1) {
    if (node[ind].num == -1) {
      w *= 0.5;
      for (int i = 0; i < 8; i++) {
	recCalcNormals(maxk, node[ind].child[i], w);
      }
    } else {
      for (int i = 0; i < node[ind].num; i++) {
	loading_bar(c++, points);

	Point&p = point[node[ind].child[i]];
	Nearest list[maxk];
	Nearest*plist = &list[0];
	int len = 0;
	float maxr = w*3;
	maxr *= maxr;
	getNeighbors(p.x, p.y, p.z, maxr, plist, len, maxk, p.x, p.y, p.z);
	/*if (rand()%100 == 0) {
	  int listed = 0;
	  for (int j = 0; j < points; j++) {
	    Point&q = point[j];
	    if (q.x < 0 || q.y < 0 || q.z < 0 || q.x >= 1 || q.y >= 1 || q.z >= 1) continue;
	    float dx = p.x-q.x;
	    float dy = p.y-q.y;
	    float dz = p.z-q.z;
	    float r2 = dx*dx+dy*dy+dz*dz;
	    if (r2 <= maxr) {
	      listed++;
	      int found = 0;
	      for (int k = 0; k < len; k++) {
		if (plist[k].ind == j) found = 1;
	      }
	      if (!found) {
		cout << j << ' ' << maxr << ' ' << r2 << ' ' << w*w*9 << ' ' << len << endl;
	      }
	    }
	  }
	  if (listed < len) {
	    cout << listed << " < " << len << endl;
	  }
	  }*/
	//cout << maxr << ' ' << w*w*9 <<  ' ' << len << endl;
	double sum[3] = {0,0,0};
	for (int j = 0; j < len; j++) {
	  Point&q = point[list[j].ind];
	  for (int k = 0; k < 3; k++)
	    sum[k] += q.v[k];
	  /*float dx = p.x-q.x, dy = p.y-q.y, dz = p.z-q.z;
	  if (dx*dx+dy*dy+dz*dz > maxr) {
	    cout << "error: " << dx << ' ' << dy << ' ' << dz << ' ' << dx*dx+dy*dy+dz*dz << ' ' << list[j].r << " > " <<  maxr << endl;
	    }*/
	}
	/*if (len == 0) {
	  cout << p.x << ' ' << p.y << ' ' << p.z << endl;
	  continue;
	  }*/
	double ilen = 1./len;
	for (int j = 0; j < 3; j++)
	  sum[j] *= ilen;
	double A=0, B=0, C=0, D=0, E=0, F=0; // symmetric 3x3 matrix
	for (int j = 0; j < len; j++) {
	  Point&q = point[list[j].ind];
	  double dx = q.x-sum[0], dy = q.y-sum[1], dz = q.z-sum[2];
	  A += dx*dx;
	  B += dx*dy;
	  C += dx*dz;
	  D += dy*dy;
	  E += dy*dz;
	  F += dz*dz;
	}
	double a = D*F-E*E;
	double b = C*E-B*F;
	double c = B*E-D*C;
	double d = A*F-C*C;
	double e = C*B-A*E;
	double f = A*D-B*B;

	double det = a*d*f+b*e*c*2-a*e*e-f*b*b-d*c*c;
	//cout << det << endl;
	if (det*det > 1e-40) {
	  double idet = 1./cbrt(det);
	  a *= idet, b *= idet, c *= idet, d *= idet, e *= idet, f *= idet;
	}
	  //cout << "before " << len << endl;
	  //cout << a << ' ' << b << ' ' << c << ' ' << d << ' ' << e << ' ' << f << endl;
	for (int j = 0; j < 0; j++) {
	  double A = a*a+b*b+c*c;
	  double B = a*b+b*d+c*e;
	  double C = a*c+b*e+c*f;
	  double D = b*b+d*d+e*e;
	  double E = b*c+d*e+e*f;
	  double F = c*c+e*e+f*f;

	  a = A*A+B*B+C*C;
	  b = A*B+B*D+C*E;
	  c = A*C+B*E+C*F;
	  d = B*B+D*D+E*E;
	  e = B*C+D*E+E*F;
	  f = C*C+E*E+F*F;
	}
	double nx, ny, nz;
	getPose(node[ind].child[i], nx, ny, nz);
	nx -= p.x;
	ny -= p.y;
	nz -= p.z;
	//double inorm = 1./sqrt(nx*nx+ny*ny+nz*nz);
	//nx *= inorm, ny *= inorm, nz *= inorm;
	//cout << endl;
	//cout << nx << ' ' << ny << ' ' << nz << endl;
	for (int j = 0; j < 4; j++) {
	  double NX = a*nx+b*ny+c*nz;
	  double NY = b*nx+d*ny+e*nz;
	  double NZ = c*nx+e*ny+f*nz;

	  nx = a*NX+b*NY+c*NZ;
	  ny = b*NX+d*NY+e*NZ;
	  nz = c*NX+e*NY+f*NZ;
	  
	  //cout << nx << ' ' << ny << ' ' << nz << endl;
	}
	double inorm = 1./sqrt(nx*nx+ny*ny+nz*nz);
	nx *= inorm, ny *= inorm, nz *= inorm;

	p.nx = nx;
	p.ny = ny;
	p.nz = nz;

	if (nx < 0) nx = 0;
	if (ny < 0) ny = 0;
	if (nz < 0) nz = 0;
	p.col = Color(nx*255, ny*255, nz*255);
	//cout << nx << ' ' << ny << ' ' << nz << endl;
	//cout << a*d*f+b*e*c*2-a*e*e-f*b*b-d*c*c << endl;
	//cout << a << ' ' << b << ' ' << c << ' ' << d << ' ' << e << ' ' << f << endl;
      }
    }
  }
  void getNeighbors(float&x, float&y, float&z, float&r, Nearest*&nearest, int&len, const int&maxk, float dx, float dy, float dz, float w = 1, int ind = 0) {
    w *= 0.5;
    float ax = max(fabs(dx-w)-w, 0.0f);
    float ay = max(fabs(dy-w)-w, 0.0f);
    float az = max(fabs(dz-w)-w, 0.0f);
    if (ax*ax+ay*ay+az*az > r) return;
    //cout << ind << endl;
    if (node[ind].num == -1) {
      for (int k = 0; k < 2; k++) {
	for (int j = 0; j < 2; j++) {
	  for (int i = 0; i < 2; i++) {
	    getNeighbors(x, y, z, r, nearest, len, maxk, dx-i*w, dy-j*w, dz-k*w, w, node[ind].child[i+j*2+k*4]);
	  }
	}
      }
    } else {
      for (int i = 0; i < node[ind].num; i++) {
	Point&p = point[node[ind].child[i]];
	dx = p.x-x;
	dy = p.y-y;
	dz = p.z-z;
	float r2 = dx*dx+dy*dy+dz*dz;
	//cout << r2 << ' ' << r << endl;
	if (r2 < r) {
	  if (len < maxk) {
	    nearest[len].r = r2;
	    nearest[len++].ind = node[ind].child[i];
	    if (len == maxk) {
	      r = 0;
	      for (int j = 0; j < maxk; j++)
		r = max(r, nearest[j].r);
	    }
	  } else {
	    float newr = 0;
	    for (int j = 0; j < maxk; j++) {
	      if (nearest[j].r == r) {
		r = 1e9;
		nearest[j].r = r2;
		nearest[j].ind = node[ind].child[i];
	      }
	      newr = max(newr, nearest[j].r);
	    }
	    r = newr;
	  }
	}
      }
    }
  }
  void save(char*file) {
    FILE*fp = fopen(file, "w");
    int count = 0;
    for (int i = 0; i < points; i++) {
      int ok = 1;
      for (int j = 0; j < 3; j++) {
	if (point[i].v[j] < 0 || point[i].v[j] >= 1) ok = 0;
      }
      count += ok;
    }
    setlocale(LC_ALL, "C");
    fprintf(fp, "ply\n"
	    "format ascii 1.0\n"
	    "element vertex %d\n"
	    "property float x\n"
	    "property float y\n"
	    "property float z\n"
	    "property float nx\n"
	    "property float ny\n"
	    "property float nz\n"
	    "end_header\n", count);
    for (int i = 0; i < points; i++) {
      int ok = 1;
      for (int j = 0; j < 3; j++) {
	if (point[i].v[j] < 0 || point[i].v[j] >= 1) ok = 0;
      }
      if (ok) {
	fprintf(fp, "%f %f %f %f %f %f\n", point[i].x, point[i].y, point[i].z, point[i].nx, point[i].ny, point[i].nz);
      }
    }
    fclose(fp);
  }
};
