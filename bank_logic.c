#include "bank_logic.h"

// --- Private (Internal) Functions ---

int generateAccountNumber()
{
  FILE *fp = fopen("lastAccount.txt", "r");
  int last = 1000;
  if (fp)
  {
    fscanf(fp, "%d", &last);
    fclose(fp);
  }
  fp = fopen("lastAccount.txt", "w");
  fprintf(fp, "%d", last + 1);
  fclose(fp);
  return last + 1;
}

void recordTransaction(struct Account acc, const char *type, float amount)
{
  char filename[50];
  sprintf(filename, "transactions_%d.txt", acc.accountNumber);
  FILE *fp = fopen(filename, "a");
  if (!fp)
    return;

  time_t now = time(NULL);
  char *timestamp = ctime(&now);
  timestamp[strlen(timestamp) - 1] = '\0';

  fprintf(fp, "[%s] %s: %.2f\n", timestamp, type, amount);
  fclose(fp);
}

void saveAccount(struct Account acc)
{
  FILE *fp = fopen("accounts.txt", "a");
  if (!fp)
    return;
  fprintf(fp, "%d,%s,%s,%.2f\n", acc.accountNumber, acc.username, acc.password, acc.balance);
  fclose(fp);
}

void updateAccount(struct Account acc)
{
  struct Account temp;
  FILE *fp = fopen("accounts.txt", "r");
  FILE *tempFile = fopen("temp.txt", "w");
  if (!fp || !tempFile)
    return;

  while (fscanf(fp, "%d,%49[^,],%19[^,],%f\n", &temp.accountNumber, temp.username, temp.password, &temp.balance) == 4)
  {
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

// --- Public (Interface) Functions ---

int loginUser(struct Account *acc, const char *username, const char *password)
{
  FILE *fp = fopen("accounts.txt", "r");
  if (!fp)
    return 0;

  // Reset file pointer to the beginning to start search
  fseek(fp, 0, SEEK_SET);

  // Create a temporary structure for reading file lines
  struct Account temp_acc;

  // Loop through the file to find a match
  while (fscanf(fp, "%d,%49[^,],%19[^,],%f\n",
                &temp_acc.accountNumber, temp_acc.username, temp_acc.password, &temp_acc.balance) == 4)
  {

    if (strcmp(username, temp_acc.username) == 0 && strcmp(password, temp_acc.password) == 0)
    {
      // Found a match, copy all data to the provided 'acc' pointer
      *acc = temp_acc;
      fclose(fp);
      return 1; // Success
    }
  }

  fclose(fp);
  return 0; // Failure
}

int userSignup(const char *username, const char *password)
{
  // Note: A robust system would check if the username already exists here.

  struct Account acc;
  strncpy(acc.username, username, sizeof(acc.username) - 1);
  acc.username[sizeof(acc.username) - 1] = '\0';
  strncpy(acc.password, password, sizeof(acc.password) - 1);
  acc.password[sizeof(acc.password) - 1] = '\0';
  acc.accountNumber = generateAccountNumber();
  acc.balance = 0.0;
  saveAccount(acc);
  return acc.accountNumber; // Return new account number on success
}

void depositMoney(struct Account *acc, float amount)
{
  if (amount <= 0)
    return;
  acc->balance += amount;
  recordTransaction(*acc, "Deposited", amount);
  updateAccount(*acc);
}

int withdrawMoney(struct Account *acc, float amount)
{
  if (amount > acc->balance || amount <= 0)
    return 0;

  acc->balance -= amount;
  recordTransaction(*acc, "Withdrawn", amount);
  updateAccount(*acc);
  return 1; // Success
}

char *viewTransactions(int accountNumber)
{
  char filename[50], line[100];
  sprintf(filename, "transactions_%d.txt", accountNumber);
  FILE *fp = fopen(filename, "r");

  if (!fp)
  {
    return strdup("No transactions found.");
  }

  // Read the entire file content into a buffer
  fseek(fp, 0, SEEK_END);
  long fsize = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char *buffer = malloc(fsize + 200); // +200 for header and safety margin
  if (buffer == NULL)
  { // Corrected check
    fclose(fp);
    return strdup("Memory allocation error.");
  }

  // Add header to the output string
  strcpy(buffer, "=== TRANSACTION HISTORY ===\n");
  char *current_pos = buffer + strlen(buffer);

  while (fgets(line, sizeof(line), fp))
  {
    strcat(current_pos, line);
    current_pos += strlen(line);
  }
  fclose(fp);
  return buffer; // Caller must free this memory
}

char *viewAllAccounts()
{
  FILE *fp = fopen("accounts.txt", "r");
  struct Account acc;
  if (!fp)
    return strdup("No accounts found.");

  // Dynamic buffer allocation logic is complex, using a large fixed size for now
  char *buffer = malloc(4096);
  if (buffer == NULL)
  { // Corrected check (Line 171 fix)
    fclose(fp);
    return strdup("Memory error.");
  }
  buffer[0] = '\0';

  char line[200];
  strcat(buffer, "--- ALL ACCOUNTS ---\n");

  while (fscanf(fp, "%d,%49[^,],%19[^,],%f\n",
                &acc.accountNumber, acc.username, acc.password, &acc.balance) == 4)
  {
    sprintf(line, "Acc# %d | User: %s | Balance: %.2f\n", acc.accountNumber, acc.username, acc.balance);
    strcat(buffer, line);
  }
  fclose(fp);
  return buffer; // Caller must free this memory
}

int deleteAccount(int target)
{
  struct Account temp;
  FILE *fp = fopen("accounts.txt", "r");
  FILE *tempFile = fopen("temp.txt", "w");
  if (!fp || !tempFile)
    return 0;

  int found = 0;
  while (fscanf(fp, "%d,%49[^,],%19[^,],%f\n",
                &temp.accountNumber, temp.username, temp.password, &temp.balance) == 4)
  {
    if (temp.accountNumber != target)
      fprintf(tempFile, "%d,%s,%s,%.2f\n", temp.accountNumber, temp.username, temp.password, temp.balance);
    else
      found = 1;
  }
  fclose(fp);
  fclose(tempFile);
  remove("accounts.txt");
  rename("temp.txt", "accounts.txt");

  return found;
}