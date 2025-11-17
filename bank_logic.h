#ifndef BANK_LOGIC_H
#define BANK_LOGIC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ADMIN_USER "admin"
#define ADMIN_PASS "1234"

// Account structure
struct Account
{
  int accountNumber;
  char username[50];
  char password[20];
  float balance;
};

// Function Declarations that the GUI will use
// Note: acc is updated by reference if login is successful.
int loginUser(struct Account *acc, const char *username, const char *password);
int userSignup(const char *username, const char *password); // Returns new account number
void depositMoney(struct Account *acc, float amount);
int withdrawMoney(struct Account *acc, float amount); // Returns 1 on success, 0 on failure
char *viewTransactions(int accountNumber);            // Returns a dynamically allocated string
void updateAccount(struct Account acc);

// Admin functions
char *viewAllAccounts();              // Returns a dynamically allocated string
int deleteAccount(int accountNumber); // Returns 1 on success, 0 on not found

#endif // BANK_LOGIC_H