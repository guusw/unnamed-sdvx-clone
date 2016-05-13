#version 440
in layout(location=0) vec3 inPos;
in layout(location=1) vec4 inColor;
in layout(location=2) vec4 inParams;

out gl_PerVertex
{
	vec4 gl_Position;
};
out layout(location=1) vec4 fsColor;
out layout(location=2) vec4 fsParams;

void main()
{
	fsColor = inColor;
	fsParams = inParams;
	gl_Position = vec4(inPos,1);
}