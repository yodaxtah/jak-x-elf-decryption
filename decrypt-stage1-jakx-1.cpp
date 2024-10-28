#include <iostream>
#include <fstream>
#include <vector>
#include "minilzo.h"

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
    const size_t compressed_offset = 0x166ab4 - 0x100000;
    const size_t compressed_size = 0x88f9f;
    const size_t expected_decompressed_size = 0x170CA4;
    
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
    std::vector<unsigned char> decompressed_data(expected_decompressed_size); // As per the decompiled code
    lzo_uint decompressed_size = 0;

    // Decompress
    int r = lzo1x_decompress(compressed_data.data(), compressed_data.size(),
                             decompressed_data.data(), &decompressed_size, nullptr);

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

    // Write the decompressed part
    output.write(reinterpret_cast<const char*>(decompressed_data.data()), decompressed_size);

    output.close();

    std::cout << "Decompression completed successfully" << std::endl;
    return 0;
}
