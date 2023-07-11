#pragma once

#include <memory>
#include <type_traits>
#include <scoped_allocator>
#include <string>
#include <unordered_map>
#include <map>
#include <queue>
#include <set>

#include <assert.h>

#include "Allocators/BaseAlloc.h"


template <typename T>
struct CustomAlloc
{
	template <typename U> friend struct CustomAlloc;

	using value_type = T;
	using pointer = T*;

	using propagate_on_container_copy_assignment = std::true_type;
	using propagate_on_container_move_assignment = std::true_type;
	using propagate_on_container_swap = std::true_type;
	
	// I don't know how to get rid of this, as removing it will fail to build, so for now asserts will do to enforce using the other constructor
	CustomAlloc() = default; 

	// Use this one 
	explicit CustomAlloc(BaseAlloc* a) : m_BaseAlloc(a) {}

	template <typename U>
	CustomAlloc(CustomAlloc<U> const& rhs) : m_BaseAlloc(rhs.m_BaseAlloc) {}


	pointer allocate(std::size_t n)
	{
		// Check if you're supplying the right arguments in the container's constructor
		assert(m_BaseAlloc && "m_BaseAlloc is null!"); 
		return static_cast<pointer>(m_BaseAlloc->Allocate(n * sizeof(T), alignof(T)));
	}

	void deallocate(pointer p, std::size_t n)
	{
		// Check if you're supplying the right arguments in the container's constructor
		assert(m_BaseAlloc && "m_BaseAlloc is null!");
		m_BaseAlloc->Free(p);
	}

	template <typename U>
	bool operator==(CustomAlloc<U> const & rhs) const
	{
		return m_BaseAlloc == rhs.m_BaseAlloc;
	}

	template <typename U>
	bool operator!=(CustomAlloc<U> const & rhs) const
	{
		return m_BaseAlloc != rhs.m_BaseAlloc;
	}

private:
	BaseAlloc* m_BaseAlloc = nullptr;
};


template <typename T>
using ScopedCustomAlloc = std::scoped_allocator_adaptor<CustomAlloc<T>>;

template<class T, class Alloc = ScopedCustomAlloc<T>>
using ve_vector = std::vector<T, Alloc>;

using ve_string = std::basic_string<char, std::char_traits<char>, ScopedCustomAlloc<char>>;

template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>, class Alloc = ScopedCustomAlloc<std::pair<Key, T>>>
using ve_unordered_map = std::unordered_map<Key, T, Hash, KeyEqual, Alloc>;

template<class Key, class T, class Compare = std::less<Key>, class Alloc = ScopedCustomAlloc<std::pair<Key, T>>>
using ve_map = std::map<Key, T, Compare, Alloc>;

template<class T, class Alloc = ScopedCustomAlloc<T>>
using ve_deque = std::deque<T, Alloc>;

template<class T, class Container = ve_deque<T>>
using ve_queue = std::queue<T, Container>;

template<class T, class Compare = std::less<T>, class Alloc = ScopedCustomAlloc<T>>
using ve_set = std::set<T, Compare, Alloc>;

template<class T, class Container = std::vector<T>, class Compare = std::less<typename Container::value_type>>
using ve_priority_queue = std::priority_queue<T, Container, Compare>;



