
#ifndef BAURA_USER_MANAGER_H
#define BAURA_USER_MANAGER_H

#include <string>
#include <vector>
#include <memory>

enum class AccountType { LOCAL, BAURA_CLOUD };

struct UserAccount {
    uint64_t user_id;
    std::string username;
    std::string password_hash;
    AccountType type;
    bool is_admin;
    std::string profile_path;
};

class UserManager {
public:
    uint64_t create_local_account(const std::string& name, const std::string& pwd);
    bool authenticate(const std::string& name, const std::string& pwd);
    void link_to_cloud_account(uint64_t local_id, const std::string& cloud_email);
    UserAccount* get_current_user();

private:
    std::vector<UserAccount> users;
    uint64_t current_user_id;
};
#endif
