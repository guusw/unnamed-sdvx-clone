#version 330
#extension GL_ARB_separate_shader_objects : enable

in layout(location=1) vec2 fsTex;
out layout(location=0) vec4 target;

uniform sampler2D mainTex;

void main()
{	
	vec4 mainColor = texture(mainTex, fsTex.xy);
	target = mainColor;
	//target = vec4(0, fsTex.y, 0, 1);
}