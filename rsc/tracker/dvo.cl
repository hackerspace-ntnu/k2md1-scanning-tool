__constant sampler_t int_sampler = CLK_NORMALIZED_COORDS_FALSE |
	CLK_ADDRESS_CLAMP |
	CLK_FILTER_NEAREST;

__constant sampler_t bilinear = CLK_NORMALIZED_COORDS_FALSE |
	CLK_ADDRESS_CLAMP |
	CLK_FILTER_LINEAR;

__kernel void Wrap(__read_only image2d_t src_depth, 
									 __read_only image2d_t dest_depth, 
									 __constant float transform[12], 
									 __constant float projection[4], 
									 __global float A[36], 
									 __global float b[6]) {

	__local float local_A[36*8*8], local_b[6*8*8];

	int2 ij = (int2)(get_global_id(0), get_global_id(1));

	//Deproject 3d point from Z1 depth texture
	float pz1 = read_imagef(src_depth, int_sampler, ij).x;

	//if (pz1 > 1e7) return; //Undefined pixel

	float px1 = (ij.x-projection[0])*pz1*projection[3];
	float py1 = (ij.y-projection[1])*pz1*projection[3];

	//Transform to 3d point in I2 reference (with current estimate)
	float x = transform[0]*px1+transform[1]*py1+transform[2]*pz1+transform[3];
	float y = transform[4]*px1+transform[5]*py1+transform[6]*pz1+transform[7];
	float z = transform[8]*px1+transform[9]*py1+transform[10]*pz1+transform[11];

	//Project to I2 coordinate
	float iz = 1.f/z;
	float2 p2 = (float2)(x*projection[2]*iz+projection[0], 
											 y*projection[2]*iz+projection[1]);
	float4 dest = read_imagef(dest_depth, bilinear, p2);

	//if (dest.w < 0) return;

	//Calculate jacobian of I2
	float izz = iz*iz;
	const float dI2x = dest.x;
	const float dI2y = dest.y;

	float jacobian[6] = {dI2x*iz, 
												dI2y*iz, 
												-(dI2x*x+dI2y*y)*izz-1, 
												-dI2x*x*y*izz-dI2y*(1+y*y*izz)-y, 
												dI2x*(1+x*x*izz)+dI2y*x*y*izz+x, 
												(dI2y*x-dI2x*y)*iz};

	const float ivar = 0.01f;
	float residual = dest.z-z;
	float weight = 6./(5+residual*residual*ivar);
	
	int myid = get_local_id(0)+get_local_id(1)*8;
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j <= i; j++) {
			local_A[myid+(i*6+j)*64] += jacobian[i]*jacobian[j]*weight;
		}
		local_b[myid+i*64] -= weight*jacobian[i]*residual;
	}
	barrier(CLK_LOCAL_MEM_FENCE);
	//int groupid = get_group_id(0)+get_group_id(1)*get_num_groups(0);
	//if (get_local_id(0) == 0 && get_local_id(1) == 0) {
		for (int i = 0; i < 6; i++) {
			for (int j = 0; j <= i; j++) {
				A[i*6+j] += local_A[myid+(i*6+j)*64];
			}
			b[i] += local_b[myid*6+i];
		}
		//}
}

