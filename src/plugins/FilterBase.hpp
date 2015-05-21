/*
Project: SSBRenderer
File: FilterBase.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include <vector>

// Cross interface filter functions
namespace FilterBase{
	// Meta informations
	const char* get_name();
	const char* get_description();
	enum class ArgType{BOOL, BOOL_OPT, INTEGER, INTEGER_OPT, FLOAT, FLOAT_OPT, STRING, STRING_OPT};
	std::vector<ArgType> get_opt_args();
	// Process
	struct VideoInfo{
		int width, height;
		bool has_alpha;
		double fps;
		long frames;
	};
	struct Variant{
		ArgType type;
		union{
			bool b;
			int i;
			float f;
			const char* s;
		};
	};
	void init(VideoInfo vinfo, std::vector<Variant> args, void**) throw (const char*);
}
