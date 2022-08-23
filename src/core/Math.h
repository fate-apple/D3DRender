#pragma once
#include "pch.h"
#include "Vec.h"

//personal learning use.care less about performance


#define M_PI 3.14159265359f
#define M_PI_OVER_2 (M_PI * 0.5f)
#define M_PI_OVER_180 (M_PI / 180.f)
#define M_180_OVER_PI (180.f / M_PI)
#define deg2rad(deg) ((deg) * M_PI_OVER_180)
#define rad2deg(rad) ((rad) * M_180_OVER_PI)
static constexpr uint32 bucketize(uint32 problemSize, uint32 bucketSize)
{
    return (problemSize + bucketSize - 1) / bucketSize;
}
static constexpr uint64 bucketize(uint64 problemSize, uint64 bucketSize)
{
    return (problemSize + bucketSize - 1) / bucketSize;
}
static constexpr bool isPowerOfTwo(uint32 i)
{
    return (i & (i - 1)) == 0;
}

static float easeInQuadratic(float t)		{ return t * t; }                                                           //Quadratic-bezier(0, 0, 1)
static float easeOutQuadratic(float t)		{ return t * (2.f - t); }
static float easeInOutQuadratic(float t)	{ return (t < 0.5f) ? (2.f * t * t) : (-1.f + (4.f - 2.f * t) * t); }

static float easeInCubic(float t)			{ return t * t * t; }
static float easeOutCubic(float t)			{ float tmin1 = t - 1.f; return tmin1 * tmin1 * tmin1 + 1.f; }
static float easeInOutCubic(float t)		{ return (t < 0.5f) ? (4.f * t * t * t) : ((t - 1.f) * (2.f * t - 2.f) * (2.f * t - 2.f) + 1.f); }

static float easeInQuartic(float t)			{ return t * t * t * t; }
static float easeOutQuartic(float t)		{ float tmin1 = t - 1.f; return 1.f - tmin1 * tmin1 * tmin1 * tmin1; }
static float easeInOutQuartic(float t)		{ float tmin1 = t - 1.f; return (t < 0.5f) ? (8.f * t * t * t * t) : (1.f - 8.f * tmin1 * tmin1 * tmin1 * tmin1); }

static float easeInQuintic(float t)			{ return t * t * t * t * t; }
static float easeOutQuintic(float t)		{ float tmin1 = t - 1.f; return 1.f + tmin1 * tmin1 * tmin1 * tmin1 * tmin1; }
static float easeInOutQuintic(float t)		{ float tmin1 = t - 1.f; return t < 0.5 ? 16.f * t * t * t * t * t : 1.f + 16.f * tmin1 * tmin1 * tmin1 * tmin1 * tmin1; }

static float easeInSine(float t)			{ return sin((t - 1.f) * M_PI_OVER_2) + 1.f; }
static float easeOutSine(float t)			{ return sin(t * M_PI_OVER_2); }
static float easeInOutSine(float t)			{ return 0.5f * (1 - cos(t * M_PI)); }

static float easeInCircular(float t)		{ return 1.f - sqrt(1.f - (t * t)); }
static float easeOutCircular(float t)		{ return sqrt((2.f - t) * t); }
static float easeInOutCircular(float t)		{ return (t < 0.5f) ? (0.5f * (1.f - sqrt(1.f - 4.f * (t * t)))) : (0.5f * (sqrt(-((2.f * t) - 3.f) * ((2.f * t) - 1.f)) + 1.f)); }

static float easeInExponential(float t)		{ return (t == 0.f) ? t : powf(2.f, 10.f * (t - 1.f)); }
static float easeOutExponential(float t)	{ return (t == 1.f) ? t : 1.f - powf(2.f, -10.f * t); }
static float easeInOutExponential(float t)	{ if (t == 0.f || t == 1.f) { return t; } return (t < 0.5f) ? (0.5f * powf(2.f, (20.f * t) - 10.f)) : (-0.5f * powf(2.f, (-20.f * t) + 10.f) + 1.f); }

