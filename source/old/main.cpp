#include "httplib.h"
#include "json.hpp"
#include <exception>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "trigger_cfg.h"
#include "modules.hpp"
#include "rule_srv.h"

using namespace std;
using namespace std::string_literals;
using json = nlohmann::json;

void test_handler(const httplib::Request& req, httplib::Response& res){
    json ret = {{"message", "It is test url"}};
    res.set_content(ret.dump(), "text/json");
}

// rule map based
// atomic<bool> started = true;
// int main(){
//     httplib::Server srv;
//     srv.Get("/test"s, test_handler);
//     constexpr auto port = 8080;
//     constexpr auto addr = "localhost";
//     cout << "Server listen on http://" << addr << ":" << port << endl;
//     {
//         RuleServer::testInit();
//         srv.Get(RuleServer::rule_URI, RuleServer::handle_rules);
//         srv.Post(RuleServer::rule_URI, RuleServer::handle_rules);
//         srv.Patch(RuleServer::rule_URI, RuleServer::handle_rules);

//         srv.Get(RuleServer::mods_URI, RuleServer::handle_mods);
//         srv.Post(RuleServer::mods_URI, RuleServer::handle_mods);

//         srv.Get(RuleServer::switch_URI, RuleServer::handle_switch);
//         srv.Post(RuleServer::switch_URI, RuleServer::handle_switch);
//         srv.Delete(RuleServer::switch_URI, RuleServer::handle_switch);
//         srv.Patch(RuleServer::switch_URI, RuleServer::handle_switch);
//     }
//     srv.listen(addr, port);
//     // test();
// }


//event based - V2
#include "v2.h"
int main(){

    auto q = make_shared<EventQueue>();
    
    Switch trig{};
    trig.setEventQueue(q);

    cv_ctrl targ{};
    targ.setEventQueue(q);

    trig.emitEvent();


}
