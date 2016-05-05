#version 440

in layout(location=0) vec2 inPos;
in layout(location=1) vec2 inTex;

out gl_PerVertex
{
	vec4 gl_Position;
};
out layout(location=1) vec2 texVp;

uniform ivec2 viewport;

void main()
{
	texVp = inTex * vec2(viewport);
	gl_Position = vec4(inPos.xy, 0, 1);
}