static float inElastic(float t)				{ return sin(13.f * M_PI_OVER_2 * t) * powf(2.f, 10.f * (t - 1.f)); }
static float outElastic(float t)			{ return sin(-13.f * M_PI_OVER_2 * (t + 1.f)) * powf(2.f, -10.f * t) + 1.f; }
static float inOutElastic(float t)			{ return (t < 0.5f) ? (0.5f * sin(13.f * M_PI_OVER_2 * (2.f * t)) * powf(2.f, 10.f * ((2.f * t) - 1.f))) : (0.5f * (sin(-13.f * M_PI_OVER_2 * ((2.f * t - 1.f) + 1.f)) * powf(2.f, -10.f * (2.f * t - 1.f)) + 2.f)); }

static float inBack(float t)				{ const float s = 1.70158f; return t * t * ((s + 1.f) * t - s); }
static float outBack(float t)				{ const float s = 1.70158f; return --t, 1.f * (t * t * ((s + 1.f) * t + s) + 1.f); }
static float inOutBack(float t)				{ const float s = 1.70158f * 1.525f; return (t < 0.5f) ? (t *= 2.f, 0.5f * t * t * (t * s + t - s)) : (t = t * 2.f - 2.f, 0.5f * (2.f + t * t * (t * s + t + s))); }

#define bounceout(p) ( \
(t) < 4.f/11.f ? (121.f * (t) * (t))/16.f : \
(t) < 8.f/11.f ? (363.f/40.f * (t) * (t)) - (99.f/10.f * (t)) + 17.f/5.f : \
(t) < 9.f/10.f ? (4356.f/361.f * (t) * (t)) - (35442.f/1805.f * (t)) + 16061.f/1805.f \
: (54.f/5.f * (t) * (t)) - (513.f/25.f * (t)) + 268.f/25.f )

static float inBounce(float t)				{ return 1.f - bounceout(1.f - t); }
static float outBounce(float t)				{ return bounceout(t); }
static float inOutBounce(float t)			{ return (t < 0.5f) ? (0.5f * (1.f - bounceout(1.f - t * 2.f))) : (0.5f * bounceout((t * 2.f - 1.f)) + 0.5f); }

#undef bounceout

//used for shader
struct half
{
    uint16 h;

    half() {}
    half(float f);
    half(uint16 i);

    operator float();

    static const half minValue; // Smallest possible half. Not the smallest positive, but the smallest total, so essentially -maxValue.
    static const half maxValue;
};

half operator+(half a, half b);
half& operator+=(half& a, half b);

half operator-(half a, half b);
half& operator-=(half& a, half b);

half operator*(half a, half b);
half& operator*=(half& a, half b);

half operator/(half a, half b);
half& operator/=(half& a, half b);

//only vec4 has simd
// using vec4 = Vec4<float>;
// using vec3 = Vec3<float>;
// using vec2 = Vec2<float>;
typedef Vec2<float> vec2;
typedef Vec3<float> vec3;
typedef Vec4<float> vec4;

static vec3 operator+(vec3 a, vec3 b) { vec3 result = { a.x + b.x, a.y + b.y, a.z + b.z }; return result; }
static vec4 operator+(vec4 a, vec4 b) { vec4 result = { a.f4 + b.f4 }; return result; }
static vec2 operator-(vec2 a, vec2 b) { vec2 result = { a.x - b.x, a.y - b.y }; return result; }
static vec3 operator-(vec3 a, vec3 b) { vec3 result = { a.x - b.x, a.y - b.y, a.z - b.z }; return result; }
static vec4 operator-(vec4 a, vec4 b) { vec4 result = { a.f4 - b.f4 }; return result; }
static vec2 operator*(vec2 a, float b) { vec2 result = { a.x * b, a.y * b }; return result; }
static vec2 operator*(float a, vec2 b) { return b * a; }
static vec3 operator*(vec3 a, float b) { vec3 result = { a.x * b, a.y * b, a.z * b }; return result; }
static vec4 operator*(vec4 a, float b) { vec4 result = { a.f4 * w4_float(b) }; return result; }
static vec2 operator*(vec2 a, vec2 b) { vec2 result = { a.x * b.x, a.y * b.y }; return result; }
static vec3 operator/(vec3 a, float b) { vec3 result = { a.x / b, a.y / b, a.z / b }; return result; }
static vec4 operator/(vec4 a, vec4 b) { vec4 result = { a.f4 / b.f4 }; return result; }

