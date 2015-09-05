/*
Project: SSBRenderer
File: Renderer.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

// RGBA renderer on CPU or GPU (holding own image)
class Renderer{
	private:
		// Resources by implementation
		void* data;
	public:
		// Setters
		Renderer();
		Renderer(unsigned width, unsigned height);
		void set_size(unsigned width, unsigned height);
		// Free resources
		~Renderer();
		// No copy&move (-> resources limitation)
		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) = delete;
		// Getter
		unsigned width();
		unsigned height();
		void copy_image(unsigned char* image, unsigned padding);
		// Processing
		void clear_stencil();

		// TODO

};
