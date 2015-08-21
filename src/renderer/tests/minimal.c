/*
Project: SSBRenderer
File: minimal.c

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "../public.h"
#include <string.h>
#include <stdio.h>

int main(){
	// Create RGB image
	static const int width = 320, height = 240;
	unsigned char image[height*width*3];
	memset(image, 0, sizeof(image));
	// Script content
	const char* data =
	"#EVENTS\n"
	"500-2.0|||{an=7;pos=0,0;gm=p;lw=0;cl=ffffff}m 0 0 l 3 0 3 1 0 0.8";
	// Create renderer
	char warning[SSB_WARNING_LENGTH];
	ssb_renderer renderer = ssb_create_renderer_from_memory(width, height, SSB_BGR, data, warning);
	if(!renderer){
		puts(warning);
		return 1;
	}
	// Render on image
	ssb_render(renderer, image, width*3, 1000);
	// Destroy renderer
	ssb_free_renderer(renderer);
	// Check rendered pixels
	if(image[0] == 0){
		puts("Rendering failed (no results visible)");
		return 2;
	}
	// All correct
	return 0;
}
