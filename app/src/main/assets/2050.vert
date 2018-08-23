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

precision mediump float;

attribute vec2 vert_pos;
attribute vec2 ball_pos;
attribute float radius;
attribute vec3 vert_color;

uniform mat3 projection;
uniform float win_size;
varying float border_size;
varying float pixel_size;

varying vec3 color;
varying vec2 center;
varying float frag_radius;

const float border_thickness = 2.0;

void main()
{
    color = vert_color;
    gl_Position = vec4((projection * vec3(vert_pos * 2.0 * radius + ball_pos, 1.0)).xy, 0.0, 1.0);
    center = ball_pos;
    frag_radius = radius;
    border_size = border_thickness / frag_radius;
    pixel_size = 1.0 / win_size;
}
