function(BinaryToHeader input_file output_file target_name)
    file(READ ${input_file} file_data HEX)
    string(LENGTH "${file_data}" file_size)
    math(EXPR file_size "${file_size} / 2") # because each byte is represented by two hex characters
    string(REGEX REPLACE "(..)" "0x\\1, " hex_data ${file_data})
    file(WRITE ${output_file} "constexpr unsigned char _${target_name}Data[] = { ${hex_data} };constexpr size_t _${target_name}Size = sizeof(_NeuralNetworkBinaryData);")
endfunction()