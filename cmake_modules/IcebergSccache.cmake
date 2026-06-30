# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

if(MSVC_TOOLCHAIN)
  message(STATUS "Using /Z7 debug info for MSVC (avoids PDB lock contention with Ninja)")

  # Remove /Zi or /ZI
  string(REGEX REPLACE "/Z[iI]" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
  string(REGEX REPLACE "/Z[iI]" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

  string(REGEX REPLACE "/Z[iI]" "" CMAKE_C_FLAGS_RELWITHDEBINFO
                       "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
  string(REGEX REPLACE "/Z[iI]" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO
                       "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

  # Add /Z7
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Z7")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Z7")

  set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /Z7")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Z7")
endif()
