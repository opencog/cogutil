set(summary_willbuild "")
set(summary_willbuild_d "")
set(summary_willnotbuild "")
set(summary_willnotbuild_d "")
set(max_name_length "0")

file(READ "/etc/os-release" _OSR)
string(REGEX MATCH "PRETTY_NAME=\"([a-xA-Z0-9 \\.,:;!@%#/()]*)" _BARF ${_OSR})
set(OS_RELEASE ${CMAKE_MATCH_1})

macro(summary_add name description test)
  string(LENGTH ${name} namelength)
  if (${namelength} GREATER ${max_name_length})
      set (max_name_length ${namelength})
  endif (${namelength} GREATER ${max_name_length})
  if (${test} ${ARGN})
      list(APPEND summary_willbuild "${name}")
      list(APPEND summary_willbuild_d "${description}")
  else (${test} ${ARGN})
      list(APPEND summary_willnotbuild "${name}")
      list(APPEND summary_willnotbuild_d "${description}")
  endif (${test} ${ARGN})
endmacro(summary_add)

macro(summary_show_part variable descriptions title)
  list(LENGTH ${variable} _len)
  if (_len)
    message("")
    message(${title})
    message("-----------------------------------------------")
    math(EXPR _max_range "${_len} - 1")
    foreach (_index RANGE 0 ${_max_range})
      list(GET ${variable} ${_index} _item)
      list(GET ${descriptions} ${_index} _description)
      string(LENGTH ${_item} _item_len)
      set(indent_str "")
      math(EXPR indent_size "${max_name_length} - ${_item_len}")
      foreach(loopy RANGE ${indent_size})
          set(indent_str "${indent_str} ")
      endforeach(loopy)
      message("   ${_item} ${indent_str}- ${_description}.")
    endforeach (_index)
  endif (_len)
endmacro(summary_show_part)

macro(summary_show)
  message("")
  message("Building for ${OS_RELEASE}")
  summary_show_part(summary_willbuild summary_willbuild_d
      "The following components will be built:")
  summary_show_part(summary_willnotbuild summary_willnotbuild_d
      "The following components WILL NOT be built:")
  message("")
endmacro(summary_show)
