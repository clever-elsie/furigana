#pragma once
#include <json.hpp>

json::value call_curl(const config_t&config, const json::value&req);