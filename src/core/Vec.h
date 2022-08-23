#pragma once
#include "simd.h"

template <typename T>
// ReSharper disable once CppClassNeedsConstructorBecauseOfUninitializedMember
struct swizzle_base0
{
protected:
    inline T& elem(size_t i) { return (reinterpret_cast<T*>(_buffer))[i]; }

    inline T const& elem(size_t i) const { return (reinterpret_cast<const T*>(_buffer))[i]; }

    // ReSharper disable once CppUninitializedNonStaticDataMember
    char _buffer[1];
};

template <int N, typename T, template<typename> class VecType, int E0, int E1, int E2, int E3>
struct swizzle_base1 : public swizzle_base0<T>
{
};

template <typename T, template<typename> class VecType, int E0, int E1>
struct swizzle_base1<2, T, VecType, E0, E1, -1, -1> : public swizzle_base0<T>
{
    inline VecType<T> operator()() const { return VecType<T>(this->elem(E0), this->elem(E1)); }
};

template <typename T, template<typename> class VecType, int E0, int E1, int E2>
struct swizzle_base1<3, T, VecType, E0, E1, E2, -1> : public swizzle_base0<T>
{
    inline VecType<T> operator()() const { return VecType<T>(this->elem(E0), this->elem(E1), this->elem(E2)); }
};

template <typename T, template<typename> class VecType, int E0, int E1, int E2, int E3>
struct swizzle_base1<4, T, VecType, E0, E1, E2, E3> : public swizzle_base0<T>
{
    inline VecType<T> operator()() const
    {
        return VecType<T>(this->elem(E0), this->elem(E1), this->elem(E2), this->elem(E3));
    }
};

template <int N, typename T, template<typename> class VecType, int E0, int E1, int E2, int E3, int DUPLICATE_ELEMENTS>
struct swizzle_base2 : public swizzle_base1<N, T, VecType, E0, E1, E2, E3>
{
    swizzle_base2& operator=(const T& t)
    {
        for(int i = 0; i < N; ++i)
            (*this)[i] = t;
        return *this;
    }

#   define APPLY_OP(EXP)               \
inline void                            \
operator EXP (VecType<T> const& that)  \
{                                      \
    struct op {                        \
        inline void                    \
        operator() (T& e, T& t)        \
        { e EXP t; }                   \
    };                                 \
    _apply_op(that, op());             \
}

    swizzle_base2& operator=(VecType<T> const& that)
    {
        struct op
        {
            inline void operator()(T& e, T& t) { e = t; }
        };
        _apply_op(that, op());
        return *this;
    }
    swizzle_base2& operator=(VecType<T>&& that)
    {
        struct op
        {
            inline void operator()(T& e, T& t) { e = t; }
        };
        _apply_op(that, op());
        return *this;
    }

    APPLY_OP(-=)

    APPLY_OP(+=)

    APPLY_OP(*=)

    APPLY_OP(/=)

    inline T& operator[](size_t i)
    {
        const int offset_dst[4] = {E0, E1, E2, E3};
        return this->elem(offset_dst[i]);
    }

    inline T operator[](size_t i) const
    {
        const int offset_dst[4] = {E0, E1, E2, E3};
        return this->elem(offset_dst[i]);
    }

#undef  APPLY_OP

protected:
    template <typename U>
    inline void _apply_op(VecType<T> const& that, U op)
    {
        T t[N];
        for(int i = 0; i < N; ++i)
            t[i] = that.data[i];
        for(int i = 0; i < N; ++i)
            op((*this)[i], t[i]);
    }
    template <typename U>
    inline void _apply_op(VecType<T> && that, U op)
    {
        for(int i = 0; i < N; ++i)
            op((*this)[i], that.data[i]);
    }
};

template <int N, typename T, template<typename> class VecType, int E0, int E1, int E2, int E3>
struct swizzle_base2<N, T, VecType, E0, E1, E2, E3, 1> : public swizzle_base1<N, T, VecType, E0, E1, E2, E3>
{
    struct Stub
    {
    };

    inline swizzle_base2& operator=(Stub const&) { return *this; }

    inline T operator[](size_t i) const
    {
        const int offset_dst[4] = {E0, E1, E2, E3};
        return this->elem(offset_dst[i]);
    }
};

