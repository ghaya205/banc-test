#include <gtk/gtk.h>
#include <stdlib.h>
#include "bank_logic.h" // Include your banking logic header

// Global state variables for the GUI
static GtkWidget *stack;
static struct Account current_user;
static GtkWidget *main_window;
static GtkWidget *status_bar;

// --- Helper Functions ---

static void show_status(const char *message)
{
  gtk_label_set_text(GTK_LABEL(status_bar), message);
}

// --- Callback Functions (omitted for brevity, assume they are correct) ---
// (Your actual file must contain all callback functions: login_button_clicked, signup_button_clicked, etc.)

static void login_button_clicked(GtkWidget *widget, gpointer data)
{
  GtkWidget *username_entry = g_object_get_data(G_OBJECT(main_window), "username_entry");
  GtkWidget *password_entry = g_object_get_data(G_OBJECT(main_window), "password_entry");

  const char *username = gtk_editable_get_text(GTK_EDITABLE(username_entry));
  const char *password = gtk_editable_get_text(GTK_EDITABLE(password_entry));

  if (strcmp(username, ADMIN_USER) == 0 && strcmp(password, ADMIN_PASS) == 0)
  {
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "admin_page");
    show_status("Welcome Admin!");
    return;
  }

  if (loginUser(&current_user, username, password))
  {
    // Function to update user details in the account view
    void update_account_view();
    update_account_view();
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "account_page");
    show_status("Login successful!");
  }
  else
  {
    show_status("Login failed. Invalid user or password.");
  }
}

static void signup_button_clicked(GtkWidget *widget, gpointer data)
{
  GtkWidget *username_entry = g_object_get_data(G_OBJECT(main_window), "username_entry");
  GtkWidget *password_entry = g_object_get_data(G_OBJECT(main_window), "password_entry");

  const char *username = gtk_editable_get_text(GTK_EDITABLE(username_entry));
  const char *password = gtk_editable_get_text(GTK_EDITABLE(password_entry));

  if (strlen(username) < 3 || strlen(password) < 3)
  {
    show_status("Username and password must be at least 3 characters.");
    return;
  }

  int acc_num = userSignup(username, password);
  if (acc_num > 0)
  {
    char msg[100];
    sprintf(msg, "Account created! Acc#: %d. Please log in.", acc_num);
    show_status(msg);
    gtk_editable_set_text(GTK_EDITABLE(username_entry), "");
    gtk_editable_set_text(GTK_EDITABLE(password_entry), "");
  }
  else
  {
    show_status("Error creating account.");
  }
}

static void deposit_button_clicked(GtkWidget *widget, gpointer data)
{
  GtkWidget *amount_entry = g_object_get_data(G_OBJECT(main_window), "deposit_entry");
  const char *text = gtk_editable_get_text(GTK_EDITABLE(amount_entry));
  float amount = atof(text);

  if (amount > 0)
  {
    depositMoney(&current_user, amount);
    void update_account_view();
    update_account_view();
    char msg[100];
    sprintf(msg, "Deposited $%.2f. New Balance: $%.2f", amount, current_user.balance);
    show_status(msg);
    gtk_editable_set_text(GTK_EDITABLE(amount_entry), "");
  }
  else
  {
    show_status("Invalid deposit amount.");
  }
}

static void withdraw_button_clicked(GtkWidget *widget, gpointer data)
{
  GtkWidget *amount_entry = g_object_get_data(G_OBJECT(main_window), "withdraw_entry");
  const char *text = gtk_editable_get_text(GTK_EDITABLE(amount_entry));
  float amount = atof(text);

  if (amount <= 0)
  {
    show_status("Invalid withdrawal amount.");
    return;
  }

  if (withdrawMoney(&current_user, amount))
  {
    void update_account_view();
    update_account_view();
    char msg[100];
    sprintf(msg, "Withdrew $%.2f. New Balance: $%.2f", amount, current_user.balance);
    show_status(msg);
    gtk_editable_set_text(GTK_EDITABLE(amount_entry), "");
  }
  else
  {
    show_status("Withdrawal failed: Insufficient funds.");
  }
}

static void view_transactions_button_clicked(GtkWidget *widget, gpointer data)
{
  char *transactions_text = viewTransactions(current_user.accountNumber);

  GtkWidget *dialog = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Transaction History");
  gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(main_window));

  GtkWidget *scrolled_window = gtk_scrolled_window_new();
  GtkWidget *label = gtk_label_new(transactions_text);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign(GTK_LABEL(label), 0.0);
  gtk_label_set_selectable(GTK_LABEL(label), TRUE);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), label);
  gtk_window_set_child(GTK_WINDOW(dialog), scrolled_window);

  gtk_window_present(GTK_WINDOW(dialog));

  free(transactions_text);
  show_status("Transaction history opened.");
}

static void logout_button_clicked(GtkWidget *widget, gpointer data)
{
  memset(&current_user, 0, sizeof(struct Account));
  gtk_stack_set_visible_child_name(GTK_STACK(stack), "login_page");
  show_status("Logged out successfully.");
}