static float dot(vec2 a, vec2 b) { float result = a.x * b.x + a.y * b.y; return result; }
static float dot(vec3 a, vec3 b) { float result = a.x * b.x + a.y * b.y + a.z * b.z; return result; }
static float dot(vec4 a, vec4 b) { w4_float m = a.f4 * b.f4; return addElements(m); }
static float squaredLength(vec2 a) { return dot(a, a); }
static float squaredLength(vec3 a) { return dot(a, a); }
static float squaredLength(vec4 a) { return dot(a, a); }
static float length(vec2 a) { return sqrt(squaredLength(a)); }
static float length(vec3 a) { return sqrt(squaredLength(a)); }
static float length(vec4 a) { return sqrt(squaredLength(a)); }
static vec2 normalize(vec2 a) { float l = length(a); return a * (1.f / l); }
static vec3 normalize(vec3 a) { float l = length(a); return a * (1.f / l); }
static vec4 normalize(vec4 a) { float l = length(a); return a * (1.f / l); }
static vec2 abs(vec2 a) { return vec2(abs(a.x), abs(a.y)); }
static vec3 abs(vec3 a) { return vec3(abs(a.x), abs(a.y), abs(a.z)); }
static vec4 abs(vec4 a) { return vec4(abs(a.f4)); }

static vec3 cross(vec3 a, vec3 b) { vec3 result = { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; return result; }



union mat2
{
    struct
    {
        float
            m00, m10,
            m01, m11;
    };
    struct
    {
        vec2 col0;
        vec2 col1;
    };
    vec2 cols[2];
    float m[4];

    mat2() {}
    mat2(
        float m00, float m01,
        float m10, float m11);

    static const mat2 identity;
    static const mat2 zero;
};

union mat3
{

    struct
    {
        float
            m00, m10, m20,
            m01, m11, m21,
            m02, m12, m22;
    };
    struct
    {
        vec3 col0;
        vec3 col1;
        vec3 col2;
    };
    vec3 cols[3];
    float m[9];

    mat3() {}
    mat3(
        float m00, float m01, float m02,
        float m10, float m11, float m12,
        float m20, float m21, float m22);

    static const mat3 identity;
    static const mat3 zero;
};

union mat4
{
    struct
    {
        float
            m00, m10, m20, m30,
            m01, m11, m21, m31,
            m02, m12, m22, m32,
            m03, m13, m23, m33;
    };
    struct
    {
        vec4 col0;
        vec4 col1;
        vec4 col2;
        vec4 col3;
    };
    vec4 cols[4];
    float m[16];

    mat4() {}  // NOLINT(cppcoreguidelines-pro-type-member-init)
    mat4(
    float m00, float m01, float m02, float m03,
    float m10, float m11, float m12, float m13,
    float m20, float m21, float m22, float m23,
    float m30, float m31, float m32, float m33)
    :
    m00(m00), m10(m10), m20(m20), m30(m30),
    m01(m01), m11(m11), m21(m21), m31(m31),
    m02(m02), m12(m12), m22(m22), m32(m32),
    m03(m03), m13(m13), m23(m23), m33(m33) {}

    static const mat4 identity;
    static const mat4 zero;
};
static vec4 row(const mat4& a, uint32 r) { return { a.m[r], a.m[r + 4], a.m[r + 8], a.m[r + 12] }; }
static vec4 col(const mat4& a, uint32 c) { return a.cols[c]; }
mat4 operator*(const mat4& a, const mat4& b);

union quat
{
    struct 
    {
        float x,y,z,w;
    };
    struct 
    {
        vec3 v;
        float cosHalfAngle;
    };
    vec4 v4;
    w4_float f4;

    quat() {};      //default is deleted
    quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w){}
    quat(vec3 axis, float angle);
    quat(vec4 v4) : v4(v4) {}
    quat(w4_float f4) : f4(f4) {}
    static const quat identity;
    static const quat zero;
};

