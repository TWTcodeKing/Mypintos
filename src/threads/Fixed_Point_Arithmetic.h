/*provide basic fp arithmetic for kernels*/
#ifndef _FIXED_POINT_ARITHMETIC_H
#define _FIXED_POINT_ARITHMETIC_H

/*规定默认整数定点数表示浮点数*/
typedef int64_t fixed_t;

#define FIXED_POINTS_SHIFT 12 // 规定f=16
#define FIXED_POINTS_CONST(A) ((fixed_t)(A<<FIXED_POINTS_SHIFT)) //浮点数转化为它的整数表示
#define FIXED_POINTS_ADD(A,B) (A+B)

/*add mixture fixed-points with integer*/
#define FIXED_POINTS_ADD_MIX(A,B) (A+(B<<FIXED_POINTS_SHIFT))

#define FIXED_POINTS_SUB(A,B) (A-B)
#define FIXED_POINTS_SUB_MIX(A,B)(A-(B<<FIXED_POINTS_SHIFT))
#define FIXED_POINTS_MUL_MIX(A,B)(A*B)
#define FIXED_POINTS_DIV_MIX(A,B)(A/B)
#define FIXED_POINTS_MUL(A,B)((fixed_t)(((int64_t)A)*B)>>FIXED_POINTS_SHIFT)

#define FIXED_POINTS_DIV(A,B)((fixed_t)((((int64_t)A)<<FIXED_POINTS_SHIFT)/B))

//convert back to fixed_points
#define FIXED_POINTS_INVER(A)(A>>FIXED_POINTS_SHIFT)


#define FIXED_POINTS_ROUND(A) (A >= 0 ? ((A + (1 << (FIXED_POINTS_SHIFT - 1))) >>FIXED_POINTS_SHIFT) \
				: ((A - (1 << (FIXED_POINTS_SHIFT - 1))) >> FIXED_POINTS_SHIFT))

#endif