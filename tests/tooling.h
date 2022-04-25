//
// Created by Ivan Makarov on 23/3/22.
//

#ifndef ESWB_TOOLING_H
#define ESWB_TOOLING_H

#include <catch2/catch.hpp>
#include "eswb/types.h"
#include "eswb/api.h"
#include <fcntl.h>
#include <string>
#include <list>
#include <map>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <atomic>

class PseudoTopic {
    std::string name;
    std::list<PseudoTopic*> subtopics;
    PseudoTopic *parent;
    std::string path_prefix;
    eswb_topic_id_t parent_id;

    void set_parent(PseudoTopic *f) {
        parent = f;
    }

public:
    PseudoTopic(std::string _name, PseudoTopic *_parent = nullptr):
            name(_name), parent(_parent){
        if (parent != nullptr) {
            _parent->add_subtopic(this);
        }
        parent_id = 0;
    }

    void set_path_prefix(std::string p){
        path_prefix = p;
    }

    std::string &get_path_prefix(){
        return path_prefix;
    }

    auto This() {
        return this;
    }

    void set_parent_id(eswb_topic_id_t ptid) {
        parent_id = ptid;
    }

    eswb_topic_id_t get_parent_id() {
        return parent_id;
    }

    PseudoTopic *add_subtopic(PseudoTopic *f) {
        f->set_parent(this);
        subtopics.push_back(f);

        return this;
    }

    std::string get_path() {
        return parent == nullptr ? "" : parent->get_full_path();
    }

    std::string get_full_path() {
        return get_path() +
               path_prefix + name + "/";
    }

    void create_as_dirs(bool itself = true) {
        if (itself) {
            eswb_rv_t rv = eswb_mkdir(get_path().c_str(), name.c_str());
            REQUIRE(rv == eswb_e_ok);
        }

        for (auto & subtopic : subtopics) {
            subtopic->create_as_dirs();
        }
    }

    void print(int indent=0) {
        std::cout << std::string(indent*2, ' ') << name << std::endl;
        for (auto & subtopic : subtopics) {
            subtopic->print(indent + 1);
        }
    }

    int subtopics_num() {
        int c = 1;
        for (auto & st: subtopics) {
            c += st->subtopics_num();
        }

        return c;
    }

    bool operator== (const PseudoTopic& pt){
        bool rv = this->name == pt.name;

        rv &= subtopics.size() == pt.subtopics.size();

        if (!rv) {
            return false;
        }

        auto st1 = subtopics.begin();
        auto st2 = pt.subtopics.begin();

        for (; st1 != subtopics.end(); st1++, st2++) {
            PseudoTopic &pt1 = *(*st1)->This();
            PseudoTopic &pt2 = *(*st2)->This();
            rv &= pt1 == pt2;
        }

        return rv;
    }
};

class ExtractedTopicsRegistry {
    std::map<eswb_topic_id_t, PseudoTopic*> registry;
public:
    ExtractedTopicsRegistry() {
    }

    void add2reg(const char *name, eswb_topic_id_t id, eswb_topic_id_t parent_id) {
        auto nt = new PseudoTopic(std::string(name));
        nt->set_parent_id(parent_id);
        registry.insert(std::make_pair(id, nt));
    }
    void add2reg(PseudoTopic *p, eswb_topic_id_t parent_id = 0) {
        registry.insert(std::make_pair(parent_id, p));
    }

    PseudoTopic *build_hierarhy() {
        for (auto &t : registry) {
            auto i = registry.find(t.second->get_parent_id());

            if (i != registry.end()) {
                auto parent = i->second;
                if (parent != t.second) {
                    parent->add_subtopic(t.second);
                }
            } else {
                std::cerr << "There is no topic with " << t.second->get_parent_id() << " id " << std::endl;
            }
        }

        return registry.begin()->second;
    }
};


typedef std::function<void (void)> periodic_call_t;

class timed_caller {

    periodic_call_t &timed_call;
    uint32_t period;
    std::thread th;

    std::atomic_bool run;

    void loop() {
        while(run) {
            std::this_thread::sleep_for(std::chrono::milliseconds(period));
            timed_call();
        }
    }

    void once () {
        std::this_thread::sleep_for(std::chrono::milliseconds(period));
        timed_call();
    };

public:
    timed_caller(periodic_call_t &call, uint32_t p) : timed_call(call), period(p), run(false) {
    }

    ~timed_caller() {
        stop();
    }

    void start_loop(bool detach = false) {
        run = true;
        th = std::thread(&timed_caller::loop, this);
        if (detach) {
            th.detach();
        }
    }

    void start_once(bool detach = false) {
        run = true;
        th = std::thread(&timed_caller::once, this);
        if (detach) {
            th.detach();
        }
    }

    void stop() {
        run = false;
        if (th.joinable()) {
            th.join();
        }
    }

    void wait() {
        if (th.joinable()) {
            th.join();
        }
    }
};


std::string gen_name();
PseudoTopic *gen_folder(PseudoTopic *f = nullptr);

#endif //ESWB_TOOLING_H
