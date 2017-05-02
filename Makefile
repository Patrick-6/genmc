# RCMC -- Model Checking for C11 programs.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you can access it online at
# http://www.gnu.org/licenses/gpl-2.0.html.
#
# Author: Michalis Kokologiannakis <mixaskok@gmail.com>

.PHONY: clean depend

LINK_FLAGS=`llvm-config --ldflags --libs --system-libs`
GXX_FLAGS=`llvm-config --cxxflags` -std=c++14 -g
GXX=clang++

TARGET=rcmc
FILES=$(addsuffix .o, Config Parser Error LLVMModule Interpreter Execution \
	ExternalFunctions DeclareAssumePass SpinAssumePass LoopUnrollPass \
	ExecutionGraph Thread Event Driver main)

default: $(TARGET)

%.o: %.cpp %.hpp
	$(GXX) -c $< -Wno-unknown-warning-option $(GXX_FLAGS)

%.o: %.cpp
	$(GXX) -c $< -Wno-unknown-warning-option $(GXX_FLAGS)

$(TARGET): $(FILES)
	$(GXX) -o $(TARGET) $^ $(LINK_FLAGS)

clean:
	@$(RM) *.o

distclean: clean
	@$(RM) $(TARGET)
