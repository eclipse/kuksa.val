file(REMOVE_RECURSE
  "libw3c-visserver-core.pdb"
  "libw3c-visserver-core.so"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/w3c-visserver-core.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
