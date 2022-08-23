#include "pch.h"
#include "math.h"



const mat4 mat4::identity =
{
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f,
    0.f, 0.f, 0.f, 1.f,
};

const mat4 mat4::zero =
{
    0.f, 0.f, 0.f, 0.f,
    0.f, 0.f, 0.f, 0.f,
    0.f, 0.f, 0.f, 0.f,
    0.f, 0.f, 0.f, 0.f,
};

mat4 operator*(const mat4& a, const mat4& b)
{
    mat4 result;
    vec4 r0 = row(a, 0);
    vec4 r1 = row(a, 1);
    vec4 r2 = row(a, 2);
    vec4 r3 = row(a, 3);

    vec4 c0 = col(b, 0);
    vec4 c1 = col(b, 1);
    vec4 c2 = col(b, 2);
    vec4 c3 = col(b, 3);
    result.m00 = dot(r0, c0); result.m01 = dot(r0, c1); result.m02 = dot(r0, c2); result.m03 = dot(r0, c3);
    result.m10 = dot(r1, c0); result.m11 = dot(r1, c1); result.m12 = dot(r1, c2); result.m13 = dot(r1, c3);
    result.m20 = dot(r2, c0); result.m21 = dot(r2, c1); result.m22 = dot(r2, c2); result.m23 = dot(r2, c3);
    result.m30 = dot(r3, c0); result.m31 = dot(r3, c1); result.m32 = dot(r3, c2); result.m33 = dot(r3, c3);
    return result;
}

//https://zhuanlan.zhihu.com/p/45404840
quat mat3ToQuaternion(const mat3& m)
{
    float tr = m.m00 + m.m11 + m.m22;

    quat result;
    if (tr > 0.f)
    {
        float s = sqrtf(tr + 1.f) * 2.f; // S=4*qw 
        result.w = 0.25f * s;
        result.x = (m.m21 - m.m12) / s;
        result.y = (m.m02 - m.m20) / s;
        result.z = (m.m10 - m.m01) / s;
    }
    else if ((m.m00 > m.m11) && (m.m00 > m.m22))
    {
        float s = sqrtf(1.f + m.m00 - m.m11 - m.m22) * 2.f; // S=4*qx 
        result.w = (m.m21 - m.m12) / s;
        result.x = 0.25f * s;
        result.y = (m.m01 + m.m10) / s;
        result.z = (m.m02 + m.m20) / s;
    }
    else if (m.m11 > m.m22)
    {
        float s = sqrtf(1.f + m.m11 - m.m00 - m.m22) * 2.f; // S=4*qy
        result.w = (m.m02 - m.m20) / s;
        result.x = (m.m01 + m.m10) / s;
        result.y = 0.25f * s;
        result.z = (m.m12 + m.m21) / s;
    }
    else
    {
        float s = sqrtf(1.f + m.m22 - m.m00 - m.m11) * 2.f; // S=4*qz
        result.w = (m.m10 - m.m01) / s;
        result.x = (m.m02 + m.m20) / s;
        result.y = (m.m12 + m.m21) / s;
        result.z = 0.25f * s;
    }

    return normalize(result);
}

quat rotateFromTo(vec3 inFrom, vec3 inTo)
{
    vec3 from = normalize(inFrom);
    vec3 to = normalize(inTo);
    float d = dot(from, to);
    if(d>=1.f) {
        return quat(0.f, 0.f, 0.f, 1.f);
    }
    quat result;
    if( d < 1e-6f -1.f) {
        vec3 axis = cross( vec3(1.f, 0.f, 0.f), from);
        if(squaredLength(axis) == 0.f)  axis = cross( vec3(0.f, 1.f, 0.f), from);
        axis = normalize(axis);
        result = normalize( quat(axis, M_PI));
    }   else {
        //q.xyz = axis;             (l1*l2 * 2sin* cos)
        //q.w = sqrt((v1.Length ^ 2) * (v2.Length ^ 2)) + dot(v1, v2); =>  1+ dot when normalize from/to  (l1*l2 * cos* cos)
        float s = sqrt((1.f + d) * 2.f);
        float invs = 1.f /s;
        vec3 axis = cross(from, to);
        result.x = axis.x * invs;
        result.y = axis.y * invs;
        result.z = axis.z * invs;
        result.w = s*0.5f;
        result = normalize(result);
    }
    return result;
}

