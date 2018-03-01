#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform sampler2D normalSampler;
layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 VSNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 VSLightPos;
layout(location = 3) in vec3 VSPos;

layout(location = 0) out vec4 outColor;

mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv)
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );
 
    // solve the linear system
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
    // construct a scale-invariant frame 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( -T * invmax, -B * invmax, N );
}

vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord )
{
    // assume N, the interpolated vertex normal and 
    // V, the view vector (vertex to eye)
   vec3 map = texture(normalSampler, texcoord ).xyz;
   map = map * 255./127. - 128./127.;
    mat3 TBN = cotangent_frame(N, -V, texcoord);
    return normalize(TBN * map);
}

void main() {
	vec3 PN = perturb_normal(normalize(VSNormal), normalize(-VSPos), fragTexCoord);
	float LightIntense = 8.0 * pow(dot(PN,normalize(normalize(-VSPos) + normalize(VSLightPos - VSPos))),10.0);
	//float LightIntense = dot(normalize(VSLightPos),PN);
    outColor = LightIntense * texture(texSampler,fragTexCoord);
}