__kernel void UnrolledWrap(__read_only image2d_t src_depth, 
													 __read_only image2d_t dest_depth, 
													 __constant float transform[12], 
													 __constant float projection[4], 
													 __global float*Ab, 
													 __write_only image2d_t plot_img) {
#define STRIDE 29

	__local float scratch[STRIDE*8*8]; //Part sums of A and b

	int2 ij = (int2)(get_global_id(0), get_global_id(1));

	//Deproject 3d point from Z1 depth texture
	float pz1 = read_imagef(src_depth, int_sampler, ij).x+1.f;

	//if (pz1 > 1e7) return; //Undefined pixel

	float px1 = (ij.x-projection[0])*pz1*projection[3];
	float py1 = (ij.y-projection[1])*pz1*projection[3];

	//Transform to 3d point in I2 reference (with current estimate)
	float x = transform[0]*px1+transform[1]*py1+transform[2]*pz1+transform[3];
	float y = transform[4]*px1+transform[5]*py1+transform[6]*pz1+transform[7];
	float z = transform[8]*px1+transform[9]*py1+transform[10]*pz1+transform[11];

	//Project to I2 coordinate
	float iz = 1.f/z;
	float2 p2 = (float2)(x*projection[2]*iz+projection[0]+0.5f, 
											 y*projection[2]*iz+projection[1]+0.5f);
	float4 dest = read_imagef(dest_depth, bilinear, p2);

	//if (dest.w < 0) return;

	//Calculate jacobian of I2
	float izz = iz*iz;

	float dI2x = dest.x*projection[2]*.5f;
	float dI2y = dest.y*projection[2]*.5f;

	float jacobian[6] = {dI2x*iz, 
											 dI2y*iz, 
											 -(dI2x*x+dI2y*y)*izz-1, 
											 -dI2x*x*y*izz-dI2y*(1+y*y*izz)-y, 
											 dI2x*(1+x*x*izz)+dI2y*x*y*izz+x, 
											 (dI2y*x-dI2x*y)*iz};

	const float ivar = 5000.f;
	float residual = dest.z+1.f-z;
	float weight = 6.f/(5.f+residual*residual*ivar);

	int myid = get_local_id(0)+get_local_id(1)*8;

	//write_imagef(plot_img, ij, 0);
	//if (pz1 == 1.f || dest.w != 1.f) weight = 0;
	weight *= (pz1 != 0.f && dest.w == 1.f);
		//if (pz1 != 0.f && dest.w == 1.f) {
#if defined DEBUG
		const float f = 1.f;
	write_imagef(plot_img, ij, (float4)(z-1.f, 0, dest.z, 0.f)*f+(float4)(0.5f, 0.f, 0.5f, 0.f));
#endif
		//write_imagef(plot_img, ij, (float4)(jacobian[3], 0, jacobian[4], 0.f)*f+(float4)(0.5f, 0.f, 0.5f, 0.f));

	scratch[myid*STRIDE] = jacobian[0]*jacobian[0]*weight;
	scratch[myid*STRIDE+1] = jacobian[1]*jacobian[0]*weight;
	scratch[myid*STRIDE+2] = jacobian[1]*jacobian[1]*weight;
	scratch[myid*STRIDE+3] = jacobian[2]*jacobian[0]*weight;
	scratch[myid*STRIDE+4] = jacobian[2]*jacobian[1]*weight;
	scratch[myid*STRIDE+5] = jacobian[2]*jacobian[2]*weight;
	scratch[myid*STRIDE+6] = jacobian[3]*jacobian[0]*weight;
	scratch[myid*STRIDE+7] = jacobian[3]*jacobian[1]*weight;
	scratch[myid*STRIDE+8] = jacobian[3]*jacobian[2]*weight;
	scratch[myid*STRIDE+9] = jacobian[3]*jacobian[3]*weight;
	scratch[myid*STRIDE+10] = jacobian[4]*jacobian[0]*weight;
	scratch[myid*STRIDE+11] = jacobian[4]*jacobian[1]*weight;
	scratch[myid*STRIDE+12] = jacobian[4]*jacobian[2]*weight;
	scratch[myid*STRIDE+13] = jacobian[4]*jacobian[3]*weight;
	scratch[myid*STRIDE+14] = jacobian[4]*jacobian[4]*weight;
	scratch[myid*STRIDE+15] = jacobian[5]*jacobian[0]*weight;
	scratch[myid*STRIDE+16] = jacobian[5]*jacobian[1]*weight;
	scratch[myid*STRIDE+17] = jacobian[5]*jacobian[2]*weight;
	scratch[myid*STRIDE+18] = jacobian[5]*jacobian[3]*weight;
	scratch[myid*STRIDE+19] = jacobian[5]*jacobian[4]*weight;
	scratch[myid*STRIDE+20] = jacobian[5]*jacobian[5]*weight;

	scratch[myid*STRIDE+21] = jacobian[0]*residual*weight;
	scratch[myid*STRIDE+22] = jacobian[1]*residual*weight;
	scratch[myid*STRIDE+23] = jacobian[2]*residual*weight;
	scratch[myid*STRIDE+24] = jacobian[3]*residual*weight;
	scratch[myid*STRIDE+25] = jacobian[4]*residual*weight;
	scratch[myid*STRIDE+26] = jacobian[5]*residual*weight;
	scratch[myid*STRIDE+27] = (weight != 0);
	scratch[myid*STRIDE+28] = residual*residual*weight;

	/*} else {
#pragma unroll
		for (int i = 0; i < 29; i++) scratch[myid*STRIDE+i] = 0;
		}*/
	if ((myid&31) >= 29) return;

	myid += (myid>=32)*32*(STRIDE-1);
	barrier(CLK_LOCAL_MEM_FENCE);

#pragma unroll
	for (int i = 1; i < 32; i++) 
		scratch[myid] += scratch[myid+i*STRIDE];

	if (myid >= 32) return;
	barrier(CLK_LOCAL_MEM_FENCE);

	scratch[myid] += scratch[myid+32*STRIDE];
	
	int groupid = get_group_id(0)+get_group_id(1)*get_num_groups(0);
	Ab[myid+groupid*32] = scratch[myid];
}

