#include <cmath>

using std::cout;
using std::endl;

struct Quaternion {
	float w, x, y, z;
	Quaternion() {}
	Quaternion(float nx, float ny, float nz) {
		float a = sqrt(nx*nx+ny*ny+nz*nz)*.5;
		w = cos(a);
		
		float sa = a>1e-7?sin(a)*.5/a:0;
		x = nx*sa;
		y = ny*sa;
		z = nz*sa;
	}
	Quaternion(float a, float b, float c, float d) : w(a), x(b), y(c), z(d) {}
	Quaternion operator~() {
		return Quaternion(w,-x,-y,-z);
	}
	Quaternion operator*(Quaternion q) {
		return Quaternion(w*q.w-x*q.x-y*q.y-z*q.z, 
											x*q.w+w*q.x-z*q.y+y*q.z, 
											y*q.w+z*q.x+w*q.y-x*q.z, 
											z*q.w-y*q.x+x*q.y+w*q.z);
	}
	void getRotmat(float*rot) {
		rot[0] = 1-2*y*y-2*z*z, rot[1] = 2*(x*y-z*w)  , rot[2] = 2*(x*z+y*w);
		rot[3] = 2*(x*y+z*w)  , rot[4] = 1-2*x*x-2*z*z, rot[5] = 2*(y*z-x*w);
		rot[6] = 2*(x*z-y*w)  , rot[7] = 2*(y*z+x*w)  , rot[8] = 1-2*x*x-2*y*y;
	}
};

struct View {
	float x, y, z;
	Quaternion q;
	View() {}
	View(float*dof) {
		x=dof[0]; y=dof[1]; z=dof[2]; q=Quaternion(dof[3], dof[4], dof[5]);
	}
	View(float x, float y, float z, float nx, float ny, float nz) : x(x), y(y), z(z) {
		q = Quaternion(nx, ny, nz);
	}
	View operator*(View b) {
		View r;
		Quaternion p(0, b.x, b.y, b.z);
		p = q*p*(~q);
		r.x = p.x+x, r.y = p.y+y, r.z = p.z+z;
		r.q = q*b.q;
		//cout << r.q.w << ' ' << r.q.x << ' ' << r.q.y << ' ' << r.q.z << endl;
		return r;
	}
	View operator/(View b) {
		View r;
		Quaternion p(0, -b.x, -b.y, -b.z);
		r.q = q*(~b.q);
		p = r.q*p*(~r.q);
		r.x = p.x+x, r.y = p.y+y, r.z = p.z+z;
		return r;
	}
	void getRotmat(float*rot) {
		q.getRotmat(rot);
	}
	void getTransform(float*rot) {
		q.getRotmat(rot);
		for (int j = 2; j; j--) 
			for (int i = 2; i >= 0; i--)
				rot[i+j*4] = rot[i+j*3];
		rot[3] = x;
		rot[7] = y;
		rot[11] = z;
	}
	void get6DOF(float*r) {
		r[0] = x, r[1] = y, r[2] = z;
		if (q.w > 1) q.w = 1;
		if (q.w <-1) q.w =-1;
		float l = q.x*q.x+q.y*q.y+q.z*q.z;
		float If = l > 1e-3 ? acos(q.w)*2.f/sqrt(l) : 2.f;
		r[3] = q.x*If, r[4] = q.y*If, r[5] = q.z*If;
		/*for (int k = 0; k < 6; k++) {
			if (r[k] != r[k]) cout << If << endl;
			}*/
	}
};

void MatMul(float**A, float*p, float*Ap, int n) {
	for (int i = 0; i < n; i++) {
		Ap[i] = 0;
		for (int j = 0; j < n; j++) 
			Ap[i] += p[j]*A[i][j];
	}
}

void MatMul(float*A, float*p, float*Ap, int n) {
	for (int i = 0; i < n; i++) {
		Ap[i] = 0;
		for (int j = 0; j < n; j++) 
			Ap[i] += p[j]*A[i+n*j];
	}
}

template<class T>
int solve(T&A, float*r, float*x, float*p, float*Ap, int n, float eps = 1e-3) {
	for (int i = 0; i < n; i++) p[i] = r[i];
	
	float rr = 0;
	for (int i = 0; i < n; i++) rr += r[i]*r[i];

	if (rr < eps) return 0;
	int c = 0;
	while (++c < 500) {
		//cout << c << ' ' << rr << endl;
		float pAp = 0;
		MatMul(A, p, Ap, n);
		for (int i = 0; i < n; i++)
			pAp += p[i]*Ap[i];

		float alpha = rr/pAp;
		if (alpha != alpha) {
			cout << "Error in linalg solving: " << rr << endl;
			return -1;
			exit(0);
		}
		for (int i = 0; i < n; i++) 
			x[i] += alpha*p[i];

		float rr2 = 0;
		for (int i = 0; i < n; i++) {
			r[i] -= alpha*Ap[i];
			rr2 += r[i]*r[i];
		}
	
		if (rr2 < eps) return c;

		float beta = rr2/rr;
		rr = rr2;

		for (int i = 0; i < n; i++) p[i] = r[i]+beta*p[i];
	}
	return c;
}

