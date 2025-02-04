#pragma once

#include <array>
#include <vector>
#include <mutex>
#include <memory>
#include <optional>

constexpr std::size_t BUFFER_SIZE = 1024;

using Buffer = std::array<char, BUFFER_SIZE>;

class BufferPool
{
private:
	std::vector<std::shared_ptr<Buffer>> pool_;
	std::mutex mutex_;

public:
	std::shared_ptr<Buffer> get_buffer();
	void release(std::shared_ptr<Buffer>);
};