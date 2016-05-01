#version 440
in layout(location=0) vec2 inPos;
in layout(location=1) vec2 inTex;

out gl_PerVertex
{
	vec4 gl_Position;
};
out layout(location=1) vec2 fsTex;

uniform mat4 proj;
uniform mat4 camera;
uniform mat4 world;

void main()
{
	fsTex = inTex;
	fsTex.y = -fsTex.y;
	gl_Position = proj * camera * world * vec4(inPos.xy, 0, 1);
}