
#include "BAURA_USER_MANAGER.h"
#include <iostream>

uint64_t UserManager::create_local_account(const std::string& name, const std::string& pwd) {
    uint64_t id = users.size() + 1;
    users.push_back({id, name, "HASHED_PWD", AccountType::LOCAL, true, "PC/C:/Users/" + name});
    return id;
}

bool UserManager::authenticate(const std::string& name, const std::string& pwd) {
    for(auto& u : users) {
        if(u.username == name) {
            current_user_id = u.user_id;
            return true;
        }
    }
    return false;
}
