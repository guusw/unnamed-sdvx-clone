#version 440

in layout(location=1) vec4 fsColor;
in layout(location=2) vec2 fsTex;
out layout(location=0) vec4 target;

uniform sampler2D mainTex;
		
void main()
{
	target = fsColor * texture(mainTex, fsTex);
}