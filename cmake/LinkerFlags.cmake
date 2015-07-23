# Adds flags to all linking types
function(add_linker_flags)
	# Iterate through function arguments
	foreach(arg ${ARGV})
		# Iterate through linking flags for executables, modules and shared libraries
		foreach(linker_flags
			CMAKE_EXE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS)
			# Append new flags to current linking flags
			set(${linker_flags} "${${linker_flags}} ${arg}" PARENT_SCOPE)
		endforeach()
	endforeach()
endfunction()