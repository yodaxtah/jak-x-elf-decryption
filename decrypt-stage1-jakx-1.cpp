#include <iostream>
#include <fstream>
#include <vector>
#include "minilzo.h"

#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);

int main() {
    // Initialize LZO
    if (lzo_init() != LZO_E_OK) {
        std::cerr << "LZO initialization failed" << std::endl;
        return 1;
    }

    // Read the entire ELF file
    std::ifstream input{ "./game-elf", std::ios::binary };
    if (!input) {
        std::cerr << "Failed to open input file" << std::endl;
        return 1;
    }

    std::vector<unsigned char> elf_data((std::istreambuf_iterator<char>(input)),
                                        std::istreambuf_iterator<char>());
    input.close();

    // Extract the compressed portion
    const size_t compressed_offset = 0x16bbc4 - 0x100000;
    const size_t compressed_size = 0x89002;
    
    if (elf_data.size() < compressed_offset + compressed_size) {
        std::cerr << "ELF file is smaller than expected" << std::endl;
        std::cout
            << "elf_data: " << elf_data.size() << std::endl
            << "  compressed_offset: " << compressed_offset << std::endl
            << "  compressed_size: " << compressed_size << std::endl
            << "  compressed_offset + compressed_size: " << compressed_offset + compressed_size << std::endl
            ;
        return 1;
    }

    std::vector<unsigned char> compressed_data(elf_data.begin() + compressed_offset,
                                               elf_data.begin() + compressed_offset + compressed_size);

    // Prepare buffer for decompressed data
    std::vector<unsigned char> decompressed_data(0x800000); // As per the decompiled code
    lzo_uint decompressed_size = decompressed_data.size();

    // Decompress
    int r = lzo1x_decompress(compressed_data.data(), compressed_data.size(),
                             decompressed_data.data(), &decompressed_size, wrkmem);

    if (r != LZO_E_OK) {
        std::cerr << "Decompression failed" << std::endl;
        return 1;
    }

    // Resize the decompressed data to its actual size
    decompressed_data.resize(decompressed_size);

    // Write the decompressed ELF
    std::ofstream output("./decrypted-elf", std::ios::binary);
    if (!output) {
        std::cerr << "Failed to open output file" << std::endl;
        return 1;
    }

    // Write the uncompressed part of the ELF
    output.write(reinterpret_cast<const char*>(elf_data.data()), compressed_offset);

    // Write the decompressed part
    output.write(reinterpret_cast<const char*>(decompressed_data.data()), decompressed_size);

    // Write the remaining part of the original ELF, if any
    if (elf_data.size() > compressed_offset + compressed_size) {
        output.write(reinterpret_cast<const char*>(elf_data.data() + compressed_offset + compressed_size),
                     elf_data.size() - (compressed_offset + compressed_size));
    }

    output.close();

    std::cout << "Decompression completed successfully" << std::endl;
    return 0;
}
