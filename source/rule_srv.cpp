#include "rule_srv.h"
#include "trigger_cfg.h"

list<Module> RuleServer::mods = {}; 
constexpr auto json_content = "text/json"s;
shared_ptr<RuleList> RuleServer::rules(){
    static shared_ptr<RuleList> r_ = make_shared<RuleList>();
    return r_;
};
void RuleServer::handle_rules(const httplib::Request& req, httplib::Response& res) noexcept{
    try{
        if(req.method == "GET"){
            res.set_content(rules()->toJson().dump(), json_content);
        }
        else if(req.method == "POST"){
            rules()->fromJson(json::parse(req.body));
        }
        else if(req.method == "PATCH"){
            ConstRef<string> name = req.get_param_value("name");
            auto j = json::parse(req.body);
            rules()->addPolicy(name, policyFromJson(j));
        }
        else if(req.method == "DELETE"){
            ConstRef<string> name = req.get_param_value("name");
            rules()->delPolicy(name);
        }
    }
    catch(ConstRef<exception> ex){
        cout << __func__ << ": " << ex.what() << endl;
    }
}

void RuleServer::handle_mods(const httplib::Request& req, httplib::Response& res) noexcept{
    try{
        if(req.method == "GET"){
            json j;
            if(req.params.empty()){
                j = json::array();
                for(ConstRef<Module> m : mods){
                    j.emplace_back(m.name);
                }
                
            }
            else {
                auto n = req.get_param_value("name");
                auto m_it = find_if(mods.cbegin(), mods.cend(), [&n](ConstRef<Module> m){return m.name == n;});
                if(m_it == mods.cend()) {
                    res.status = 404;
                }
                else{
                    j["active"] = m_it->check();
                }
            }
            res.set_content(j.dump(), json_content);
        }
        else if(req.method == "POST"){
            mods.clear();
            auto j = json::parse(req.body);
            for(ConstRef<json> name : j){
                Module m;
                m.name = name;
                m.setupPolicy(rules());
                mods.emplace_back(std::move(m));
            }
        }
    }
    catch(ConstRef<exception> ex){
        cout << __func__ << ": " << ex.what() << endl;
    }
}

void RuleServer::handle_switch(const httplib::Request& req, httplib::Response& res) noexcept{
    try{
        if(req.method == "GET"){
            res.set_content(SwitchList::ins().getAll().dump(), json_content);
        }
        else if(req.method == "POST"){
            SwitchList::ins().add(json::parse(req.body));
        }
        else if(req.method == "DELETE"){
            SwitchList::ins().del(req.get_param_value("name"));
        }
        else if(req.method == "PATCH"){
            SwitchList::ins().change(json::parse(req.body));
        }
        
        // Signal s{
        //     .name = j.at("signal"),
        //     .new_state = j.at("state")
        // };
        // SwitchMgr::instance().notify(s);
    }
    catch(ConstRef<exception> ex){
        cout << __func__ << ": " << ex.what() << endl;
    }
}

void RuleServer::testInit(){
    for(auto i = 0; i < 5; ++i){
        Module mod;
        mod.name = to_string(i);
        mod.setupPolicy(rules());
        mods.emplace_back(std::move(mod));
    }
}


