# Copyright (c) 2014 Jean Guyomarc'h
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

set(INSTALL_MANIFEST "${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt")

if (NOT EXISTS ${INSTALL_MANIFEST})
   message(FATAL_ERROR "Cannot find install manifest: ${INSTALL_MANIFEST}")
endif()

file (STRINGS ${INSTALL_MANIFEST} files)
foreach (file ${files})
   if (EXISTS ${file})
      message (STATUS "Removing file: '${file}'")
      exec_program (
         ${CMAKE_COMMAND} ARGS "-E remove ${file}"
         OUTPUT_VARIABLE stdout
         RETURN_VALUE result
         )
      if (NOT "${result}" STREQUAL 0)
         message (FATAL_ERROR "Failed to remove ${file}")
      endif()
   else()
      message (STATUS "File ${file} does not exist!")
   endif()

endforeach(file)