__kernel void VectorWrap(__read_only image2d_t src_depth, 
												 __read_only image2d_t dest_depth, 
												 __constant float transform[12], 
												 __constant float projection[4], 
												 __global float*Ab, 
												 __write_only image2d_t plotimg, 
												 float ivar) {
#define STRIDE 29

	__local float scratch[STRIDE*8*8]; //Part sums of A and b

	int2 ij = (int2)(get_global_id(0), get_global_id(1));

	//Deproject 3d point from Z1 depth texture
	float4 pz1 = read_imagef(src_depth, int_sampler, ij)+1.f;

	//if (pz1 > 1e7) return; //Undefined pixel

	float4 px1 = ((float4)(ij.x*2, ij.x*2+1, ij.x*2, ij.x*2+1)-projection[0])*pz1*projection[3];
	float4 py1 = ((float4)(ij.y*2, ij.y*2, ij.y*2+1, ij.y*2+1)-projection[1])*pz1*projection[3];

	//Transform to 3d point in I2 reference (with current estimate)
	float4 x = transform[0]*px1+transform[1]*py1+transform[2]*pz1+transform[3];
	float4 y = transform[4]*px1+transform[5]*py1+transform[6]*pz1+transform[7];
	float4 z = transform[8]*px1+transform[9]*py1+transform[10]*pz1+transform[11];

	//Project to I2 coordinate
	float4 iz = 1.f/z;
	if (z.x == 0) iz.x = 0;
	if (z.y == 0) iz.y = 0;
	if (z.z == 0) iz.z = 0;
	if (z.w == 0) iz.w = 0;

	float8 p2 = (float8)(x*projection[2]*iz+projection[0]+0.5f, 
											 y*projection[2]*iz+projection[1]+0.5f);

	float16 dest = (float16)(read_imagef(dest_depth, bilinear, p2.s04),
													 read_imagef(dest_depth, bilinear, p2.s15),
													 read_imagef(dest_depth, bilinear, p2.s26),
													 read_imagef(dest_depth, bilinear, p2.s37));

	//if (dest.w < 0) return;

	//Calculate jacobian of I2
	float4 izz = iz*iz;

	float4 dI2x = dest.s048c*projection[2]*.5f;
	float4 dI2y = dest.s159d*projection[2]*.5f;

	float4 jacobian[6] = {dI2x*iz, 
												dI2y*iz, 
												-(dI2x*x+dI2y*y)*izz-1, 
												-dI2x*x*y*izz-dI2y*(1+y*y*izz)-y, 
												dI2x*(1+x*x*izz)+dI2y*x*y*izz+x, 
												(dI2y*x-dI2x*y)*iz};

	float4 residual = dest.s26ae+1.f-z;
	float4 weight = 6.0f/(5.0f+residual*residual*ivar);
	
	int myid = get_local_id(0)+get_local_id(1)*8;

	weight.x *= (pz1.x != 1.f && dest.s3 == 1.f);
	weight.y *= (pz1.y != 1.f && dest.s7 == 1.f);
	weight.z *= (pz1.z != 1.f && dest.sb == 1.f);
	weight.w *= (pz1.w != 1.f && dest.sf == 1.f);

#if defined DEBUG
	const float f = 1.f;
	float4 r = z-1.f, g = (float4)(pz1.x!=1.f,pz1.y!=1.f,pz1.z!=1.f,pz1.w!=1.f)-0.5f, b = dest.s26ae;
	g = -0.5f;
	//r = -0.5f;
	write_imagef(plotimg, (int2)(ij.x*2, ij.y*2), (float4)(b.x, g.x, r.x, 0.f)*f+(float4)(0.5f, 0.5f, 0.5f, 1.f));
	write_imagef(plotimg, (int2)(ij.x*2+1, ij.y*2), (float4)(b.y, g.y, r.y, 0.f)*f+(float4)(0.5f, 0.5f, 0.5f, 1.f));
	write_imagef(plotimg, (int2)(ij.x*2, ij.y*2+1), (float4)(b.z, g.z, r.z, 0.f)*f+(float4)(0.5f, 0.5f, 0.5f, 1.f));
	write_imagef(plotimg, (int2)(ij.x*2+1, ij.y*2+1), (float4)(b.w, g.w, r.w, 0.f)*f+(float4)(0.5f, 0.5f, 0.5f, 1.f));
#endif

	scratch[myid*STRIDE] = dot(jacobian[0], jacobian[0]*weight);
	scratch[myid*STRIDE+1] = dot(jacobian[1], jacobian[0]*weight);
	scratch[myid*STRIDE+2] = dot(jacobian[1], jacobian[1]*weight);
	scratch[myid*STRIDE+3] = dot(jacobian[2], jacobian[0]*weight);
	scratch[myid*STRIDE+4] = dot(jacobian[2], jacobian[1]*weight);
	scratch[myid*STRIDE+5] = dot(jacobian[2], jacobian[2]*weight);
	scratch[myid*STRIDE+6] = dot(jacobian[3], jacobian[0]*weight);
	scratch[myid*STRIDE+7] = dot(jacobian[3], jacobian[1]*weight);
	scratch[myid*STRIDE+8] = dot(jacobian[3], jacobian[2]*weight);
	scratch[myid*STRIDE+9] = dot(jacobian[3], jacobian[3]*weight);
	scratch[myid*STRIDE+10] = dot(jacobian[4], jacobian[0]*weight);
	scratch[myid*STRIDE+11] = dot(jacobian[4], jacobian[1]*weight);
	scratch[myid*STRIDE+12] = dot(jacobian[4], jacobian[2]*weight);
	scratch[myid*STRIDE+13] = dot(jacobian[4], jacobian[3]*weight);
	scratch[myid*STRIDE+14] = dot(jacobian[4], jacobian[4]*weight);
	scratch[myid*STRIDE+15] = dot(jacobian[5], jacobian[0]*weight);
	scratch[myid*STRIDE+16] = dot(jacobian[5], jacobian[1]*weight);
	scratch[myid*STRIDE+17] = dot(jacobian[5], jacobian[2]*weight);
	scratch[myid*STRIDE+18] = dot(jacobian[5], jacobian[3]*weight);
	scratch[myid*STRIDE+19] = dot(jacobian[5], jacobian[4]*weight);
	scratch[myid*STRIDE+20] = dot(jacobian[5], jacobian[5]*weight);

	scratch[myid*STRIDE+21] = dot(jacobian[0], residual*weight);
	scratch[myid*STRIDE+22] = dot(jacobian[1], residual*weight);
	scratch[myid*STRIDE+23] = dot(jacobian[2], residual*weight);
	scratch[myid*STRIDE+24] = dot(jacobian[3], residual*weight);
	scratch[myid*STRIDE+25] = dot(jacobian[4], residual*weight);
	scratch[myid*STRIDE+26] = dot(jacobian[5], residual*weight);
	
	scratch[myid*STRIDE+27] = (weight.x!=0)+(weight.y!=0)+(weight.z!=0)+(weight.w!=0);
	scratch[myid*STRIDE+28] = dot(residual, residual*weight);

	if ((myid&31) >= 29) return;

	myid += (myid>=32)*32*(STRIDE-1);
	barrier(CLK_LOCAL_MEM_FENCE);

#pragma unroll
	for (int i = 1; i < 32; i++) 
		scratch[myid] += scratch[myid+i*STRIDE];

	if (myid >= 32) return;
	barrier(CLK_LOCAL_MEM_FENCE);

	scratch[myid] += scratch[myid+32*STRIDE];
	
	int groupid = get_group_id(0)+get_group_id(1)*get_num_groups(0);
	Ab[myid+groupid*32] = scratch[myid];
}
__kernel void MultiWrap(__read_only image2d_t src_depth, 
												__read_only image2d_t dest_depth, 
												__constant float transform[12], 
												__constant float projection[4], 
												__global float*Ab, 
												__write_only image2d_t plotimg) {
#define STRIDE 29

	__local float scratch[STRIDE*8*8]; //Part sums of A and b

	int2 ij = (int2)(get_global_id(0), get_global_id(1));
	int myid = get_local_id(0)+get_local_id(1)*8;
	#pragma unroll
	for (int l = 0; l < 2; l++) {
		#pragma unroll
		for (int k = 0; k < 2; k++) {
			//Deproject 3d point from Z1 depth texture
			float4 pz1 = read_imagef(src_depth, int_sampler, (int2)(ij.x*2+k, ij.y*2+l))+1.f;
	
			//if (pz1 > 1e7) return; //Undefined pixel

			float4 px1 = ((float4)(ij.x*4+k*2, ij.x*4+k*2+1, ij.x*4+k*2, ij.x*4+k*2+1)-projection[0])*pz1*projection[3];
			float4 py1 = ((float4)(ij.y*4+l*2, ij.y*4+l*2, ij.y*4+l*2+1, ij.y*4+l*2+1)-projection[1])*pz1*projection[3];

			//Transform to 3d point in I2 reference (with current estimate)
			float4 x = transform[0]*px1+transform[1]*py1+transform[2]*pz1+transform[3];
			float4 y = transform[4]*px1+transform[5]*py1+transform[6]*pz1+transform[7];
			float4 z = transform[8]*px1+transform[9]*py1+transform[10]*pz1+transform[11];

			//Project to I2 coordinate
			float4 iz = 1.f/z;

			float8 p2 = (float8)(x*projection[2]*iz+projection[0]+0.5f, 
													 y*projection[2]*iz+projection[1]+0.5f);

			float16 dest = (float16)(read_imagef(dest_depth, bilinear, p2.s04),
															 read_imagef(dest_depth, bilinear, p2.s15),
															 read_imagef(dest_depth, bilinear, p2.s26),
															 read_imagef(dest_depth, bilinear, p2.s37));

			//if (dest.w < 0) return;

			//Calculate jacobian of I2
			float4 izz = iz*iz;

			float4 dI2x = dest.s048c*projection[2]*.5f;
			float4 dI2y = dest.s159d*projection[2]*.5f;

			float4 jacobian[6] = {dI2x*iz, 
														dI2y*iz, 
														-(dI2x*x+dI2y*y)*izz-1, 
														-dI2x*x*y*izz-dI2y*(1+y*y*izz)-y, 
														dI2x*(1+x*x*izz)+dI2y*x*y*izz+x, 
														(dI2y*x-dI2x*y)*iz};

			const float ivar = 500.f;
			float4 residual = dest.s26ae+1.f-z;
			float4 weight = 6.0f/(5.0f+residual*residual*ivar);
	
			weight.x *= (pz1.x != 1.f && dest.s3 == 1.f);
			weight.y *= (pz1.y != 1.f && dest.s7 == 1.f);
			weight.z *= (pz1.z != 1.f && dest.sb == 1.f);
			weight.w *= (pz1.w != 1.f && dest.sf == 1.f);

#if defined DEBUG
			const float f = 1.f;
			float4 r = z-1.f, g = (float4)(pz1.x!=1.f,pz1.y!=1.f,pz1.z!=1.f,pz1.w!=1.f)-0.5f, b = dest.s26ae;			
			g = -0.5f;
			write_imagef(plotimg, (int2)(ij.x*2, ij.y*2), (float4)(r.x, g.x, b.x, 0.f)*f+(float4)(0.5f, 0.5f, 0.5f, 0.f));
			write_imagef(plotimg, (int2)(ij.x*2+1, ij.y*2), (float4)(r.y, g.y, b.y, 0.f)*f+(float4)(0.5f, 0.5f, 0.5f, 0.f));
			write_imagef(plotimg, (int2)(ij.x*2, ij.y*2+1), (float4)(r.z, g.z, b.z, 0.f)*f+(float4)(0.5f, 0.5f, 0.5f, 0.f));
			write_imagef(plotimg, (int2)(ij.x*2+1, ij.y*2+1), (float4)(r.w, g.w, b.w, 0.f)*f+(float4)(0.5f, 0.5f, 0.5f, 0.f));
#endif
			if (k == 0 && l == 0) {
				scratch[myid*STRIDE] = dot(jacobian[0], jacobian[0]*weight);
				scratch[myid*STRIDE+1] = dot(jacobian[1], jacobian[0]*weight);
				scratch[myid*STRIDE+2] = dot(jacobian[1], jacobian[1]*weight);
				scratch[myid*STRIDE+3] = dot(jacobian[2], jacobian[0]*weight);
				scratch[myid*STRIDE+4] = dot(jacobian[2], jacobian[1]*weight);
				scratch[myid*STRIDE+5] = dot(jacobian[2], jacobian[2]*weight);
				scratch[myid*STRIDE+6] = dot(jacobian[3], jacobian[0]*weight);
				scratch[myid*STRIDE+7] = dot(jacobian[3], jacobian[1]*weight);
				scratch[myid*STRIDE+8] = dot(jacobian[3], jacobian[2]*weight);
				scratch[myid*STRIDE+9] = dot(jacobian[3], jacobian[3]*weight);
				scratch[myid*STRIDE+10] = dot(jacobian[4], jacobian[0]*weight);
				scratch[myid*STRIDE+11] = dot(jacobian[4], jacobian[1]*weight);
				scratch[myid*STRIDE+12] = dot(jacobian[4], jacobian[2]*weight);
				scratch[myid*STRIDE+13] = dot(jacobian[4], jacobian[3]*weight);
				scratch[myid*STRIDE+14] = dot(jacobian[4], jacobian[4]*weight);
				scratch[myid*STRIDE+15] = dot(jacobian[5], jacobian[0]*weight);
				scratch[myid*STRIDE+16] = dot(jacobian[5], jacobian[1]*weight);
				scratch[myid*STRIDE+17] = dot(jacobian[5], jacobian[2]*weight);
				scratch[myid*STRIDE+18] = dot(jacobian[5], jacobian[3]*weight);
				scratch[myid*STRIDE+19] = dot(jacobian[5], jacobian[4]*weight);
				scratch[myid*STRIDE+20] = dot(jacobian[5], jacobian[5]*weight);

				scratch[myid*STRIDE+21] = dot(jacobian[0], residual*weight);
				scratch[myid*STRIDE+22] = dot(jacobian[1], residual*weight);
				scratch[myid*STRIDE+23] = dot(jacobian[2], residual*weight);
				scratch[myid*STRIDE+24] = dot(jacobian[3], residual*weight);
				scratch[myid*STRIDE+25] = dot(jacobian[4], residual*weight);
				scratch[myid*STRIDE+26] = dot(jacobian[5], residual*weight);
	
				scratch[myid*STRIDE+27] = (weight.x!=0)+(weight.y!=0)+(weight.z!=0)+(weight.w!=0);
				scratch[myid*STRIDE+28] = dot(residual, residual*weight);
			} else {
				scratch[myid*STRIDE] += dot(jacobian[0], jacobian[0]*weight);
				scratch[myid*STRIDE+1] += dot(jacobian[1], jacobian[0]*weight);
				scratch[myid*STRIDE+2] += dot(jacobian[1], jacobian[1]*weight);
				scratch[myid*STRIDE+3] += dot(jacobian[2], jacobian[0]*weight);
				scratch[myid*STRIDE+4] += dot(jacobian[2], jacobian[1]*weight);
				scratch[myid*STRIDE+5] += dot(jacobian[2], jacobian[2]*weight);
				scratch[myid*STRIDE+6] += dot(jacobian[3], jacobian[0]*weight);
				scratch[myid*STRIDE+7] += dot(jacobian[3], jacobian[1]*weight);
				scratch[myid*STRIDE+8] += dot(jacobian[3], jacobian[2]*weight);
				scratch[myid*STRIDE+9] += dot(jacobian[3], jacobian[3]*weight);
				scratch[myid*STRIDE+10] += dot(jacobian[4], jacobian[0]*weight);
				scratch[myid*STRIDE+11] += dot(jacobian[4], jacobian[1]*weight);
				scratch[myid*STRIDE+12] += dot(jacobian[4], jacobian[2]*weight);
				scratch[myid*STRIDE+13] += dot(jacobian[4], jacobian[3]*weight);
				scratch[myid*STRIDE+14] += dot(jacobian[4], jacobian[4]*weight);
				scratch[myid*STRIDE+15] += dot(jacobian[5], jacobian[0]*weight);
				scratch[myid*STRIDE+16] += dot(jacobian[5], jacobian[1]*weight);
				scratch[myid*STRIDE+17] += dot(jacobian[5], jacobian[2]*weight);
				scratch[myid*STRIDE+18] += dot(jacobian[5], jacobian[3]*weight);
				scratch[myid*STRIDE+19] += dot(jacobian[5], jacobian[4]*weight);
				scratch[myid*STRIDE+20] += dot(jacobian[5], jacobian[5]*weight);

				scratch[myid*STRIDE+21] += dot(jacobian[0], residual*weight);
				scratch[myid*STRIDE+22] += dot(jacobian[1], residual*weight);
				scratch[myid*STRIDE+23] += dot(jacobian[2], residual*weight);
				scratch[myid*STRIDE+24] += dot(jacobian[3], residual*weight);
				scratch[myid*STRIDE+25] += dot(jacobian[4], residual*weight);
				scratch[myid*STRIDE+26] += dot(jacobian[5], residual*weight);
	
				scratch[myid*STRIDE+27] += (weight.x!=0)+(weight.y!=0)+(weight.z!=0)+(weight.w!=0);
				scratch[myid*STRIDE+28] += dot(residual, residual*weight);
			}
		}
	}
	if ((myid&31) >= 29) return;

	myid += (myid>=32)*32*(STRIDE-1);
	barrier(CLK_LOCAL_MEM_FENCE);

#pragma unroll
	for (int i = 1; i < 32; i++) 
		scratch[myid] += scratch[myid+i*STRIDE];

	if (myid >= 32) return;
	barrier(CLK_LOCAL_MEM_FENCE);

	scratch[myid] += scratch[myid+32*STRIDE];
	
	int groupid = get_group_id(0)+get_group_id(1)*get_num_groups(0);
	Ab[myid+groupid*32] = scratch[myid];
}


