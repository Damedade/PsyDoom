#version 460

//----------------------------------------------------------------------------------------------------------------------
// Gamma adjustment blit shader: blits an image to an output region with gamma adjustment.
// Does gamma adjustment via a simple 1D texture used as a gamma remapping LUT.
// Uses the 'ndc_textured' vertex shader to pass along the input uv.
//----------------------------------------------------------------------------------------------------------------------
layout(set = 0, binding = 0) uniform sampler2D tex_framebuffer;
layout(set = 0, binding = 1) uniform sampler1D tex_gamma_remap;

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_color;

void main() {
    const vec4 orig_color = texture(tex_framebuffer, in_uv);

    const vec4 adjusted_color = vec4(
        texture(tex_gamma_remap, orig_color.r).r,
        texture(tex_gamma_remap, orig_color.g).r,
        texture(tex_gamma_remap, orig_color.b).r,
        orig_color.a
    ); 

    out_color = adjusted_color;
}
