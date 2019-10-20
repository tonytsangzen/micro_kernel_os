#include <mm/trunkmalloc.h>
#include <mm/mmu.h>

/*
malloc for memory trunk management
*/

static mem_block_t* gen_block(char* p, uint32_t size) {
	uint32_t block_size = sizeof(mem_block_t);
	mem_block_t* block = (mem_block_t*)p;
	block->next = block->prev = 0;
	block->mem = p + block_size;
	block->size = size - block_size;
	return block;
}

/*if block size much bigger than the size required, break to two blocks*/
static void try_break(malloc_t* m, mem_block_t* block, uint32_t size) {
	uint32_t block_size = sizeof(mem_block_t);
	//required more than half size of block. no break.
	if((block_size+size) > (uint32_t)(block->size/2)) 
		return;
	
	//do break;
	size = ALIGN_UP(size, 4);
	char* p = block->mem + size;
	mem_block_t* newBlock = gen_block(p, block->size - size);
	newBlock->used = 0; //break a new free block.

	block->size = size;
	newBlock->next = block->next;
	if(newBlock->next != 0)
		newBlock->next->prev = newBlock;

	newBlock->prev = block;
	block->next = newBlock;

	if(m->tail == block) 
		m->tail = newBlock;
}

char* trunk_malloc(malloc_t* m, uint32_t size) {
	mem_block_t* block = m->head;
	while(block != 0) {
		if(block->used || block->size < size) {
			block = block->next;
		}
		else {
			block->used = 1;
			try_break(m, block, size);
			break;
		}
	}
	if(block != 0) {
		return block->mem;
	}

	/*Can't find any available block, expand pages*/
	uint32_t block_size = sizeof(mem_block_t);
	uint32_t expand_size = size + block_size;

	uint32_t pages = expand_size / PAGE_SIZE;	
	if((expand_size % PAGE_SIZE) > 0)
		pages++;

	char* p = (char*)m->get_mem_tail(m->arg);
	if(m->expand(m->arg, pages) != 0)
		return 0;

	block = gen_block(p, pages*PAGE_SIZE);
	block->used = 1;

	if(m->head == 0) {
		m->head = block;
	}

	if(m->tail == 0) {
		m->tail = block;
	}
	else {
		m->tail->next = block;
		block->prev = m->tail;
		m->tail = block;
	}

	try_break(m, block, size);
	return block->mem;
}

/*
try to merge around free blocks.
*/
static void try_merge(malloc_t* m, mem_block_t* block) {
	uint32_t block_size = sizeof(mem_block_t);
	//try next block	
	mem_block_t* b = block->next;
	if(b != 0 && b->used == 0) {
		block->size += (b->size + block_size);
		block->next = b->next;
		if(block->next != 0) 
			block->next->prev = block;
		else
			m->tail = block;
	}

	//try left block	
	b = block->prev;
	if(b != 0&& b->used == 0) {
		b->size += (block->size + block_size);
		b->next = block->next;
		if(b->next != 0) 
			b->next->prev = b;
		else
			m->tail = b;
	}
}

/*
try to shrink the pages.
*/
static void try_shrink(malloc_t* m) {
	uint32_t block_size = sizeof(mem_block_t);
	uint32_t addr = (uint32_t)m->tail;
	//check if page aligned.	
	if(m->tail == 0 || m->tail->used == 1 || (addr % PAGE_SIZE) != 0)
		return;

	int pages = (m->tail->size+block_size) / PAGE_SIZE;
	m->tail = m->tail->prev;
	if(m->tail != 0)
		m->tail->next = 0;
	//else
	//	m->tail = m->head;
	else
		m->head = 0; 
	m->shrink(m->arg, pages);
}

void trunk_free(malloc_t* m, char* p) {
	uint32_t block_size = sizeof(mem_block_t);
	if(((uint32_t)p) < block_size) //wrong address.
		return;
	mem_block_t* block = (mem_block_t*)(p-block_size);
	block->used = 0; //mark as free.
	try_merge(m, block);
	if(m->shrink != 0)
		try_shrink(m);
}
