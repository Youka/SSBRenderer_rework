/*
Project: SSBRenderer
File: memory.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <deque>
#include <algorithm>

namespace stdex{
	// LFU cache with key-value pairs
	template<typename Key, typename Value, size_t Limit>
	class Cache{
		public:
			// Transparent templates
			using key = Key;
			using value = Value;
			static const decltype(Limit) limit = Limit;
		private:
			// Data storage
			std::deque<std::pair<Key,Value>> memory;
			// Initialization by input
			template<typename Iter>
			void init(Iter first, Iter last){
				decltype(Limit) n = 0;
				while(first != last && n++ != Limit)
					this->memory.push_front(*first++);
			}
			// Find value by key
			typename decltype(memory)::iterator find(const Key& key){
				return std::find_if(
					this->memory.begin(), this->memory.end(),
					[&key](const std::pair<Key,Value>& entry){return key == entry.first;}
				);
			}
		public:
			// Ctors
			Cache() = default;
			Cache(std::initializer_list<std::pair<Key,Value>> init_list){
				this->init(init_list.begin(), init_list.end());
			}
			template<typename Iter>
			Cache(Iter first, Iter last){
				this->init(first, last);
			}
			// Requests
			size_t size() const{
				return this->memory.size();
			}
			bool contains(const Key& key){
				return this->find(key) != this->memory.end();
			}
			std::vector<Key> keys(){
				std::vector<Key> key_vec;
				key_vec.reserve(this->memory.size());
				for(auto& pair : this->memory)
					key_vec.push_back(pair.first);
				return key_vec;
			}
			// Modifications
			Value get(const Key& key){
				auto iter = this->find(key);
				if(iter != this->memory.end()){
					if(iter == this->memory.begin())
						return iter->second;
					auto pre_iter = iter - 1;
					std::swap(*iter, *pre_iter);
					return pre_iter->second;
				}
				return Value();
			}
			void add(const Key& key, const Value& value){
				if(this->memory.size() == Limit)
					this->memory.pop_back();
				this->memory.push_front({key, value});
                        }
			void clear(){
				this->memory.clear();
			}
	};
}
