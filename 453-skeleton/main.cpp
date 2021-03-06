#define _USE_MATH_DEFINES
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#include <glm/gtx/vector_query.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "Geometry.h"
#include "GLDebug.h"
#include "Log.h"
#include "ShaderProgram.h"
#include "Shader.h"
#include "Texture.h"
#include "Window.h"
#include "imagebuffer.h"
#include "RayTrace.h"
#include "Scene.h"
#include "Lighting.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"


int hasIntersection(Scene const &scene, Ray ray, int skipID){
	for (auto &shape : scene.shapesInScene) {
		Intersection tmp = shape->getIntersection(ray);
		if(
			shape->id != skipID
			&& tmp.numberOfIntersections!=0
			&& glm::distance(tmp.entryPoint, ray.origin) > 0.00001
			&& glm::distance(tmp.entryPoint, ray.origin) < glm::distance(ray.origin, scene.lightPosition) - 0.01
		){
			return tmp.id;
		}
	}
	return -1;
}

Intersection getClosestIntersection(Scene const &scene, Ray ray, int skipID){ //get the nearest
	Intersection closestIntersection;
	float min = std::numeric_limits<float>::max();
	for(auto &shape : scene.shapesInScene) {
		if(skipID == shape->id) {
			// Sometimes you need to skip certain shapes. Useful to
			// avoid self-intersection. ;)
			continue;
		}
		Intersection p = shape->getIntersection(ray);
		float distance = glm::distance(p.entryPoint, ray.origin);
		if(p.numberOfIntersections !=0 && distance < min){
			min = distance;
			closestIntersection = p;
		}
	}
	return closestIntersection;
}

glm::vec3 raytraceSingleRay(Scene const &scene, Ray const &ray, int depth, int source_id) {
	// TODO: Part 3: Somewhere in this function you will need to add the code to determine
	//               if a given point is in shadow or not. Think carefully about what parts
	//               of the lighting equation should be used when a point is in shadow.
	// TODO: Part 4: Somewhere in this function you will need to add the code that does reflections.
	//               NOTE: The ObjectMaterial class already has a parameter to store the object's
	//               reflective properties. Use this parameter + the color coming back from the
	//               reflected array and the color from the phong shading equation.
	Intersection result = getClosestIntersection(scene, ray, source_id); //find intersection

	PhongReflection phong;
	phong.ray = ray;
	phong.scene = scene;
	phong.material = result.material;
	phong.intersection = result;

	glm::vec3 reflectColor(0);
	glm::vec3 transmitColor(0);
	if (depth < 1 || result.numberOfIntersections == 0)
	{
		glm::vec3(0, 0, 0);
	}
	else //intersects an object
	{
		Ray shadowRay(result.entryPoint, glm::normalize(scene.lightPosition - result.entryPoint));
		if (hasIntersection(scene, shadowRay, result.id) != -1)
		{
			phong.material.diffuse = glm::vec3(0);
			phong.material.specular = glm::vec3(0);
		}
		else
		{
			if (phong.material.reflectionStrength != vec3(0))
			{
				glm::vec3 reflectedDir = glm::normalize(2 * glm::dot(result.entryNormal, -ray.direction) * result.entryNormal + ray.direction);
				Ray reflectedRay(result.entryPoint, reflectedDir);
				reflectColor = phong.material.reflectionStrength * raytraceSingleRay(scene, reflectedRay, depth - 1, result.id);
			}

			if (phong.material.indexOfRefraction != 0)
			{
				glm::vec3 transmitDir1 = glm::normalize(result.exitPoint - result.entryPoint);
				glm::vec3 transmitDir2 = transmitRay(result.exitNormal, transmitDir1, result.material.indexOfRefraction);
				Ray refractRay(result.exitPoint, transmitDir2);
				transmitColor = raytraceSingleRay(scene, refractRay, depth - 1, result.id);
			}
		}
	}

	return phong.I() + reflectColor + transmitColor;
}

