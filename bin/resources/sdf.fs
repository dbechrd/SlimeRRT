#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;

// Output fragment color
out vec4 finalColor;

//float median(float r, float g, float b)
//{
//    return max(min(r, g), min(max(r, g), b));
//}

void main()
{
    // Texel color fetching from texture sampler
    // NOTE: Calculate alpha using signed distance field (SDF)
    float distanceFromOutline = texture(texture0, fragTexCoord).a - 0.5;
    float distanceChangePerFragment = length(vec2(dFdx(distanceFromOutline), dFdy(distanceFromOutline)));
    float alpha = smoothstep(-distanceChangePerFragment, distanceChangePerFragment, distanceFromOutline);

    // Calculate final fragment color
    finalColor = vec4(fragColor.rgb, fragColor.a*alpha);

    // 0(30) : error C1031: swizzle mask element not present in operand "z"
    // 0(30) : error C1068: array index out of bounds
    // 0(32) : error C7011: implicit cast from "vec3" to "vec2"
    // 0(37) : error C1503: undefined variable "median"

    // https://medium.com/@calebfaith/implementing-msdf-font-in-opengl-ea09a9ab7e00
    //vec2 flipped_texCoords = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    //vec2 pos = flipped_texCoords.xy;
    //ivec2 sz = textureSize(texture0, 0).xy;
    //float dx = dFdx(pos.x) * sz.x;
    //float dy = dFdy(pos.y) * sz.y;
    //float toPixels = 8.0 * inversesqrt(dx * dx + dy * dy);
    //float sigDist = texture(texture0, fragTexCoord).a;
    //float w = fwidth(sigDist);
    //float opacity = smoothstep(0.5 - w, 0.5 + w, sigDist);
    //finalColor = vec4(fragColor.rgb, fragColor.a*opacity);
}