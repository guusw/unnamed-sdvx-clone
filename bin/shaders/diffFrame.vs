#version 330
#extension GL_ARB_separate_shader_objects : enable

in layout(location=0) vec2 inPos;
in layout(location=1) vec2 inTex;

out gl_PerVertex
{
	vec4 gl_Position;
};
out layout(location=1) vec2 fsTex;

uniform mat4 proj;
uniform mat4 world;

void main()
{
	fsTex = inTex;
	fsTex.y = 1.0 - fsTex.y;
	gl_Position = proj * world * vec4(inPos.xy, 0, 1);
}