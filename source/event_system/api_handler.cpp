#include "api_handler.hpp"
#include <exception>

void APIHandler::registerRoutes(httplib::Server& srv) {
    // GET /trigger — trigger list by kinds (read-only)
    srv.Get(trigger_URI, [this](const httplib::Request& req, httplib::Response& res) {
        handleTriggerList(req, res);
    });

    // GET /target — target list (read-only, system-only editable)
    srv.Get(target_URI, [this](const httplib::Request& req, httplib::Response& res) {
        handleTargetList(req, res);
    });

    // GET /action — list all actions
    srv.Get(action_URI, [this](const httplib::Request& req, httplib::Response& res) {
        handleActionList(req, res);
    });

    // POST /action — create action with cmds
    srv.Post(action_URI, [this](const httplib::Request& req, httplib::Response& res) {
        handleActionCreate(req, res);
    });

    // DELETE /action — delete action by name
    srv.Delete(action_URI, [this](const httplib::Request& req, httplib::Response& res) {
        handleActionDelete(req, res);
    });

    // GET /link — list trigger-action links
    srv.Get(link_URI, [this](const httplib::Request& req, httplib::Response& res) {
        handleLinkList(req, res);
    });

    // POST /link — link trigger with action
    srv.Post(link_URI, [this](const httplib::Request& req, httplib::Response& res) {
        handleLinkCreate(req, res);
    });

    // DELETE /link — unlink trigger from action
    srv.Delete(link_URI, [this](const httplib::Request& req, httplib::Response& res) {
        handleLinkDelete(req, res);
    });

    cout << "APIHandler: routes registered\n";
}

void APIHandler::handleTriggerList(const httplib::Request& req, httplib::Response& res) {
    try {
        if (triggers_) {
            res.set_content(triggers_->toKindListJson().dump(), json_content);
        } else {
            res.set_content(json::object().dump(), json_content);
        }
    } catch (const exception& ex) {
        res.status = 500;
        res.set_content(json{{"error", ex.what()}}.dump(), json_content);
    }
}

void APIHandler::handleTargetList(const httplib::Request& req, httplib::Response& res) {
    try {
        if (targets_) {
            res.set_content(targets_->toJson().dump(), json_content);
        } else {
            res.set_content(json::array().dump(), json_content);
        }
    } catch (const exception& ex) {
        res.status = 500;
        res.set_content(json{{"error", ex.what()}}.dump(), json_content);
    }
}

void APIHandler::handleActionList(const httplib::Request& req, httplib::Response& res) {
    try {
        if (actions_) {
            res.set_content(actions_->actionsToJson().dump(), json_content);
        } else {
            res.set_content(json::object().dump(), json_content);
        }
    } catch (const exception& ex) {
        res.status = 500;
        res.set_content(json{{"error", ex.what()}}.dump(), json_content);
    }
}

void APIHandler::handleActionCreate(const httplib::Request& req, httplib::Response& res) {
    try {
        auto j = json::parse(req.body);
        auto action = Action::fromJson(j);
        if (actions_) {
            actions_->addAction(action);
            res.set_content(json{{"status", "created"}, {"name", action.name}}.dump(), json_content);
        } else {
            res.status = 500;
            res.set_content(json{{"error", "no action registry"}}.dump(), json_content);
        }
    } catch (const exception& ex) {
        res.status = 400;
        res.set_content(json{{"error", ex.what()}}.dump(), json_content);
    }
}

void APIHandler::handleActionDelete(const httplib::Request& req, httplib::Response& res) {
    try {
        string name = req.get_param_value("name");
        if (actions_) {
            actions_->removeAction(name);
            res.set_content(json{{"status", "deleted"}, {"name", name}}.dump(), json_content);
        } else {
            res.status = 500;
            res.set_content(json{{"error", "no action registry"}}.dump(), json_content);
        }
    } catch (const exception& ex) {
        res.status = 400;
        res.set_content(json{{"error", ex.what()}}.dump(), json_content);
    }
}

void APIHandler::handleLinkList(const httplib::Request& req, httplib::Response& res) {
    try {
        if (actions_) {
            res.set_content(actions_->linksToJson().dump(), json_content);
        } else {
            res.set_content(json::array().dump(), json_content);
        }
    } catch (const exception& ex) {
        res.status = 500;
        res.set_content(json{{"error", ex.what()}}.dump(), json_content);
    }
}

void APIHandler::handleLinkCreate(const httplib::Request& req, httplib::Response& res) {
    try {
        auto j = json::parse(req.body);
        auto link = TriggerActionLink::fromJson(j);
        if (actions_) {
            actions_->addLink(link);
            res.set_content(json{{"status", "linked"}, {"trigger", link.trigger}, {"action", link.action}}.dump(), json_content);
        } else {
            res.status = 500;
            res.set_content(json{{"error", "no action registry"}}.dump(), json_content);
        }
    } catch (const exception& ex) {
        res.status = 400;
        res.set_content(json{{"error", ex.what()}}.dump(), json_content);
    }
}

void APIHandler::handleLinkDelete(const httplib::Request& req, httplib::Response& res) {
    try {
        string trigger = req.get_param_value("trigger");
        string action = req.get_param_value("action");
        if (actions_) {
            actions_->removeLink(trigger, action);
            res.set_content(json{{"status", "unlinked"}, {"trigger", trigger}, {"action", action}}.dump(), json_content);
        } else {
            res.status = 500;
            res.set_content(json{{"error", "no action registry"}}.dump(), json_content);
        }
    } catch (const exception& ex) {
        res.status = 400;
        res.set_content(json{{"error", ex.what()}}.dump(), json_content);
    }
}
