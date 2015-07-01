# Adds flags to all linking types
function(add_linker_flags new_flags)
	# Iterate through linking flags for executables, modules, shared and static libraries
	foreach(linker_flags
		CMAKE_EXE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
		# Append new flags to current linking flags
		set(${linker_flags} "${${linker_flags}} ${new_flags}" PARENT_SCOPE)
	endforeach()
endfunction()