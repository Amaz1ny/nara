#pragma once
#include <Windows.h>
#include <iostream>
#include <Psapi.h>
#include <TlHelp32.h>
#include "Driver.h"

enum DRV_CTLCODE
{
	CODE_WRITE_MEMORY_Physical = 1,
	CODE_READ_MEMORY_Physical,
	CODE_WRITE_MEMORY,
	CODE_READ_MEMORY,
	CODE_GET_MODULE,
	CODE_AllocUserMemory,
	CODE_FreeUserMemory,
	CODE_CreateUserThread,
	CODE_GET_VERSION
};


class MemoryToolsWrapper {
public:
	MemoryToolsWrapper( Driver* DriverInstance) {
		driver = DriverInstance;
	}
	template<typename T>
	inline auto read(uint64_t Address) -> T {
		T* buffer = nullptr;
		driver->MmRead(&Address, buffer, sizeof(T));
		return *buffer;
	}

	template<typename T>
	inline auto write(uint64_t Address, T Value) -> void {
		return driver->MmWrite(Address, &Value, sizeof(T));
	}

	inline bool write(uint64_t Address, void* Buffer, size_t size) {
		return driver->MmWrite(&Address, Buffer, size);
	}

	inline bool read(uint64_t Address, void* Buffer, size_t size) {
		return driver->MmRead(&Address, Buffer, size);
	}
private:
	Driver* driver;
};