static void admin_view_button_clicked(GtkWidget *widget, gpointer data)
{
  char *all_accounts = viewAllAccounts();

  GtkWidget *dialog = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "All Bank Accounts");
  gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 400);
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(main_window));

  GtkWidget *scrolled_window = gtk_scrolled_window_new();
  GtkWidget *label = gtk_label_new(all_accounts);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
  gtk_label_set_xalign(GTK_LABEL(label), 0.0);
  gtk_label_set_selectable(GTK_LABEL(label), TRUE);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), label);
  gtk_window_set_child(GTK_WINDOW(dialog), scrolled_window);

  gtk_window_present(GTK_WINDOW(dialog));

  free(all_accounts);
  show_status("All accounts view opened.");
}

static void admin_delete_button_clicked(GtkWidget *widget, gpointer data)
{
  GtkWidget *entry = g_object_get_data(G_OBJECT(main_window), "admin_delete_entry");
  const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
  int target_acc = atoi(text);

  if (target_acc <= 1000)
  {
    show_status("Invalid account number format.");
    return;
  }

  if (deleteAccount(target_acc))
  {
    char msg[100];
    sprintf(msg, "Account #%d deleted successfully.", target_acc);
    show_status(msg);
    gtk_editable_set_text(GTK_EDITABLE(entry), "");
  }
  else
  {
    char msg[100];
    sprintf(msg, "Failed to delete account #%d. Not found.", target_acc);
    show_status(msg);
  }
}

// --- UI Construction Functions ---

void update_account_view()
{
  GtkWidget *acc_label = g_object_get_data(G_OBJECT(main_window), "account_info_label");
  GtkWidget *bal_label = g_object_get_data(G_OBJECT(main_window), "balance_label");

  char acc_info[100];
  sprintf(acc_info, "Welcome, %s! (Acc: %d)", current_user.username, current_user.accountNumber);
  gtk_label_set_text(GTK_LABEL(acc_label), acc_info);

  char balance_info[50];
  sprintf(balance_info, "Current Balance: $%.2f", current_user.balance);
  gtk_label_set_text(GTK_LABEL(bal_label), balance_info);
}

void create_login_page()
{
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
  gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(box, GTK_ALIGN_CENTER);

  GtkWidget *title = gtk_label_new("<big><b>Bank System Login</b></big>");
  gtk_label_set_use_markup(GTK_LABEL(title), TRUE);

  GtkWidget *username_entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(username_entry), "Username");
  gtk_widget_set_size_request(username_entry, 250, -1);

  GtkWidget *password_entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(password_entry), "Password");
  gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
  gtk_widget_set_size_request(password_entry, 250, -1);

  GtkWidget *login_button = gtk_button_new_with_label("Login");
  GtkWidget *signup_button = gtk_button_new_with_label("Sign Up");

  GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_append(GTK_BOX(button_box), login_button);
  gtk_box_append(GTK_BOX(button_box), signup_button);
  gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);

  gtk_box_append(GTK_BOX(box), title);
  gtk_box_append(GTK_BOX(box), username_entry);
  gtk_box_append(GTK_BOX(box), password_entry);
  gtk_box_append(GTK_BOX(box), button_box);

  g_object_set_data(G_OBJECT(main_window), "username_entry", username_entry);
  g_object_set_data(G_OBJECT(main_window), "password_entry", password_entry);

  g_signal_connect(login_button, "clicked", G_CALLBACK(login_button_clicked), NULL);
  g_signal_connect(signup_button, "clicked", G_CALLBACK(signup_button_clicked), NULL);

  gtk_stack_add_titled(GTK_STACK(stack), box, "login_page", "Login");
}

