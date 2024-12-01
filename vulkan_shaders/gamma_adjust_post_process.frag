#version 460

//----------------------------------------------------------------------------------------------------------------------
// Gamma adjustment post process shader: applies gamma adjustment to an input attachment.
// Does gamma adjustment via a simple 1D texture used as a gamma remapping LUT.
// Uses the 'ndc_position_only' vertex shader.
//----------------------------------------------------------------------------------------------------------------------
layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputAttachment;
layout(set = 0, binding = 1) uniform sampler1D tex_gamma_remap;

layout(location = 0) out vec4 out_color;

void main() {
    const vec4 orig_color = subpassLoad(inputAttachment);

    const vec4 adjusted_color = vec4(
        texture(tex_gamma_remap, orig_color.r).r,
        texture(tex_gamma_remap, orig_color.g).r,
        texture(tex_gamma_remap, orig_color.b).r,
        orig_color.a
    ); 

    out_color = adjusted_color;
}
