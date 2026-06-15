#pragma once

#include "trigger_cfg.h"

#include "httplib.h"
#include "modules.hpp"


class Module;
struct RuleServer {
    static shared_ptr<RuleList> rules();
    static list<Module> mods;  
    static void handle_rules(const httplib::Request& req, httplib::Response& res) noexcept;
    static constexpr auto rule_URI = "/rule"s;

    static void handle_mods(const httplib::Request& req, httplib::Response& res) noexcept;
    static constexpr auto mods_URI = "/mods"s;

    static void handle_switch(const httplib::Request& req, httplib::Response& res) noexcept;
    static constexpr auto switch_URI = "/switch"s;
    static void testInit();
};