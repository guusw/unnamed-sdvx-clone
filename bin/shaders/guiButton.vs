#version 440
in layout(location=0) vec3 inPos;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	gl_Position = vec4(inPos,1);
}