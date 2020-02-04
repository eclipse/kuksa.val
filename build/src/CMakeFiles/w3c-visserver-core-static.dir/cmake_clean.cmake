file(REMOVE_RECURSE
  "libw3c-visserver-core-static.pdb"
  "libw3c-visserver-core-static.a"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/w3c-visserver-core-static.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
