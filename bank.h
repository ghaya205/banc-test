#ifndef BANK_H
#define BANK_H

struct Account {
    int accountNumber;
    char username[50];
    char password[20];
    float balance;
};

int generateAccountNumber();
void saveAccount(struct Account acc);
int loginUser(struct Account *acc, const char *username, const char *password);
void updateAccount(struct Account acc);
void depositMoney(struct Account *acc, float amount);
int withdrawMoney(struct Account *acc, float amount);
void recordTransaction(struct Account acc, const char *type, float amount);

#endif
