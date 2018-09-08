/// @file
/// @brief Vector and matrix types

// Copyright 2018 Matthew Chandler

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef TYPES_HPP
#define TYPES_HPP

#include <utility>
#include <cstdlib>

#ifdef USE_GLM
#include <glm/glm.hpp>
#endif


/// OpenGL Font rendering types

/// @ingroup textogl
namespace textogl
{
    namespace detail
    {
        // fallback types to use if GLM is not available

        /// 2D Vector
        template<typename T>
        struct Vec2
        {
            union {T x = {}, r;}; ///< X / R component
            union {T y = {}, g;}; ///< Y / G component

            Vec2() = default;
            Vec2(T x, T y): x(x), y(y) {}

            /// Access component by index

            /// To pass vector to OpenGL, do: <tt>&vec2[0]</tt>
            /// @{
            T & operator[](std::size_t i) { return (&x)[i]; }
            const T & operator[](std::size_t i) const { return (&x)[i]; }
            /// @}
        };

        /// 4D Vector
        template<typename T>
        struct Vec4
        {
            union {T x = {}, r;}; ///< X / R component
            union {T y = {}, g;}; ///< Y / G component
            union {T z = {}, b;}; ///< Z / B component
            union {T w = {}, a;}; ///< W / A component

            Vec4() = default;
            Vec4(T x, T y, T z, T w): x(x), y(y), z(z), w(w) {}

            /// Access component by index

            /// To pass vector to OpenGL, do: <tt>&vec4[0]</tt>
            /// @{
            T & operator[](std::size_t i) { return (&x)[i]; }
            const T & operator[](std::size_t i) const { return (&x)[i]; }
            /// @}
        };

        /// 4x4 Matrix
        template <typename T>
        class Mat4
        {
        private:
            Vec4<T> _data[4]{{}, {}, {}, {}};
        public:

            Mat4() = default;

            Mat4(Vec4<T> & x, const Vec4<T> & y, const Vec4<T> & z, const Vec4<T> &w)
            {
                _data[0] = x; _data[1] = y; _data[2] = z; _data[3] = w;
            }

            Mat4(T xx, T xy, T xz, T xw,
                 T yx, T yy, T yz, T yw,
                 T zx, T zy, T zz, T zw,
                 T wx, T wy, T wz, T ww)
            {
                _data[0][0] = xx; _data[0][1] = xy; _data[0][2] = xz; _data[0][3] = xw;
                _data[1][0] = yx; _data[1][1] = yy; _data[1][2] = yz; _data[1][3] = yw;
                _data[2][0] = zx; _data[2][1] = zy; _data[2][2] = zz; _data[2][3] = zw;
                _data[3][0] = wx; _data[3][1] = wy; _data[3][2] = wz; _data[3][3] = ww;
            }

            Mat4(T diagonal): Mat4(diagonal, 0, 0, 0,
                                   0, diagonal, 0, 0,
                                   0, 0, diagonal, 0,
                                   0, 0, 0, diagonal)
            {}

            /// Access component by index

            /// To pass vector to OpenGL, do: <tt>&mat4[0][0]</tt>
            /// @{
            Vec4<T> & operator[](std::size_t i) { return _data[i]; }
            const Vec4<T> & operator[](std::size_t i) const { return _data[i]; }
            /// @}

#ifndef USE_GLM
            /// Multiply 2 Mat4s
            template <typename U>
            Mat4<decltype(std::declval<T>() * std::declval<U>())> operator*(const Mat4<U> & b) const
            {
                Mat4<T> out;
                for(int col = 0; col < 4; ++col)
                {
                    for(int row = 0; row < 4; ++row)
                    {
                        out[col][row] = 0;
                        for(int k = 0; k < 4; ++k)
                            out[col][row] += _data[k][row] * b[col][k];
                    }
                }
                return out;
            }

            /// Multiply 2 Mat4s
            template <typename U>
            Mat4<decltype(std::declval<T>() * std::declval<U>())> & operator*=(const Mat4<U> & b)
            {
                return *this = *this * b;
            }
#endif
        };

        // for template alias specialization
        template<typename T> struct Vec2_t {  using type = Vec2<T>; };
        template<typename T> struct Vec4_t {  using type = Vec4<T>; };
        template<typename T> struct Mat4_t {  using type = Mat4<T>; };

#ifdef USE_GLM
        // specialize to glm types
        template<> struct Vec2_t<float>        { using type = glm::vec2; };
        template<> struct Vec2_t<double>       { using type = glm::dvec2; };
        template<> struct Vec2_t<int>          { using type = glm::ivec2; };
        template<> struct Vec2_t<unsigned int> { using type = glm::uvec2; };

        template<> struct Vec4_t<float>        { using type = glm::vec4; };
        template<> struct Vec4_t<double>       { using type = glm::dvec4; };
        template<> struct Vec4_t<int>          { using type = glm::ivec4; };
        template<> struct Vec4_t<unsigned int> { using type = glm::uvec4; };

        template<> struct Mat4_t<float>        { using type = glm::mat4; };
        template<> struct Mat4_t<double>       { using type = glm::dmat4; };
#endif
    }

    /// 2D Vector

    /// @note If GLM is available, this is an alias for glm::vec2 / dvec2 / ...
    template<typename T = float> using Vec2 = typename detail::Vec2_t<T>::type;

    /// 4D Vector

    /// @note If GLM is available, this is an alias for glm::vec4 / dvec4 / ...
    template<typename T = float> using Vec4 = typename detail::Vec4_t<T>::type;

    /// 4D Matrix

    /// @note If GLM is available, this is an alias for glm::mat4 / dmat4
    template<typename T = float> using Mat4 = typename detail::Mat4_t<T>::type;

    /// %Color vector

    /// @note If GLM is available, this is an alias for glm::vec4
    using Color = Vec4<float>;

}

#endif // TYPES_HPP
