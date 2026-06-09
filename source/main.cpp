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

void test();

atomic<bool> started = true;
int main(){
    httplib::Server srv;
    srv.Get("/test"s, test_handler);
    constexpr auto port = 8080;
    constexpr auto addr = "localhost";
    cout << "Server listen on http://" << addr << ":" << port << endl;
    {
        RuleServer::testInit();
        srv.Get(RuleServer::rule_URI, RuleServer::handle_rules);
        srv.Post(RuleServer::rule_URI, RuleServer::handle_rules);
        srv.Patch(RuleServer::rule_URI, RuleServer::handle_rules);

        srv.Get(RuleServer::mods_URI, RuleServer::handle_mods);
        srv.Post(RuleServer::mods_URI, RuleServer::handle_mods);
    }
    srv.listen(addr, port);
    // test();
}

// void test(){
//     class AlwaysActive : public RuleI{
//         public:
//         bool isActive() const override{return true;}
//     };
//     class AlwaysInActive : public RuleI{
//         public:
//         bool isActive() const override{return false;}
//     };
    
//     auto policy = make_shared<RuleList>();
//     set<string> targets = {"0"};
//     auto sw_policy = make_shared<Switcher>();
//     sw_policy->on();
//     policy->add_policy("permanent", Policy{sw_policy, targets});

//     targets = {"1"};
//     auto always_not = make_shared<AlwaysInActive>();
//     policy->add_policy("inactive"s, Policy{always_not, targets});

//     list<jthread> ths;
//     for(auto i = 0; i < 2; ++i){
//         ths.emplace_back(std::move(jthread(
//         [=](){
//             Module mod;
//             mod.name = to_string(i);
//             mod.setupPolicy(policy);
//             while(started){
//                 mod.work();
//             }
//         })));
//     }
//     using namespace chrono_literals;
//     this_thread::sleep_for(20s);
//     started = false;
// }