#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bank.h"

#define ADMIN_USER "admin"
#define ADMIN_PASS "1234"

/* -------------------- Utility Functions -------------------- */

int generateAccountNumber() {
    FILE *fp = fopen("lastAccount.txt", "r");
    int last = 1000;
    if (fp) {
        fscanf(fp, "%d", &last);
        fclose(fp);
    }
    fp = fopen("lastAccount.txt", "w");
    fprintf(fp, "%d", last + 1);
    fclose(fp);
    return last + 1;
}

void recordTransaction(struct Account acc, const char *type, float amount) {
    char filename[50];
    sprintf(filename, "transactions_%d.txt", acc.accountNumber);
    FILE *fp = fopen(filename, "a");
    if (!fp) return;

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp)-1] = '\0';

    fprintf(fp, "[%s] %s: %.2f\n", timestamp, type, amount);
    fclose(fp);
}

void saveAccount(struct Account acc) {
    FILE *fp = fopen("accounts.txt", "a");
    if (!fp) {
        printf("Error saving account.\n");
        return;
    }
    fprintf(fp, "%d,%s,%s,%.2f\n", acc.accountNumber, acc.username, acc.password, acc.balance);
    fclose(fp);
}

int loginUser(struct Account *acc, const char *username, const char *password) {
    FILE *fp = fopen("accounts.txt", "r");
    if (!fp) return 0;

    while (fscanf(fp, "%d,%49[^,],%19[^,],%f\n",
                  &acc->accountNumber, acc->username, acc->password, &acc->balance) == 4) {
        if (strcmp(username, acc->username) == 0 && strcmp(password, acc->password) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

void updateAccount(struct Account acc) {
    struct Account temp;
    FILE *fp = fopen("accounts.txt", "r");
    FILE *tempFile = fopen("temp.txt", "w");
    if (!fp || !tempFile) return;

    while (fscanf(fp, "%d,%49[^,],%19[^,],%f\n",
                  &temp.accountNumber, temp.username, temp.password, &temp.balance) == 4) {
        if (temp.accountNumber == acc.accountNumber)
            fprintf(tempFile, "%d,%s,%s,%.2f\n", acc.accountNumber, acc.username, acc.password, acc.balance);
        else
            fprintf(tempFile, "%d,%s,%s,%.2f\n", temp.accountNumber, temp.username, temp.password, temp.balance);
    }

    fclose(fp);
    fclose(tempFile);
    remove("accounts.txt");
    rename("temp.txt", "accounts.txt");
}

void depositMoney(struct Account *acc, float amount) {
    acc->balance += amount;
    recordTransaction(*acc, "Deposited", amount);
    updateAccount(*acc);
}

int withdrawMoney(struct Account *acc, float amount) {
    if (amount > acc->balance)
        return 0;
    acc->balance -= amount;
    recordTransaction(*acc, "Withdrawn", amount);
    updateAccount(*acc);
    return 1;
}

/* -------------------- Console Menus -------------------- */

void viewTransactions(struct Account acc) {
    char filename[50], line[100];
    sprintf(filename, "transactions_%d.txt", acc.accountNumber);
    FILE *fp = fopen(filename, "r");

    if (!fp) {
        printf("\nNo transactions found.\n");
        return;
    }

    printf("\n=== TRANSACTION HISTORY ===\n");
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    fclose(fp);
}

void userSignup() {
    struct Account acc;
    printf("\nEnter new username: ");
    scanf("%s", acc.username);
    printf("Enter password: ");
    scanf("%s", acc.password);
    acc.accountNumber = generateAccountNumber();
    acc.balance = 0.0;
    saveAccount(acc);
    printf("Account created successfully! Your account number: %d\n", acc.accountNumber);
}

void userMenu() {
    int choice;
    struct Account acc;
    while (1) {
        printf("\n--- USER MENU ---\n");
        printf("1. Sign Up\n2. Login\n3. Exit\nChoice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: userSignup(); break;
            case 2:
                {
                    char username[50], password[20];
                    printf("Username: "); scanf("%s", username);
                    printf("Password: "); scanf("%s", password);
                    if (loginUser(&acc, username, password)) {
                        printf("\nWelcome, %s!\n", acc.username);
                        int userChoice;
                        do {
                            printf("\n--- ACCOUNT MENU ---\n");
                            printf("1. Deposit\n2. Withdraw\n3. View Transactions\n4. Logout\nChoice: ");
                            scanf("%d", &userChoice);
                            float amount;
                            switch (userChoice) {
                                case 1:
                                    printf("Enter amount to deposit: "); scanf("%f", &amount);
                                    depositMoney(&acc, amount);
                                    printf("Deposit successful! New balance: %.2f\n", acc.balance);
                                    break;
                                case 2:
                                    printf("Enter amount to withdraw: "); scanf("%f", &amount);
                                    if (withdrawMoney(&acc, amount))
                                        printf("Withdrawal successful! New balance: %.2f\n", acc.balance);
                                    else
                                        printf("Insufficient funds!\n");
                                    break;
                                case 3: viewTransactions(acc); break;
                            }
                        } while (userChoice != 4);
                    } else {
                        printf("Invalid username or password.\n");
                    }
                }
                break;
            case 3: return;
        }
    }
}

void viewAllAccounts() {
    FILE *fp = fopen("accounts.txt", "r");
    struct Account acc;
    if (!fp) {
        printf("No accounts found.\n");
        return;
    }
    printf("\n--- ALL ACCOUNTS ---\n");
    while (fscanf(fp, "%d,%49[^,],%19[^,],%f\n",
                  &acc.accountNumber, acc.username, acc.password, &acc.balance) == 4) {
        printf("Acc# %d | User: %s | Balance: %.2f\n", acc.accountNumber, acc.username, acc.balance);
    }
    fclose(fp);
}

void deleteAccount() {
    int target;
    printf("Enter account number to delete: ");
    scanf("%d", &target);

    struct Account temp;
    FILE *fp = fopen("accounts.txt", "r");
    FILE *tempFile = fopen("temp.txt", "w");
    if (!fp || !tempFile) return;

    int found = 0;
    while (fscanf(fp, "%d,%49[^,],%19[^,],%f\n",
                  &temp.accountNumber, temp.username, temp.password, &temp.balance) == 4) {
        if (temp.accountNumber != target)
            fprintf(tempFile, "%d,%s,%s,%.2f\n", temp.accountNumber, temp.username, temp.password, temp.balance);
        else
            found = 1;
    }
    fclose(fp);
    fclose(tempFile);
    remove("accounts.txt");
    rename("temp.txt", "accounts.txt");

    if (found) printf("Account deleted successfully!\n");
    else printf("Account not found.\n");
}

void adminMenu() {
    char user[20], pass[20];
    printf("\nAdmin username: ");
    scanf("%s", user);
    printf("Admin password: ");
    scanf("%s", pass);

    if (strcmp(user, ADMIN_USER) == 0 && strcmp(pass, ADMIN_PASS) == 0) {
        int choice;
        do {
            printf("\n--- ADMIN MENU ---\n");
            printf("1. View All Accounts\n2. Delete Account\n3. Exit\nChoice: ");
            scanf("%d", &choice);
            switch (choice) {
                case 1: viewAllAccounts(); break;
                case 2: deleteAccount(); break;
            }
        } while (choice != 3);
    } else {
        printf("Invalid admin credentials.\n");
    }
}

int main() {
    int choice;
    while (1) {
        printf("\n=== BANK MANAGEMENT SYSTEM ===\n");
        printf("1. Admin Panel\n2. User Portal\n3. Exit\nChoice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: adminMenu(); break;
            case 2: userMenu(); break;
            case 3: exit(0);
        }
    }
}
