#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "mesh.h"

class Spring
{
public:
	Spring(Vertex* node1, Vertex* node2, float hookC, float dampC);
	void Simulate(float timeStamp);	

public:
	Vertex* node1;
	Vertex* node2;
	float originLength;
	float hookC;		// hook coefficient
	float dampC;		// damp coefficient
};