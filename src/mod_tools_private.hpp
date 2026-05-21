/*-------------------------------------------------------------------------------

BARONY
File: mod_tools_private.hpp
Desc: Internal header for mod_tools implementation files — provides common
      includes and the MOD_TOOLS_CPP define needed by mod_tools.hpp.

Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "cJSON.h"

#define MOD_TOOLS_CPP
#include "items.hpp"
#include "mod_tools.hpp"
#include "menu.h"
#include "classdescriptions.h"
#include "draw.hpp"
#include "player.hpp"
#include "scores.hpp"
#include "ui/Field.hpp"
#include "ui/Image.hpp"

#include "ui/MainMenu.hpp"
#include "shops.hpp"
#include "interface/ui.hpp"
#include "ui/GameUI.hpp"

#include "init.h"
#include "ui/LoadingScreen.hpp"

#include <thread>
#include <future>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
