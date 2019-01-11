// Copyright 2019 Matthew Chandler

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

varying vec3 color;
varying vec2 center;
varying float frag_radius;
varying float border_size;
varying float pixel_size;

uniform mat3 inv_projection;
uniform vec2 screen_size;
uniform float win_size;

void main()
{
    vec2 coord = (inv_projection * vec3(2.0 * gl_FragCoord.xy / screen_size - vec2(1.0), 1.0)).xy;
    float r = distance(coord, center);

    if(r >= frag_radius)
        discard;

    r /= frag_radius;

    float alpha = 1.0 - smoothstep(1.0 - 4.0 * pixel_size, 1.0, r);
    gl_FragColor = vec4(mix(color, vec3(0.0), smoothstep(1.0 - border_size - 2.0 * pixel_size, 1.0 - border_size + 2.0 * pixel_size, r)), alpha);
}
