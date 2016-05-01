#version 440

in layout(location=1) vec2 fsTex;
out layout(location=0) vec4 target;

uniform sampler2D mainTex;
uniform vec4 color;

void main()
{
	target = texture(mainTex, fsTex) * color;
}