__kernel void SumAb(__global float*Ab_part, __global float*Ab, int parts) {
	int id = get_global_id(0);
	if (id >= 29) return;
	float sum = 0;
	for (int i = 0; i < parts*32; i += 32*4) {
		sum += Ab_part[id+i];
		sum += Ab_part[id+i+32];
		sum += Ab_part[id+i+64];
		sum += Ab_part[id+i+96];
	}
	Ab[id] = sum;
}

__constant sampler_t clampedge = CLK_NORMALIZED_COORDS_FALSE |
	CLK_ADDRESS_CLAMP_TO_EDGE |
	CLK_FILTER_NEAREST;

__kernel void shrink(__read_only image2d_t in, __write_only image2d_t out) {
	int i = get_global_id(0);
	int j = get_global_id(1);
	float a = read_imagef(in, int_sampler, (int2)(i*2, j*2)).x;
	float b = read_imagef(in, int_sampler, (int2)(i*2+1, j*2)).x;
	float c = read_imagef(in, int_sampler, (int2)(i*2, j*2+1)).x;
	float d = read_imagef(in, int_sampler, (int2)(i*2+1, j*2+1)).x;
	float div = (a!=0.f)+(b!=0.f)+(c!=0.f)+(d!=0.f);
	float val = div > 1 ? (a+b+c+d)/div : 0.f;
	write_imagef(out, (int2)(i, j), val);
}

