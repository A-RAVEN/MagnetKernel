#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(std140, push_constant) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	mat4 model;
	vec3 light;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 VSNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 VSLightPos;
layout(location = 3) out vec3 VSPos;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	vec4 VSPosRaw = ubo.view * ubo.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * VSPosRaw;
    VSNormal = mat3(ubo.view * ubo.model) * inNormal;
	VSLightPos = vec3(ubo.view * vec4(ubo.light,1.0));
	VSPos = vec3(VSPosRaw);
	fragTexCoord = inTexCoord;
}