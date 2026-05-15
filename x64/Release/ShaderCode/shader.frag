#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D ourTexture;

void main() {
	// OpneCV의 좌상단 시작점을 OpenGL의 좌하단 시작점으로 매핑하기 위해 y축 반전
	FragColor = texture(ourTexture, vec2(TexCoord.x, 1.0 - TexCoord.y));
}
