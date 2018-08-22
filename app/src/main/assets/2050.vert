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