__kernel void shrinkPacked(__read_only image2d_t in, __write_only image2d_t out) {
	int i = get_global_id(0);
	int j = get_global_id(1);
	float val[4];
#pragma unroll
	for (int l = 0; l < 2; l++) {
#pragma unroll
		for (int k = 0; k < 2; k++) {
			float4 v = read_imagef(in, int_sampler, (int2)(i*2+k, j*2+l));
			float div = (v.x!=0.f)+(v.y!=0.f)+(v.z!=0.f)+(v.w!=0.f);
			val[k+l*2] = div > 1 ? (v.x+v.y+v.z+v.w)/div : 0.f;
		}
	}
	write_imagef(out, (int2)(i, j), (float4)(val[0], val[1], val[2], val[3]));
}

__kernel void PackSource(__read_only image2d_t in, __write_only image2d_t out) {
	int i = get_global_id(0);
	int j = get_global_id(1);
	float a = read_imagef(in, int_sampler, (int2)(i*2, j*2)).x;
	float b = read_imagef(in, int_sampler, (int2)(i*2+1, j*2)).x;
	float c = read_imagef(in, int_sampler, (int2)(i*2, j*2+1)).x;
	float d = read_imagef(in, int_sampler, (int2)(i*2+1, j*2+1)).x;
	write_imagef(out, (int2)(i, j), (float4)(a, b, c, d));
}

__kernel void PrepareSource(__read_only image2d_t in, __write_only image2d_t out) {
	int2 ij = (int2)(get_global_id(0), get_global_id(1));
	float4 val = read_imagef(in, int_sampler, ij);
	write_imagef(out, ij, val);
}

__kernel void PrepareDest(__read_only image2d_t in, __write_only image2d_t out) {
	int i = get_global_id(0), j = get_global_id(1);

	float nx = read_imagef(in, clampedge, (int2)(i+1, j)).x;
	float px = read_imagef(in, clampedge, (int2)(i-1, j)).x;
	float ny = read_imagef(in, clampedge, (int2)(i, j+1)).x;
	float py = read_imagef(in, clampedge, (int2)(i, j-1)).x;
	float4 val = (float4)(nx-px, ny-py, 
												read_imagef(in, clampedge, (int2)(i, j)).x, 0.f);
	val.x = (px != 0.f && nx != 0.f ? val.x : 0);
	val.y = (ny != 0.f && py != 0.f ? val.y : 0);
	val.w = (val.z != 0.f);
	write_imagef(out, (int2)(i, j), val);
}