struct dual_quat
{
    quat real;
    quat dual;

    dual_quat() {}
    dual_quat(quat real, quat dual) : real(real), dual(dual) {}
    dual_quat(quat rotation, vec3 translation);

    vec3 getTranslation();
    quat getRotation();

    static const dual_quat identity;
    static const dual_quat zero;
};

static quat normalize(quat a) { return { normalize(a.v4).f4 }; }
static quat conjugate(quat a) { return { -a.x, -a.y, -a.z, a.w }; }

static quat operator*(quat a, quat b)
{
    //note we has stored cosHalfAngle
    quat result;
    result.w = a.w * b.w - dot(a.v, b.v);
    result.v = a.v * b.w + b.v * a.w + cross(a.v, b.v);
    return result;
}
static vec3 operator*(quat q, vec3 v)
{
    quat p(v.x, v.y, v.z, 0.f);
    return (q * p * conjugate(q)).v;
}

quat mat3ToQuaternion(const mat3& m);

quat rotateFromTo(vec3 from, vec3 to);





struct trs
{
    quat rotation;
    vec3 position;
    vec3 scale;

    trs() {}
    trs(vec3 position, quat rotation = quat::identity, vec3 scale = { 1.f, 1.f, 1.f }) : position(position), rotation(rotation), scale(scale) {}

    static const trs identity;
};
trs mat4ToTRS(const mat4& m);

/*-------------------------------------Viewport--------------------------------------------------------------*/
mat4 CreateSkyViewMatrix(const mat4& v);
mat4 CreatePerspectiveProjectionMatrix(float fov, float aspect, float nearPlane, float farPlane);
mat4 CreatePerspectiveProjectionMatrix(float width, float height, float fx, float fy, float cx, float cy, float nearPlane, float farPlane);
mat4 InvertPerspectiveProjectionMatrix(const mat4& m);
mat4 LookAt(vec3 eye, vec3 target, vec3 up);
mat4 CreateViewMatrix(vec3 position, quat rotation);
mat4 InvertAffine(const mat4& m);

/*-------------------------------------animation--------------------------------------------------------------*/
// used in hlsli
static constexpr float clamp(float v, float l, float u) { float r = max(l, v); r = min(u, r); return r; }
static constexpr uint32 clamp(uint32 v, uint32 l, uint32 u) { uint32 r = max(l, v); r = min(u, r); return r; }
static constexpr int32 clamp(int32 v, int32 l, int32 u) { int32 r = max(l, v); r = min(u, r); return r; }
static constexpr float clamp01(float v) { return clamp(v, 0.f, 1.f); }
static constexpr float saturate(float v) { return clamp01(v); }

static constexpr float lerp(float l, float u, float t) { return l + t * (u - l); }
static constexpr float inverseLerp(float l, float u, float v) { return (v - l) / (u - l); }

template <typename  T>
static T evaluateSpline(const float* ts, const T* values, int32 num, float t)
{
    assert(num > 2);
    int32 k = 0;
    while( k < num -1 && ts[k+1] >= 0.f && ts[k] < t) {
        ++k;
    }
    if (ts[k + 1] < 0.f)
    {
        num = k + 1;
    }
    const float h1 = clamp01(inverseLerp(ts[k - 1], ts[k], t));
    const float h2 = h1 * h1;
    const float h3 = h2 * h1;
    const vec4 h(h3, h2, h1, 1.f);
    T result;
    if constexpr (std::is_same_v<std::remove_cv_t<T>, quat> || std::is_same_v<std::remove_cv_t<T>, dual_quat>)
    {
        result = T::zero;
    }
    else
    {
        result = 0;
    }
    int32 m = num - 1;
    result += values[clamp(k - 2, 0, m)] * dot(vec4(-1, 2, -1, 0), h);
    result += values[k - 1] * dot(vec4(3, -5, 0, 2), h);
    result += values[k] * dot(vec4(-3, 4, 1, 0), h);
    result += values[clamp(k + 1, 0, m)] * dot(vec4(1, -1, 0, 0), h);

    result *= 0.5f;

    return result;
}
