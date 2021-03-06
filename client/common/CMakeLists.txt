# FreeRDP: A Remote Desktop Protocol Implementation
# FreeRDP Client Common
#
# Copyright 2012 Marc-Andre Moreau <marcandre.moreau@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(MODULE_NAME "freerdp-client")
set(MODULE_PREFIX "FREERDP_CLIENT")

set(${MODULE_PREFIX}_SRCS
	client.c
	file.c)

set(FREERDP_CHANNELS_CLIENT_PATH "../../channels/client")
foreach(FREERDP_CHANNELS_CLIENT_SRC ${FREERDP_CHANNELS_CLIENT_SRCS})
	set(${MODULE_PREFIX}_SRCS ${${MODULE_PREFIX}_SRCS} "${FREERDP_CHANNELS_CLIENT_PATH}/${FREERDP_CHANNELS_CLIENT_SRC}")
endforeach()

if(MSVC)
	set(${MODULE_PREFIX}_SRCS ${${MODULE_PREFIX}_SRCS} module.def)
endif()

add_library(${MODULE_NAME} ${${MODULE_PREFIX}_SRCS})

set_target_properties(${MODULE_NAME} PROPERTIES VERSION ${FREERDP_VERSION_FULL} SOVERSION ${FREERDP_VERSION} PREFIX "lib")

set(${MODULE_PREFIX}_LIBS ${${MODULE_PREFIX}_LIBS}
	${FREERDP_CHANNELS_CLIENT_LIBS})

target_link_libraries(${MODULE_NAME} ${${MODULE_PREFIX}_LIBS})
set_target_properties(${MODULE_NAME} PROPERTIES LINK_INTERFACE_LIBRARIES "")

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries)

set_property(TARGET ${MODULE_NAME} PROPERTY FOLDER "Client/Common")
