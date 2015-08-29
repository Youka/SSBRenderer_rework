/*
Project: SSBRenderer
File: Renderer_ogl.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "Renderer.hpp"
#include <glfw3.h>
#include <png.h>

#include <thread>
#include <mutex>
#include <unordered_map>

// Manage GL context by thread & references
struct Context{
	std::unique_ptr<GLFWwindow,std::function<void(GLFWwindow*)>> window;
	unsigned ref_count;
};
static std::unordered_map<std::thread::id,Context> contexts;
static std::mutex contexts_mutex;

Renderer::Renderer(){
	// Update contexts for new creation
	{
		std::unique_lock<std::mutex> lock(contexts_mutex);
		auto thread_id = std::this_thread::get_id();
		if(contexts.count(thread_id))
			++contexts[thread_id].ref_count;
		else{
			// Initialize GLFW on first context
			if(contexts.empty())
				glfwInit();
			// Create context
			GLFWwindow* window = glfwCreateWindow(1, 1, "Dummy", NULL, NULL);
			glfwHideWindow(window);
			glfwMakeContextCurrent(window);
			contexts[thread_id] = {
				std::unique_ptr<GLFWwindow,std::function<void(GLFWwindow*)>>(window, [](GLFWwindow* w){glfwMakeContextCurrent(NULL); glfwDestroyWindow(w);}),
				1
			};
		}
	}

	// TODO: create resources

}

Renderer::Renderer(unsigned width, unsigned height) : Renderer(){
	this->set_size(width, height);
}

void Renderer::set_size(unsigned width, unsigned height){

	// TODO

}

Renderer::~Renderer(){
	// Update contexts for one deletion
	{
		std::unique_lock<std::mutex> lock(contexts_mutex);
		auto thread_id = std::this_thread::get_id();
		if(--contexts[thread_id].ref_count == 0){
			// Destroy context
			contexts.erase(thread_id);
			// Terminate GLFW on non-remaining context
			if(contexts.empty())
				glfwTerminate();
		}
	}

	// TODO: delete resources

}

unsigned width(){

	// TODO

	return 0;
}

unsigned height(){

	// TODO

	return 0;
}

void copy_image(unsigned char* image, unsigned padding){

	// TODO

}
