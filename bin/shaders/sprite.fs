#version 330
#extension GL_ARB_separate_shader_objects : enable

in layout(location=1) vec2 fsTex;
out layout(location=0) vec4 target;

uniform sampler2D mainTex;
uniform vec4 color;

void main()
{	
	vec4 mainColor = texture(mainTex, fsTex.xy);
	target = mainColor * color;
}