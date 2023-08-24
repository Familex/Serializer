cmake_minimum_required (VERSION 3.8)

function (make_file_includable_as_raw_string_literal input_file output_file)
    file (READ ${input_file} content)
    set (test_configs_delim "====")
    set (content "R\"${test_configs_delim}(\n${content}\n)${test_configs_delim}\"")
    file (WRITE ${output_file} ${content})
endfunction (make_file_includable_as_raw_string_literal)

function (process_configs)
    file (GLOB_RECURSE test_configs "**/configs/*yaml")
    foreach (file ${test_configs})
        set (output_file "${file}.raw")
        make_file_includable_as_raw_string_literal (${file} ${output_file})
    endforeach ()
endfunction (process_configs)

process_configs ()
