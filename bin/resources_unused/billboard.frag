#version 150

uniform sampler2D texture;

in vec2 tex_coord;

void main()
{
    // Read and apply a color from the texture
    gl_FragColor = texture2D(texture, tex_coord);
    //gl_FragColor = vec4(tex_coord, 0.0, 1.0);
}