void create_account_page()
{
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

  // FIX: Replaced gtk_widget_set_margin_all with individual setters
  gtk_widget_set_margin_start(box, 20);
  gtk_widget_set_margin_end(box, 20);
  gtk_widget_set_margin_top(box, 20);
  gtk_widget_set_margin_bottom(box, 20);

  GtkWidget *acc_label = gtk_label_new("Welcome, User!");
  gtk_label_set_markup(GTK_LABEL(acc_label), "<big><b>Welcome, User!</b></big>");
  gtk_widget_set_halign(acc_label, GTK_ALIGN_START);
  g_object_set_data(G_OBJECT(main_window), "account_info_label", acc_label);

  GtkWidget *bal_label = gtk_label_new("Current Balance: $0.00");
  gtk_label_set_markup(GTK_LABEL(bal_label), "<b>Current Balance: $0.00</b>");
  gtk_widget_set_halign(bal_label, GTK_ALIGN_START);
  g_object_set_data(G_OBJECT(main_window), "balance_label", bal_label);

  GtkWidget *deposit_label = gtk_label_new("Deposit Amount:");
  GtkWidget *deposit_entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(deposit_entry), "Enter amount");
  GtkWidget *deposit_button = gtk_button_new_with_label("Deposit");
  g_object_set_data(G_OBJECT(main_window), "deposit_entry", deposit_entry);

  GtkWidget *deposit_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_append(GTK_BOX(deposit_row), deposit_label);
  gtk_box_append(GTK_BOX(deposit_row), deposit_entry);
  gtk_box_append(GTK_BOX(deposit_row), deposit_button);

  GtkWidget *withdraw_label = gtk_label_new("Withdraw Amount:");
  GtkWidget *withdraw_entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(withdraw_entry), "Enter amount");
  GtkWidget *withdraw_button = gtk_button_new_with_label("Withdraw");
  g_object_set_data(G_OBJECT(main_window), "withdraw_entry", withdraw_entry);

  GtkWidget *withdraw_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_append(GTK_BOX(withdraw_row), withdraw_label);
  gtk_box_append(GTK_BOX(withdraw_row), withdraw_entry);
  gtk_box_append(GTK_BOX(withdraw_row), withdraw_button);

  GtkWidget *view_transactions_button = gtk_button_new_with_label("View Transactions");
  GtkWidget *logout_button = gtk_button_new_with_label("Logout");

  GtkWidget *action_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_append(GTK_BOX(action_row), view_transactions_button);
  gtk_box_append(GTK_BOX(action_row), logout_button);
  gtk_widget_set_halign(action_row, GTK_ALIGN_CENTER);

  gtk_box_append(GTK_BOX(box), acc_label);
  gtk_box_append(GTK_BOX(box), bal_label);
  gtk_box_append(GTK_BOX(box), deposit_row);
  gtk_box_append(GTK_BOX(box), withdraw_row);
  gtk_box_append(GTK_BOX(box), action_row);

  g_signal_connect(deposit_button, "clicked", G_CALLBACK(deposit_button_clicked), NULL);
  g_signal_connect(withdraw_button, "clicked", G_CALLBACK(withdraw_button_clicked), NULL);
  g_signal_connect(view_transactions_button, "clicked", G_CALLBACK(view_transactions_button_clicked), NULL);
  g_signal_connect(logout_button, "clicked", G_CALLBACK(logout_button_clicked), NULL);

  gtk_stack_add_titled(GTK_STACK(stack), box, "account_page", "Account");
}

void create_admin_page()
{
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

  // FIX: Replaced gtk_widget_set_margin_all with individual setters
  gtk_widget_set_margin_start(box, 20);
  gtk_widget_set_margin_end(box, 20);
  gtk_widget_set_margin_top(box, 20);
  gtk_widget_set_margin_bottom(box, 20);

  GtkWidget *title = gtk_label_new("<big><b>Admin Panel</b></big>");
  gtk_label_set_use_markup(GTK_LABEL(title), TRUE);
  gtk_widget_set_halign(title, GTK_ALIGN_START);

  GtkWidget *view_button = gtk_button_new_with_label("View All Accounts");

  GtkWidget *delete_label = gtk_label_new("Account # to Delete:");
  GtkWidget *delete_entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(delete_entry), "Enter account number");
  GtkWidget *delete_button = gtk_button_new_with_label("Delete Account");
  g_object_set_data(G_OBJECT(main_window), "admin_delete_entry", delete_entry);

  GtkWidget *delete_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_append(GTK_BOX(delete_row), delete_label);
  gtk_box_append(GTK_BOX(delete_row), delete_entry);
  gtk_box_append(GTK_BOX(delete_row), delete_button);

  GtkWidget *logout_button = gtk_button_new_with_label("Logout");

  gtk_box_append(GTK_BOX(box), title);
  gtk_box_append(GTK_BOX(box), view_button);
  gtk_box_append(GTK_BOX(box), delete_row);
  gtk_box_append(GTK_BOX(box), logout_button);

  g_signal_connect(view_button, "clicked", G_CALLBACK(admin_view_button_clicked), NULL);
  g_signal_connect(delete_button, "clicked", G_CALLBACK(admin_delete_button_clicked), NULL);
  g_signal_connect(logout_button, "clicked", G_CALLBACK(logout_button_clicked), NULL);

  gtk_stack_add_titled(GTK_STACK(stack), box, "admin_page", "Admin");
}

// --- Main Application Setup ---

static void activate(GtkApplication *app, gpointer user_data)
{
  main_window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(main_window), "GTK4 Bank Management System");
  gtk_window_set_default_size(GTK_WINDOW(main_window), 500, 450);

  GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child(GTK_WINDOW(main_window), main_vbox);

  stack = gtk_stack_new();
  gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
  gtk_box_append(GTK_BOX(main_vbox), stack);

  create_login_page();
  create_account_page();
  create_admin_page();

  gtk_stack_set_visible_child_name(GTK_STACK(stack), "login_page");

  status_bar = gtk_label_new("Welcome! Please log in or sign up.");
  gtk_widget_set_margin_start(status_bar, 10);
  gtk_widget_set_margin_end(status_bar, 10);
  gtk_widget_set_margin_top(status_bar, 10);
  gtk_widget_set_margin_bottom(status_bar, 10);
  gtk_widget_set_halign(status_bar, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(main_vbox), status_bar);

  gtk_window_present(GTK_WINDOW(main_window));
}

int main(int argc, char *argv[])
{
  GtkApplication *app;
  int status;

  app = gtk_application_new("org.gtk.bankmanagersystem", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}