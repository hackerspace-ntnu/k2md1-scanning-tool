__constant sampler_t int_sampler = CLK_NORMALIZED_COORDS_FALSE |
  CLK_ADDRESS_CLAMP |
  CLK_FILTER_NEAREST;

__constant sampler_t bilinear = CLK_NORMALIZED_COORDS_FALSE |
  CLK_ADDRESS_CLAMP |
  CLK_FILTER_LINEAR;

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
  if (z.x <= 0) iz.x = 1e7;
  if (z.y <= 0) iz.y = 1e7;
  if (z.z <= 0) iz.z = 1e7;
  if (z.w <= 0) iz.w = 1e7;

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

  weight.x *= (pz1.x != 1.f && dest.s3 == 1.f && residual.x > -0.01f);
  weight.y *= (pz1.y != 1.f && dest.s7 == 1.f && residual.y > -0.01f);
  weight.z *= (pz1.z != 1.f && dest.sb == 1.f && residual.z > -0.01f);
  weight.w *= (pz1.w != 1.f && dest.sf == 1.f && residual.w > -0.01f);

#if defined DEBUG
  const float f = 0.5f;
  float4 r = z-(float4)(1.f), g = (float4)(0.5f)-(float4)(pz1.x != 1.f && dest.s3 == 1.f,(pz1.y != 1.f && dest.s7 == 1.f), (pz1.z != 1.f && dest.sb == 1.f), (pz1.w != 1.f && dest.sf == 1.f) ), b = residual*10.f;//dest.s26ae;
  r = sin(r*30.f);
  //b = sin(b*30.f);
  //g = -0.5f;
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
