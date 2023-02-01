#include "Spring.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Spring::Spring(Vertex* node1, Vertex* node2, float hookC, float dampC)
	:node1(node1), node2(node2), hookC(hookC), dampC(dampC)
{
	originLength = glm::length(node1->oldPos - node2->oldPos);
}


void Spring::Simulate(float etha)
{
	float currentLength = glm::length(node1->Position - node2->Position);
	glm::vec3 f = glm::vec3(node2->Position - node1->Position);
	glm::vec3 forceDir = f / glm::length(f);
	glm::vec3 velocityDiff = node2->velocity - node1->velocity;

	float NormaliseHock = (float)(currentLength - originLength) * hookC;
	float NormaliseDamp = glm::dot(forceDir, velocityDiff) * dampC;
	glm::vec3 force = forceDir * (NormaliseHock + NormaliseDamp);

	node2->force += -force;
	node1->force += force;
}