# cmake/modules/cgm.cmake
#
# Copyright (C) 2006  Alan W. Irwin
#
# This file is part of PLplot.
#
# PLplot is free software; you can redistribute it and/or modify
# it under the terms of the GNU Library General Public License as published
# by the Free Software Foundation; version 2 of the License.
#
# PLplot is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with the file PLplot; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA

# Configuration for the cgm device driver (supporting the cgm device).
#
# The following variables are set / modified
#
# cgm_TARGETS	     - list of targets which the cgm dynamic device
# 		       depends on.
# cgm_COMPILE_FLAGS	  - individual COMPILE_FLAGS required to compile cgm
# 			    device.
# DRIVERS_LINK_FLAGS - list of targets which the cgm static device
# 		       depends on.

if(PLD_cgm)
  set(cgm_TARGETS nistcd)
  set(cgm_COMPILE_FLAGS "-I${CMAKE_SOURCE_DIR}/lib/nistcd")
  
  set(DRIVERS_LINK_FLAGS ${DRIVERS_LINK_FLAGS} nistcd)
endif(PLD_cgm)