/*-------------------------------------Transform--------------------------------------------------------------*/



trs mat4ToTRS(const mat4& m)
{
    trs result;

    vec3 c0(m.m00, m.m10, m.m20);
    vec3 c1(m.m01, m.m11, m.m21);
    vec3 c2(m.m02, m.m12, m.m22);
    result.scale.x = sqrt(dot(c0, c0));
    result.scale.y = sqrt(dot(c1, c1));
    result.scale.z = sqrt(dot(c2, c2));


    vec3 invScale = 1.f / result.scale;

    result.position.x = m.m03;
    result.position.y = m.m13;
    result.position.z = m.m23;

    mat3 R;
    // normalize manually
    R.m00 = m.m00 * invScale.x;
    R.m10 = m.m10 * invScale.x;
    R.m20 = m.m20 * invScale.x;

    R.m01 = m.m01 * invScale.y;
    R.m11 = m.m11 * invScale.y;
    R.m21 = m.m21 * invScale.y;

    R.m02 = m.m02 * invScale.z;
    R.m12 = m.m12 * invScale.z;
    R.m22 = m.m22 * invScale.z;

    result.rotation = mat3ToQuaternion(R);

    return result;
}

/*-------------------------------------Viewport--------------------------------------------------------------*/

mat4 CreateSkyViewMatrix(const mat4& v)
{
    mat4 result = v;
    result.m03 = 0.f; result.m13 = 0.f; result.m23 = 0.f;
    return result;
}

mat4 CreatePerspectiveProjectionMatrix(float fov, float aspect, float nearPlane, float farPlane)
{
    mat4 result;
    float a =  1.f / tan(0.5f * fov);       //depth / height
    result.m00 = a / aspect;                    //depth / width
    result.m01 = 0.f;
    result.m02 = 0.f;
    result.m03 = 0.f;

    result.m10 = 0.f;
    result.m11 = a;                         
    result.m12 = 0.f;
    result.m13 = 0.f;

    result.m20 = 0.f;
    result.m21 = 0.f;

    if (farPlane > 0.f)
    {
        result.m22 = -farPlane / (farPlane - nearPlane);
        result.m23 = result.m22 * nearPlane;
    }
    else
    {
        result.m22 = -1.f;
        result.m23 = -nearPlane;
    }
    result.m30 = 0.f;
    result.m31 = 0.f;
    result.m32 = -1.f;
    result.m33 = 0.f;

    return result;
}


mat4 CreatePerspectiveProjectionMatrix(float width, float height, float fx, float fy, float cx, float cy, float nearPlane, float farPlane)
{
    mat4 result;
   
    result.m00 = fx / (0.5f * width);                    
    result.m01 = 0.f;
    result.m02 = 1.f - cx / (0.5f * width);
    result.m03 = 0.f;

    result.m10 = 0.f;
    result.m11 = fy / (0.5f * height);                         
    result.m12 = cx / (0.5f * width) - 1.f;         //screen (0,0) stands for top-left
    result.m13 = 0.f;

    result.m20 = 0.f;
    result.m21 = 0.f;

    if (farPlane > 0.f)
    {
        result.m22 = -farPlane / (farPlane - nearPlane);
        result.m23 = result.m22 * nearPlane;
    }
    else
    {
        result.m22 = -1.f;
        result.m23 = -nearPlane;
    }
    result.m30 = 0.f;
    result.m31 = 0.f;
    result.m32 = -1.f;
    result.m33 = 0.f;

    return result;
}

