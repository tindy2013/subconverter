#include "../handler/settings.h"
#include <string>

Settings global;

bool fileExist(const std::string&, bool) { return false; }
std::string fileGet(const std::string&, bool) { return ""; }
