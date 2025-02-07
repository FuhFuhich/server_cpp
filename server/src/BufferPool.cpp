#include "BufferPool.h"

std::shared_ptr<Buffer> BufferPool::get_buffer()
{
	std::lock_guard<std::mutex> lock(mutex_);

	if (!pool_.empty())
	{
		auto buffer = pool_.back();
		pool_.pop_back();
		return buffer;
	}

	return std::make_shared<Buffer>();
}

void BufferPool::release(std::shared_ptr<Buffer> buffer)
{
	std::lock_guard<std::mutex> lock(mutex_);
	
	// Ограничиваем кол-во буфферов 100. shared_ptr сам удалит
	if (pool_.size() < MAX_POOL_SIZE)
	{
		pool_.push_back(buffer);
	}
}
