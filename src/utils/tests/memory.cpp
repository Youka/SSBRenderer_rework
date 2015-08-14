/*
Project: SSBRenderer
File: memory.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "../memory.hpp"
#include <stdexcept>

int main(){
	stdex::Cache<char, int, 3> cache{{'A', 1}, {'B', 2}, {'C', 3}, {'D', 4}};
	if(cache.size() != decltype(cache)::limit)
		throw std::logic_error("Current cache size & max size don't fit");
	cache.add('E', 5);
	if(cache.contains('A'))
		throw std::logic_error("Found outdated value in cache");
	if(cache.get('B') != 2)
		throw std::logic_error("Invalid cache entry");
	return 0;
}
