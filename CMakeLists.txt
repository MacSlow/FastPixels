################################################################################
#23456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
#
#  FastPixels - a experiment into hand-tuned x86-assembly
# 
#  Copyright 2016-2017 Mirco Müller
# 
#  Author(s):
#    Mirco "MacSlow" Müller <macslow@gmail.com>
# 
#  This program is free software: you can redistribute it and/or modify it
#  under the terms of the GNU General Public License version 3, as published
#  by the Free Software Foundation.
# 
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranties of
#  MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
#  PURPOSE.  See the GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License along
#  with this program.  If not, see <http://www.gnu.org/licenses/>.
# 
################################################################################

cmake_minimum_required (VERSION 2.8.11)
project (FastPixels)

enable_language (ASM_NASM)
add_library (AsmLib asmfuncs.nasm asmfuncsSSSE3.nasm)

set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTOUIC ON)
set (CMAKE_INCLUDE_CURRENT_DIR ON)

find_package (Qt5Core)
find_package (Qt5Gui)
find_package (Qt5Widgets)

add_compile_options (-std=c++14 -Wall -Werror -pedantic -Ofast -march=native)

add_definitions (${qt_definitions})
include_directories (${Qt5Core_INCLUDES})
include_directories (${Qt5Qml_INCLUDES})
include_directories (./)

set (FastPixels_sources main.cpp mainwindow.cpp)

add_executable (FastPixels ${FastPixels_sources})
qt5_use_modules (FastPixels Core Widgets Gui)
target_link_libraries (FastPixels AsmLib -pthread ${qt_libraries})

add_custom_target (perf
	DEPENDS FastPixels
	COMMAND perf stat ./FastPixels
)

add_custom_target (report
	DEPENDS FastPixels
	COMMAND perf record -g ./FastPixels
	COMMAND perf report -g 'graph,0.5,caller' --sort comm,dso,sym
)
