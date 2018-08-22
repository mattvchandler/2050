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
