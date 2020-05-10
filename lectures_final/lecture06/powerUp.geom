#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 200) out;

in vec2 UV[];
in vec3 vertexNormal[];

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelMatrix;

uniform float time;
uniform int explodeValue;

out vec2 interp_UV;

float magnitude = 8.0;

void NewVertex(vec4 center, vec3 offset){
	vec4 newVertexPos = projectionMatrix * viewMatrix * modelMatrix * (center + vec4(offset * 0.2, 0.0));
	gl_Position = newVertexPos;
	EmitVertex();
}

vec4 Explode(vec4 position, vec3 normal)
{
    vec3 direction = normal * explodeValue * time * magnitude;
    return position + vec4(direction, 0.0);
} 

void Implode(int vertex){
	vec4 center = gl_in[vertex].gl_Position + vec4(vertexNormal[vertex] * -explodeValue * time * magnitude, 0.0);
	
	NewVertex(center, vec3(-1.0, -1.0, 0.0));
	NewVertex(center, vec3(1.0, -1.0, 0.0));
	NewVertex(center, vec3(-1.0, 1.0, 0.0));
	NewVertex(center, vec3(1.0, 1.0, 0.0));
	EndPrimitive();
}

vec3 GetNormal()
{
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
    return normalize(cross(b, a));
}

void main(){
	for(int i = 0; i < gl_in.length(); i++){
		interp_UV = UV[i];
		gl_Position = projectionMatrix * viewMatrix * modelMatrix * Explode(gl_in[i].gl_Position, GetNormal());
		EmitVertex();
	}
	EndPrimitive();
	if(explodeValue != 0){
		Implode(0);
		Implode(1);
		Implode(2);
	}
}