void matmul4(float*A, float*B, float*C) {
	for (int i = 0; i < 4; i++) 
		for (int j = 0; j < 4; j++) {
			C[i*4+j] = 0;
			for (int k = 0; k < 4; k++) 
				C[i*4+j] += A[i*4+k]*B[k*4+j];
		}
}

struct se3 {
	union {
		struct {
			float x, y, z, nx, ny, nz;
		};
		float l[6];
	};
	se3() {}
	se3(float*dof) {
		x=dof[0]; y=dof[1]; z=dof[2]; nx=dof[3]; ny=dof[4]; nz=dof[5];
	}
	se3(float x, float y, float z, float nx, float ny, float nz) {
		l[0]=x,l[1]=y,l[2]=z,l[3]=nx,l[4]=ny,l[5]=nz;
	}
	void exp(float*R, int full = 1) {
		float thetasq = nx*nx+ny*ny+nz*nz;
		float A, B, C;
		if (thetasq < 1e-5) {
			A = 1-thetasq*(1.f/6.f-thetasq/120.f);
			B = 1.f/2.f-thetasq*(1.f/24.f-thetasq/720.f);
			C = 1.f/6.f-thetasq*(1.f/120.f-thetasq/5040.f);
		} else {
			float theta = sqrt(thetasq);
			A = sin(theta)/theta;
			B = (1-cos(theta))/thetasq;
			C = (1-A)/thetasq;
		}
		R[0] = 1-B*(ny*ny+nz*nz), R[1] = B*nx*ny-A*nz     , R[2] = B*nx*nz+A*ny;
		R[4] = B*nx*ny+A*nz     , R[5] = 1-B*(nx*nx+nz*nz), R[6] = B*ny*nz-A*nx;
		R[8] = B*nx*nz-A*ny     , R[9] = B*ny*nz+A*nx     , R[10]= 1-B*(nx*nx+ny*ny);

		R[3] = x*( 1-C*(ny*ny+nz*nz) )+y*( C*nx*ny-B*nz )+z*( C*nx*nz+B*ny );
		R[7] = x*( C*nx*ny+B*nz )+y*( 1-C*(nx*nx+nz*nz) )+z*( C*ny*nz-B*nx );
		R[11]= x*( C*nx*nz-B*ny )+y*( C*ny*nz+B*nx )+z*( 1-C*(nx*nx+ny*ny) );

		if (full == 1)
			for (int i = 12; i < 16; i++) R[i]=i==15;
	}
	void log(float*R) {
		float cos_ = (R[0]+R[5]+R[10]-1.f)/2.f;
		if (cos_ > 1) cos_ = 1;
		if (cos_ <-1) cos_ =-1;
		float theta = acos(cos_);
		float thetasq = theta*theta;
		float A, B, C;
		if (thetasq < 1e-5) {
			A = 1-thetasq*(1.f/6.f-thetasq/120.f);
			C = (1.f/12.f-thetasq*(1.f/180.f-thetasq/840.f))/(1.f-thetasq*(1.f/12.f-thetasq/360.f));
		} else {
			A = sin(theta)/theta;
			B = (1-cos(theta))/thetasq;
			C = (1-A/(B*2.f))/thetasq;
		}
		//cout << A << ' ' << B << ' ' << C << endl;
		A = 0.5f/A;
		nx = A*(R[9]-R[6]);
		ny = A*(R[2]-R[8]);
		nz = A*(R[4]-R[1]);
		
		B = -0.5f;
		x = R[3]*( 1-C*(ny*ny+nz*nz) )+R[7]*( C*nx*ny-B*nz )+R[11]*( C*nx*nz+B*ny );
		y = R[3]*( C*nx*ny+B*nz )+R[7]*( 1-C*(nx*nx+nz*nz) )+R[11]*( C*ny*nz-B*nx );
		z = R[3]*( C*nx*nz-B*ny )+R[7]*( C*ny*nz+B*nx )+R[11]*( 1-C*(nx*nx+ny*ny) );
	}
	void invert() {
		x=-x,y=-y,z=-z,nx=-nx,ny=-ny,nz=-nz;
	}
	se3 operator-() {
		return se3(-x,-y,-z,-nx,-ny,-nz);
	}
	se3 operator*(se3 s) {
		float A[16], B[16], C[16];
		//for (int i = 12; i < 16; i++) A[i]=B[i]=i==15;
		exp(A),s.exp(B);
		matmul4(A, B, C);
		se3 ret;
		ret.log(C);
		return ret;
	}
	se3 operator*(float f) {
		return se3(x*f,y*f,z*f,nx*f,ny*f,nz*f);
	}
	float norm() {
		return x*x+y*y+z*z+nx*nx+ny*ny+nz*nz;
	}
};
