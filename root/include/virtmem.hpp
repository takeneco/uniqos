/**
 * @file    include/virtmem.hpp
 * @version 0.0.1
 * @date    2009-08-09
 * @author  Kato Takeshi
 * @brief   メモリ管理クラスの宣言。
 */
// (C) Kato Takeshi 2009

#ifndef INCLUDE_VIRTMEM_HPP
#define INCLUDE_VIRTMEM_HPP


#include "mem.hpp"

#define PAGE_SIZE  (1 << PAGE_SIZE_SHIFT)
#define PAGE_MASK  (~(PAGE_SIZE - 1))

class physical_memory
{
public:
	
};

class virtual_memory
{

};


#endif  // INCLUDE_VIRTMEM_HPP