struct RayAndPixel {
	Ray ray;
	int x;
	int y;
};

std::vector<RayAndPixel> getRaysForViewpoint(Scene const &scene, ImageBuffer &image, glm::vec3 viewPoint) {
	// This is the function you must implement for part 1
	//
	// This function is responsible for creating the rays that go
	// from the viewpoint out into the scene with the appropriate direction
	// and angles to produce a perspective image.
	int x = 0;
	int y = 0;
	std::vector<RayAndPixel> rays;

	const glm::vec3 origin(0, 0, 0);
	for (float i = -0.5; x < image.Width(); x++) {
		y = 0;
		for (float j = -0.5; y < image.Height(); y++) {
			glm::vec3 direction = glm::normalize(glm::vec3{ i - viewPoint.x, j-viewPoint.y, -1.0 });
			Ray r = Ray(origin, direction);
			rays.push_back({r, x, y});
			j += 1.f / image.Height();
		}
		i += 1.f / image.Width();
	}
	return rays;
}

void raytraceImage(Scene const &scene, ImageBuffer &image, glm::vec3 viewPoint) {
	// Reset the image to the current size of the screen.
	image.Initialize();

	// Get the set of rays to cast for this given image / viewpoint
	std::vector<RayAndPixel> rays = getRaysForViewpoint(scene, image, viewPoint);


	// This loops processes each ray and stores the resulting pixel in the image.
	// final color into the image at the appropriate location.
	//
	// I've written it this way, because if you're keen on this, you can
	// try and parallelize this loop to ensure that your ray tracer makes use
	// of all of your CPU cores
	//
	// Note, if you do this, you will need to be careful about how you render
	// things below too
	// std::for_each(std::begin(rays), std::end(rays), [&] (auto const &r) {
	for (auto const & r : rays) {
		glm::vec3 color = raytraceSingleRay(scene, r.ray, 10, -1);
		image.SetPixel(r.x, r.y, color);
	}
}

// EXAMPLE CALLBACKS
class Assignment5 : public CallbackInterface {

public:
	Assignment5() {
		viewPoint = glm::vec3(0, 0, 0);
		scene = initScene1();
		raytraceImage(scene, outputImage, viewPoint);
	}

	virtual void keyCallback(int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
			shouldQuit = true;
		}

		if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
			scene = initScene1();
			sceneNum = 1;
			refractionToggle = false;
			raytraceImage(scene, outputImage, viewPoint);
		}

		if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
			scene = initScene2();
			sceneNum = 2;
			refractionToggle = false;
			raytraceImage(scene, outputImage, viewPoint);
		}

		if (key == GLFW_KEY_T && action == GLFW_PRESS)
		{
			refractionToggle = !refractionToggle;
			if (sceneNum == 1)
			{
				scene = initScene1(refractionToggle);
			}
			else
			{
				scene = initScene2(refractionToggle);
			}
			raytraceImage(scene, outputImage, viewPoint);
		}
	}
	bool shouldQuit = false;
	int sceneNum = 1;
	bool refractionToggle = false;
	ImageBuffer outputImage;
	Scene scene;
	glm::vec3 viewPoint;

};
// END EXAMPLES


int main() {
	Log::debug("Starting main");

	// WINDOW
	glfwInit();

	// Change your image/screensize here.
	int width = 800;
	int height = 800;
	Window window(width, height, "CPSC 453");

	GLDebug::enable();

	// CALLBACKS
	std::shared_ptr<Assignment5> a5 = std::make_shared<Assignment5>(); // can also update callbacks to new ones
	window.setCallbacks(a5); // can also update callbacks to new ones

	// RENDER LOOP
	while (!window.shouldClose() && !a5->shouldQuit) {
		glfwPollEvents();

		glEnable(GL_FRAMEBUFFER_SRGB);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		a5->outputImage.Render();

		window.swapBuffers();
	}


	// Save image to file:
	// outpuImage.SaveToFile("foo.png")

	glfwTerminate();
	return 0;
}