template <int N, typename T, template<typename> class VecType, int E0, int E1, int E2, int E3>
struct swizzle : public swizzle_base2<N, T, VecType, E0, E1, E2, E3, (E0 == E1 || E0 == E2 || E0 == E3 || (E1 == E2 && E1 > 0)
                                          ||
                                          (E1 == E3 && E1 > 0) || (E2 == E3 && E2 > 0))>
{
    typedef swizzle_base2<N, T, VecType, E0, E1, E2, E3, (E0 == E1 || E0 == E2 || E0 == E3 || (E1 == E2 && E1 > 0) ||
                              (E1 == E3 && E1 > 0) || (E2 == E3 && E2 > 0))> base_type;

    using base_type::operator=;
    // swizzle& operator=(const T& rhs)
    // {
    //     return base_type::operator = (rhs);
    // }
    // swizzle& operator=(VecType<T> const& rhs)
    // {
    //     return base_type::operator = (rhs);
    // }
    // swizzle& operator=(VecType<T> && rhs)
    // {
    //     return base_type::operator = (rhs);
    // }

    inline operator VecType<T>() const { return (*this)(); }
};

#define SWIZZLE3_2_MEMBERS(T, V, E0, E1, E2) \
    struct { swizzle<2,T, V, 0,0,-1, -1> E0 ## E0; }; \
    struct { swizzle<2,T, V, 0,1,-1, -1> E0 ## E1; }; \
    struct { swizzle<2,T, V, 0,2,-1, -1> E0 ## E2; }; \
    struct { swizzle<2,T, V, 1,0,-1, -1> E1 ## E0; }; \
    struct { swizzle<2,T, V, 1,1,-1, -1> E1 ## E1; }; \
    struct { swizzle<2,T, V, 1,2,-1, -1> E1 ## E2; }; \
    struct { swizzle<2,T, V, 2,0,-1, -1> E2 ## E0; }; \
    struct { swizzle<2,T, V, 2,1,-1, -1> E2 ## E1; }; \
    struct { swizzle<2,T, V, 2,2,-1, -1> E2 ## E2; };

#define SWIZZLE3_3_MEMBERS(T, V, E0, E1, E2) \
    struct { swizzle<3, T, V, 0,0,0, -1> E0 ## E0 ## E0; }; \
    struct { swizzle<3, T, V, 0,0,1, -1> E0 ## E0 ## E1; }; \
    struct { swizzle<3, T, V, 0,0,2, -1> E0 ## E0 ## E2; }; \
    struct { swizzle<3, T, V, 0,1,0, -1> E0 ## E1 ## E0; }; \
    struct { swizzle<3, T, V, 0,1,1, -1> E0 ## E1 ## E1; }; \
    struct { swizzle<3, T, V, 0,1,2, -1> E0 ## E1 ## E2; }; \
    struct { swizzle<3, T, V, 0,2,0, -1> E0 ## E2 ## E0; }; \
    struct { swizzle<3, T, V, 0,2,1, -1> E0 ## E2 ## E1; }; \
    struct { swizzle<3, T, V, 0,2,2, -1> E0 ## E2 ## E2; }; \
    struct { swizzle<3, T, V, 1,0,0, -1> E1 ## E0 ## E0; }; \
    struct { swizzle<3, T, V, 1,0,1, -1> E1 ## E0 ## E1; }; \
    struct { swizzle<3, T, V, 1,0,2, -1> E1 ## E0 ## E2; }; \
    struct { swizzle<3, T, V, 1,1,0, -1> E1 ## E1 ## E0; }; \
    struct { swizzle<3, T, V, 1,1,1, -1> E1 ## E1 ## E1; }; \
    struct { swizzle<3, T, V, 1,1,2, -1> E1 ## E1 ## E2; }; \
    struct { swizzle<3, T, V, 1,2,0, -1> E1 ## E2 ## E0; }; \
    struct { swizzle<3, T, V, 1,2,1, -1> E1 ## E2 ## E1; }; \
    struct { swizzle<3, T, V, 1,2,2, -1> E1 ## E2 ## E2; }; \
    struct { swizzle<3, T, V, 2,0,0, -1> E2 ## E0 ## E0; }; \
    struct { swizzle<3, T, V, 2,0,1, -1> E2 ## E0 ## E1; }; \
    struct { swizzle<3, T, V, 2,0,2, -1> E2 ## E0 ## E2; }; \
    struct { swizzle<3, T, V, 2,1,0, -1> E2 ## E1 ## E0; }; \
    struct { swizzle<3, T, V, 2,1,1, -1> E2 ## E1 ## E1; }; \
    struct { swizzle<3, T, V, 2,1,2, -1> E2 ## E1 ## E2; }; \
    struct { swizzle<3, T, V, 2,2,0, -1> E2 ## E2 ## E0; }; \
    struct { swizzle<3, T, V, 2,2,1, -1> E2 ## E2 ## E1; }; \
    struct { swizzle<3, T, V, 2,2,2, -1> E2 ## E2 ## E2; };

#define SWIZZLE4_2_MEMBERS(T, V, E0, E1, E2, E3) \
    struct { swizzle<2,T, V, 0,0,-1, -1> E0 ## E0; }; \
    struct { swizzle<2,T, V, 0,1,-1, -1> E0 ## E1; }; \
    struct { swizzle<2,T, V, 0,2,-1, -1> E0 ## E2; }; \
    struct { swizzle<2,T, V, 1,0,-1, -1> E1 ## E0; }; \
    struct { swizzle<2,T, V, 1,1,-1, -1> E1 ## E1; }; \
    struct { swizzle<2,T, V, 1,2,-1, -1> E1 ## E2; }; \
    struct { swizzle<2,T, V, 2,0,-1, -1> E2 ## E0; }; \
    struct { swizzle<2,T, V, 2,1,-1, -1> E2 ## E1; }; \
    struct { swizzle<2,T, V, 2,2,-1, -1> E2 ## E2; }; \
struct { swizzle<2,T, V, 0,3,-1, -1> E0 ## E3; };     \
struct { swizzle<2,T, V, 1,3,-1, -1> E1 ## E3; };     \
struct { swizzle<2,T, V, 2,3,-1, -1> E2 ## E3; };     \
struct { swizzle<2,T, V, 3,3,-1, -1> E3 ## E3; };


#define SWIZZLE4_3_MEMBERS(T, V, E0, E1, E2, E3) \
    struct { swizzle<3, T, V, 0,0,0, -1> E0 ## E0 ## E0; }; \
    struct { swizzle<3, T, V, 0,0,1, -1> E0 ## E0 ## E1; }; \
    struct { swizzle<3, T, V, 0,0,2, -1> E0 ## E0 ## E2; }; \
    struct { swizzle<3, T, V, 0,1,0, -1> E0 ## E1 ## E0; }; \
    struct { swizzle<3, T, V, 0,1,1, -1> E0 ## E1 ## E1; }; \
    struct { swizzle<3, T, V, 0,1,2, -1> E0 ## E1 ## E2; }; \
    struct { swizzle<3, T, V, 0,2,0, -1> E0 ## E2 ## E0; }; \
    struct { swizzle<3, T, V, 0,2,1, -1> E0 ## E2 ## E1; }; \
    struct { swizzle<3, T, V, 0,2,2, -1> E0 ## E2 ## E2; }; \
    struct { swizzle<3, T, V, 1,0,0, -1> E1 ## E0 ## E0; }; \
    struct { swizzle<3, T, V, 1,0,1, -1> E1 ## E0 ## E1; }; \
    struct { swizzle<3, T, V, 1,0,2, -1> E1 ## E0 ## E2; }; \
    struct { swizzle<3, T, V, 1,1,0, -1> E1 ## E1 ## E0; }; \
    struct { swizzle<3, T, V, 1,1,1, -1> E1 ## E1 ## E1; }; \
    struct { swizzle<3, T, V, 1,1,2, -1> E1 ## E1 ## E2; }; \
    struct { swizzle<3, T, V, 1,2,0, -1> E1 ## E2 ## E0; }; \
    struct { swizzle<3, T, V, 1,2,1, -1> E1 ## E2 ## E1; }; \
    struct { swizzle<3, T, V, 1,2,2, -1> E1 ## E2 ## E2; }; \
    struct { swizzle<3, T, V, 2,0,0, -1> E2 ## E0 ## E0; }; \
    struct { swizzle<3, T, V, 2,0,1, -1> E2 ## E0 ## E1; }; \
    struct { swizzle<3, T, V, 2,0,2, -1> E2 ## E0 ## E2; }; \
    struct { swizzle<3, T, V, 2,1,0, -1> E2 ## E1 ## E0; }; \
    struct { swizzle<3, T, V, 2,1,1, -1> E2 ## E1 ## E1; }; \
    struct { swizzle<3, T, V, 2,1,2, -1> E2 ## E1 ## E2; }; \
    struct { swizzle<3, T, V, 2,2,0, -1> E2 ## E2 ## E0; }; \
    struct { swizzle<3, T, V, 2,2,1, -1> E2 ## E2 ## E1; }; \
    struct { swizzle<3, T, V, 2,2,2, -1> E2 ## E2 ## E2; };

#define SWIZZLE4_4_MEMBERS(T, V, E0, E1, E2, E3) \
    struct { swizzle<4, T, V, 0,1,2, 3> E0 ## E1 ## E2 ## E3; };




template <typename T>
union Vec2
{
    struct
    {
        T x, y;
    };
    T data[2];

    Vec2()
    {
    }
    Vec2(T v)
        : Vec2(v, v)
    {
    }
    Vec2(T tx, T ty)
        : x(tx), y(ty)
    {
    }
    static const Vec2 zero;
};


template <typename T>
union Vec3
{
    struct
    {
        T x, y, z;
    };
    T data[3];
    SWIZZLE3_2_MEMBERS(T, ::Vec2, x, y, z)
    SWIZZLE3_3_MEMBERS(T, ::Vec3, x, y, z)


    Vec3()
    {
    }
    Vec3(float v)
        : Vec3(v, v, v)
    {
    }
    Vec3(T tx, T ty, T tz)
        : x(tx), y(ty), z(tz)
    {
    }
    Vec3(Vec2<T> xy, T z)
        : x(xy.x), y(xy.y), z(z)
    {
    }
};

template <typename T>
union Vec4
{
    struct
    {
        T x, y, z, w;
    };
    struct
    {
        T r, g, b, a;
    };
    SWIZZLE4_2_MEMBERS(T, ::Vec2, x, y, z, w)
    SWIZZLE4_3_MEMBERS(T, ::Vec3, x, y, z, w)
    SWIZZLE4_4_MEMBERS(T, ::Vec4, x, y, z, w)
    SWIZZLE4_2_MEMBERS(T, ::Vec2, r, g, b, a)
    SWIZZLE4_3_MEMBERS(T, ::Vec3, r, g, b, a)
    SWIZZLE4_4_MEMBERS(T, ::Vec4, r, g, b, a)

    Vec4()
    {
    }
    Vec4(float v)
        : Vec4(v, v, v, v)
    {
    }
    Vec4(T tx, T ty, T tz, T tw)
        : x(tx), y(ty), z(tz), w(tw)
    {
    }
    Vec4(Vec3<T> xyz, float w)
        : x(xyz.x), y(xyz.y), z(xyz.z), w(w)
    {
    }
};

template <>
union Vec4<float>
{
    struct
    {
        float x, y, z, w;
    };
    struct
    {
        float r, g, b, a;
    };
    SWIZZLE4_2_MEMBERS(float, ::Vec2, x, y, z, w)
    SWIZZLE4_3_MEMBERS(float, ::Vec3, x, y, z, w)
    SWIZZLE4_4_MEMBERS(float, ::Vec4, x, y, z, w)
    SWIZZLE4_2_MEMBERS(float, ::Vec2, r, g, b, a)
    SWIZZLE4_3_MEMBERS(float, ::Vec3, r, g, b, a)
    SWIZZLE4_4_MEMBERS(float, ::Vec4, r, g, b, a)
    w4_float f4;
    float data[4];

    Vec4()
    {
    }
    Vec4(float v)
        : Vec4(v, v, v, v)
    {
    }
    Vec4(float x, float y, float z, float w)
        : x(x), y(y), z(z), w(w)
    {
    }
    Vec4(Vec3<float> xyz, float w)
        : x(xyz.x), y(xyz.y), z(xyz.z), w(w)
    {
    }
    Vec4(w4_float f4)
        : f4(f4)
    {
    }
};