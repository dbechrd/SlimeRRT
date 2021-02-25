#version 150

// The render target's resolution (used for scaling)
uniform vec2 resolution;

// The billboards' size
uniform vec2 size;

uniform float angle;

uniform isampler1D tile_map;    // contains tileType of every tile in the map
uniform int tile_perRow;        // number of tiles per row of texture atlas
uniform float tile_w_pct;       // tile width in 0-1 range (% of texture size)
uniform float tile_h_pct;       // tile width in 0-1 range (% of texture size)

// Input is the passed point cloud
layout (points) in;

// The output will consist of triangle strips with four vertices each
layout (triangle_strip, max_vertices = 4) out;

// Output texture coordinates
out vec2 tex_coord;

vec2 rotate(vec2 p)
{
    float c = cos(angle);
    float s = sin(angle);

    //vec3 p = vec3(p2, 0.0);
    //
    //// Axis of rotation
    //float x = 0.0;
    //float y = 0.0;
    //float z = 1.0;
    //
    //vec3 q;
    //q.x = p.x * (x*x * (1.0 - c) + c)
    //    + p.y * (x*y * (1.0 - c) + z * s)
    //    + p.z * (x*z * (1.0 - c) - y * s);
    //
    //q.y = p.x * (y*x * (1.0 - c) - z * s)
    //    + p.y * (y*y * (1.0 - c) + c)
    //    + p.z * (y*z * (1.0 - c) + x * s);
    //
    //q.z = p.x * (z*x * (1.0 - c) + y * s)
    //    + p.y * (z*y * (1.0 - c) - x * s)
    //    + p.z * (z*z * (1.0 - c) + c);
    //
    //return q.xy;

    vec2 q;
    q.x = p.x * c + p.y * s;
    q.y = p.x *-s + p.y * c;
    return q;
}

// Main entry point
void main()
{
    //int tileType = tileTypes[gl_PrimitiveID];

    // Iterate over all input vertices
    for (int i = 0; i < gl_in.length(); i++)
    {
        float u = float(texelFetch(tile_map, i, 0) % tile_perRow) * tile_w_pct;
        float v = float(texelFetch(tile_map, i, 0) / tile_perRow) * tile_h_pct;

        // Retrieve the passed vertex position
        vec2 pos = gl_in[i].gl_Position.xy;

        vec2 bottomLeft  = vec2(-size.x, -size.y);
        vec2 bottomRight = vec2(+size.x, -size.y);
        vec2 topLeft     = vec2(-size.x, +size.y);
        vec2 topRight    = vec2(+size.x, +size.y);

        bottomLeft  = pos + rotate(bottomLeft ) / resolution;
        bottomRight = pos + rotate(bottomRight) / resolution;
        topLeft     = pos + rotate(topLeft    ) / resolution;
        topRight    = pos + rotate(topRight   ) / resolution;

        // Bottom left vertex
        gl_Position = vec4(bottomLeft, 0.f, 1.f);
        tex_coord = vec2(u, v);
        EmitVertex();

        // Bottom right vertex
        gl_Position = vec4(bottomRight, 0.f, 1.f);
        tex_coord = vec2(u + tile_w_pct, v);
        EmitVertex();

        // Top left vertex
        gl_Position = vec4(topLeft, 0.f, 1.f);
        tex_coord = vec2(u, v + tile_h_pct);
        EmitVertex();

        // Top right vertex
        gl_Position = vec4(topRight, 0.f, 1.f);
        tex_coord = vec2(u + tile_w_pct, v + tile_h_pct);
        EmitVertex();

        // And finalize the primitive
        EndPrimitive();
    }
}
