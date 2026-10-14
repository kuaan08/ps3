#include <Memory.hpp>
#include <logger.hpp>
#include <cstdio>
#include <stdexcept>

static_assert(true, "A logging header must never redefine true as false.");

static void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}

int main() {
    try {
        require(SPU_MEM_END == SPU_MEM_START + SPU_MEM_SIZE, "SPU region bounds are wrong");
        bool panic_ok = false;
        try {
            Helpers::panic("PS3 test error %d", 42);
        } catch (const std::runtime_error& e) {
            panic_ok = std::string(e.what()) == "PS3 test error 42";
        }
        require(panic_ok, "Error formatter returned padded or invalid text");

        // Use the emulator's real memory implementation, not a mock.
        Memory mem;
        const auto* entry = mem.alloc(2 * PAGE_SIZE);
        const u64 vaddr = entry->vaddr;
        const u64 page = vaddr >> PAGE_SHIFT;

        require(mem.isMapped(vaddr).first, "Fresh allocation not mapped");
        require(mem.read_table[page] != nullptr, "Read fastmem not active");
        require(mem.write_table[page] != nullptr, "Write fastmem not active");

        mem.write<u32>(vaddr + 4, 0x1234ABCD);
        require(mem.read<u32>(vaddr + 4) == 0x1234ABCD, "Guest memory round-trip failed");
        mem.write<u64>(vaddr + PAGE_SIZE + 8, 0xABCDEF1234567890ULL);
        require(mem.read<u64>(vaddr + PAGE_SIZE + 8) == 0xABCDEF1234567890ULL, "Second guest page failed");

        mem.free(const_cast<MemoryRegion::MapEntry*>(entry));
        require(!mem.isMapped(vaddr).first, "Freed allocation remains mapped");
        require(mem.read_table[page] == nullptr, "Free left a stale read-page pointer");
        require(mem.write_table[page] == nullptr, "Free left a stale write-page pointer");
        require(mem.read_table[page + 1] == nullptr, "Second page stale after free");

        auto* entry2 = mem.alloc(PAGE_SIZE);
        require(entry2->vaddr == vaddr, "Freed virtual memory was not reusable");
        const u64 mapped_vaddr = entry2->vaddr;
        mem.unmap(mapped_vaddr);
        require(mem.read_table[page] == nullptr, "Unmap left stale fastmem cache");
        require(!mem.isMapped(mapped_vaddr).first, "Unmap failed");

        // MemoryRegion allocates with new[]: mirror PlayStation3's destructor.
        for (auto* region : mem.regions)
            delete[] region->mem;
        puts("PASS: guest read/write, free, remap, unmap and cache invalidation");
        return 0;
    } catch (const std::exception& e) {
        fprintf(stderr, "FAILED: %s\n", e.what());
        return 1;
    }
}