mat4 InvertPerspectiveProjectionMatrix(const mat4& m)
{
    // deduce start from m.row[3] to inv.row[2] 
    mat4 inv;
	
    inv.m00 = 1.f / m.m00;
    inv.m01 = 0.f;
    inv.m02 = 0.f;
    inv.m03 = m.m02 / m.m00;

    inv.m10 = 0.f;
    inv.m11 = 1.f / m.m11;
    inv.m12 = 0.f;
    inv.m13 = m.m12 / m.m11;

    inv.m20 = 0.f;
    inv.m21 = 0.f;
    inv.m22 = 0.f;
    inv.m23 = -1.f;

    inv.m30 = 0.f;
    inv.m31 = 0.f;
    inv.m32 = 1.f / m.m23;
    inv.m33 = m.m22 / m.m23;

    return inv;
}

// [      1       0       0   0 ]   [ xaxis.x  yaxis.x  zaxis.x 0 ]
// [      0       1       0   0 ] * [ xaxis.y  yaxis.y  zaxis.y 0 ]
// [      0       0       1   0 ]   [ xaxis.z  yaxis.z  zaxis.z 0 ]
// [ -eye.x  -eye.y  -eye.z   1 ]   [       0        0        0 1 ]
//
//   [         xaxis.x          yaxis.x          zaxis.x  0 ]
// = [         xaxis.y          yaxis.y          zaxis.y  0 ]
//   [         xaxis.z          yaxis.z          zaxis.z  0 ]
//   [ dot(xaxis,-eye)  dot(yaxis,-eye)  dot(zaxis,-eye)  1 ]  
//  note although we store element in matrix by col major like hlsl, we still use left hand multiply. so transverse above
mat4 LookAt(vec3 eye, vec3 target, vec3 up)
{
    vec3 zAxis = normalize(eye - target);
    vec3 xAxis = normalize(cross(up, zAxis));
    vec3 yAxis = normalize(cross(zAxis, xAxis));

    mat4 result;
    result.m00 = xAxis.x; result.m10 = yAxis.x; result.m20 = zAxis.x; result.m30 = 0.f;
    result.m01 = xAxis.y; result.m11 = yAxis.y; result.m21 = zAxis.y; result.m31 = 0.f;
    result.m02 = xAxis.z; result.m12 = yAxis.z; result.m22 = zAxis.z; result.m32 = 0.f;
    result.m03 = -dot(xAxis, eye); result.m13 = -dot(yAxis, eye); result.m23 = -dot(zAxis, eye); result.m33 = 1.f;

    return result;
}
mat4 CreateViewMatrix(vec3 position, quat rotation)
{
    //look at -z axis. so -near & -far are actual frusta plane
    vec3 target = position + rotation * vec3(0.f, 0.f, -1.f);
    vec3 up = rotation * vec3(0.f, 1.f, 0.f);
    return LookAt(position, target, up);
}

// inv * view * vec = vec
// recall the Prove that rotation matrix is orthogonal
// first it is easy to calculate rotation matrix of x,y,z Axis: Mx,My,Mz that MxT=Mx−1 etc. then combine Mx,My,Mz to get rotation matrix
mat4 InvertAffine(const mat4& m)
{
    vec3 xAxis(m.m00, m.m10, m.m20);
    vec3 yAxis(m.m01, m.m11, m.m21);
    vec3 zAxis(m.m02, m.m12, m.m22);
    vec3 pos(m.m03, m.m13, m.m23);

    vec3 invXAxis = xAxis / squaredLength(xAxis);
    vec3 invYAxis = yAxis / squaredLength(yAxis);
    vec3 invZAxis = zAxis / squaredLength(zAxis);
    vec3 invPos(-dot(invXAxis, pos), -dot(invYAxis, pos), -dot(invZAxis, pos));

    
    mat4 result;
    result.m00 = invXAxis.x; result.m10 = invYAxis.x; result.m20 = invZAxis.x; result.m30 = 0.f;
    result.m01 = invXAxis.y; result.m11 = invYAxis.y; result.m21 = invZAxis.y; result.m31 = 0.f;
    result.m02 = invXAxis.z; result.m12 = invYAxis.z; result.m22 = invZAxis.z; result.m32 = 0.f;
    result.m03 = invPos.x; result.m13 = invPos.y; result.m23 = invPos.z; result.m33 = 1.f;
    return result;
}