//------------------------------------------------------------------------------
// NOTES:
// Normally you wouldn't get any of this file for this assignment. But this year
// is an extraordinary year and given the timeline left for the class, we are
// aiming to provide you with more than we have in previous years.
//
// The intent is to provide you with enough to implement on your own that you
// are able to learn a wide breadth of stuff about ray-tracing, but without
// requiring you to put in the time that it would normally take to write a
// ray-tracer from scratch.
//------------------------------------------------------------------------------
#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <iostream>

#include "Material.h"

using namespace std;
using namespace glm;
float dot_normalized(vec3 v1, vec3 v2);
void debug(char* str, vec3 a);

struct Ray {
	vec3 origin;
	vec3 direction;

	Ray(vec3 point, vec3 dir){
		origin = point;
		direction = dir;
	}
	Ray(): origin(vec3(0,0,0)), direction(vec3(0,0,0))
	{}
};

struct Intersection{
	int numberOfIntersections;
	vec3 entryPoint;
	vec3 exitPoint;
	vec3 entryNormal;
	vec3 exitNormal;
	int id;

	ObjectMaterial material;

	Intersection(): numberOfIntersections(0), entryPoint(0,0,0), exitPoint(0,0,0), entryNormal(0,0,0), exitNormal(0, 0, 0), id(-1), material()
	{}
};

struct Triangle{
	vec3 p1, p2, p3;
	Triangle(vec3 a, vec3 b, vec3 c){
		p1 = a;
		p2 = b;
		p3 = c;
	}
	Triangle(vec3 t[3]){
		p1 = t[0];
		p2 = t[1];
		p3 = t[2];
	}
};

class Shape{
public:
	virtual Intersection getIntersection(Ray ray) = 0;

	int id;
	ObjectMaterial material;

	Shape(): material()
	{}
};

class Triangles: public Shape{
public:
	vector<Triangle> triangles;
	Intersection getIntersection(Ray ray);
	Intersection intersectTriangle(Ray ray, Triangle t);
	void initTriangles(int num, vec3* t, int ID);
};

class Sphere: public Shape{
public:
	vec3 centre;
	float radius;
	Sphere(vec3 c, float r, int ID);
	Intersection getIntersection(Ray ray);
};

class Plane: public Shape{
public:
	vec3 point;
	vec3 normal;
	Plane(vec3 p, vec3 n, int ID);
	Intersection getIntersection(Ray ray);
};

glm::vec3 transmitRay(const glm::vec3& normal, const glm::vec3& rayDirection, const float n);
