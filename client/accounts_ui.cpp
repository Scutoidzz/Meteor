/**
 * client/accounts_ui.cpp — Accounts UI screens implementation
 * ===========================================================
 * Login, sign-up, and account management screens.
 */

#include "accounts_ui.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QStackedWidget>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QScreen>
#include <QDesktopServices>
#include <QUrl>
#include <QDate>

#include <regex>

// ─────────────────────────────────────────────────────────────────────────────
// Helper functions
// ─────────────────────────────────────────────────────────────────────────────

static bool isValidEmail(const QString& email) {
    // Simple email validation
    static const std::regex pattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email.toStdString(), pattern);
}

static bool isValidPassword(const QString& password) {
    // Password must be at least 8 characters
    return password.length() >= 8;
}

static std::pair<int, int> idealWindowSize(int screenW, int screenH, double fraction = 0.5) {
    int width = std::max(500, static_cast<int>(screenW * fraction));
    int height = std::max(400, static_cast<int>(screenH * fraction));
    return {width, height};
}

// ─────────────────────────────────────────────────────────────────────────────
// LoginScreen implementation
// ─────────────────────────────────────────────────────────────────────────────

LoginScreen::LoginScreen(QWidget* parent)
    : QWidget(parent), m_emailInput(nullptr), m_passwordInput(nullptr),
      m_loginBtn(nullptr), m_signUpBtn(nullptr), m_forgotBtn(nullptr),
      m_rememberMe(nullptr) {
    initUI();
}

void LoginScreen::initUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    layout->setContentsMargins(40, 40, 40, 40);

    // Title
    QLabel* title = new QLabel("Login to Meteor");
    QFont titleFont = title->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    // Subtitle
    QLabel* subtitle = new QLabel("Enter your credentials to access your account");
    QFont subtitleFont = subtitle->font();
    subtitleFont.setPointSize(10);
    subtitle->setFont(subtitleFont);
    subtitle->setStyleSheet("color: #888;");
    layout->addWidget(subtitle);

    layout->addSpacing(20);

    // Email input
    layout->addWidget(new QLabel("Email:"));
    m_emailInput = new QLineEdit();
    m_emailInput->setPlaceholderText("your@email.com");
    m_emailInput->setMinimumHeight(40);
    layout->addWidget(m_emailInput);

    // Password input
    layout->addWidget(new QLabel("Password:"));
    m_passwordInput = new QLineEdit();
    m_passwordInput->setPlaceholderText("••••••••");
    m_passwordInput->setEchoMode(QLineEdit::Password);
    m_passwordInput->setMinimumHeight(40);
    layout->addWidget(m_passwordInput);

    // Remember me checkbox
    m_rememberMe = new QCheckBox("Remember me");
    layout->addWidget(m_rememberMe);

    layout->addSpacing(10);

    // Login button
    m_loginBtn = new QPushButton("Login");
    m_loginBtn->setMinimumHeight(44);
    m_loginBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #007AFF;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #0051D5;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #003DA8;"
        "}"
    );
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginScreen::onLoginClicked);
    layout->addWidget(m_loginBtn);

    layout->addSpacing(16);

    // Forgot password button
    m_forgotBtn = new QPushButton("Forgot Password?");
    m_forgotBtn->setFlat(true);
    m_forgotBtn->setStyleSheet("color: #007AFF; text-decoration: underline;");
    connect(m_forgotBtn, &QPushButton::clicked, this, &LoginScreen::switchToResetPassword);
    layout->addWidget(m_forgotBtn);

    layout->addSpacing(16);

    // Sign up prompt
    QHBoxLayout* signupPrompt = new QHBoxLayout();
    signupPrompt->addWidget(new QLabel("Don't have an account?"));
    m_signUpBtn = new QPushButton("Sign Up");
    m_signUpBtn->setFlat(true);
    m_signUpBtn->setStyleSheet("color: #007AFF; text-decoration: underline; padding: 0;");
    connect(m_signUpBtn, &QPushButton::clicked, this, &LoginScreen::switchToSignUp);
    signupPrompt->addWidget(m_signUpBtn);
    signupPrompt->addStretch();
    layout->addLayout(signupPrompt);

    layout->addStretch();
}

void LoginScreen::onLoginClicked() {
    QString email = m_emailInput->text().trimmed();
    QString password = m_passwordInput->text();

    if (email.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter your email address.");
        return;
    }

    if (!isValidEmail(email)) {
        QMessageBox::warning(this, "Validation Error", "Please enter a valid email address.");
        return;
    }

    if (password.isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Please enter your password.");
        return;
    }

    emit loginAttempted(email, password);
}

// ─────────────────────────────────────────────────────────────────────────────
// SignUpScreen implementation
// ─────────────────────────────────────────────────────────────────────────────

SignUpScreen::SignUpScreen(QWidget* parent)
    : QWidget(parent), m_emailInput(nullptr), m_passwordInput(nullptr),
      m_confirmPasswordInput(nullptr), m_usernameInput(nullptr),
      m_signUpBtn(nullptr), m_backBtn(nullptr), m_errorLabel(nullptr) {
    initUI();
}

void SignUpScreen::initUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(14);
    layout->setContentsMargins(40, 40, 40, 40);

    // Title
    QLabel* title = new QLabel("Create Account");
    QFont titleFont = title->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    // Subtitle
    QLabel* subtitle = new QLabel("Join Meteor to get started");
    QFont subtitleFont = subtitle->font();
    subtitleFont.setPointSize(10);
    subtitle->setFont(subtitleFont);
    subtitle->setStyleSheet("color: #888;");
    layout->addWidget(subtitle);

    layout->addSpacing(16);

    // Username input
    layout->addWidget(new QLabel("Username:"));
    m_usernameInput = new QLineEdit();
    m_usernameInput->setPlaceholderText("Choose a username");
    m_usernameInput->setMinimumHeight(40);
    layout->addWidget(m_usernameInput);

    // Email input
    layout->addWidget(new QLabel("Email:"));
    m_emailInput = new QLineEdit();
    m_emailInput->setPlaceholderText("your@email.com");
    m_emailInput->setMinimumHeight(40);
    layout->addWidget(m_emailInput);

    // Password input
    layout->addWidget(new QLabel("Password:"));
    m_passwordInput = new QLineEdit();
    m_passwordInput->setPlaceholderText("••••••••");
    m_passwordInput->setEchoMode(QLineEdit::Password);
    m_passwordInput->setMinimumHeight(40);
    layout->addWidget(m_passwordInput);

    // Confirm password input
    layout->addWidget(new QLabel("Confirm Password:"));
    m_confirmPasswordInput = new QLineEdit();
    m_confirmPasswordInput->setPlaceholderText("••••••••");
    m_confirmPasswordInput->setEchoMode(QLineEdit::Password);
    m_confirmPasswordInput->setMinimumHeight(40);
    layout->addWidget(m_confirmPasswordInput);

    // Error label
    m_errorLabel = new QLabel();
    m_errorLabel->setStyleSheet("color: #FF3B30;");
    m_errorLabel->setWordWrap(true);
    layout->addWidget(m_errorLabel);

    layout->addSpacing(10);

    // Sign up button
    m_signUpBtn = new QPushButton("Create Account");
    m_signUpBtn->setMinimumHeight(44);
    m_signUpBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #007AFF;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #0051D5;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #003DA8;"
        "}"
    );
    connect(m_signUpBtn, &QPushButton::clicked, this, &SignUpScreen::onSignUpClicked);
    layout->addWidget(m_signUpBtn);

    layout->addSpacing(16);

    // Back to login
    m_backBtn = new QPushButton("Back to Login");
    m_backBtn->setFlat(true);
    m_backBtn->setStyleSheet("color: #007AFF;");
    connect(m_backBtn, &QPushButton::clicked, this, &SignUpScreen::switchToLogin);
    layout->addWidget(m_backBtn);

    layout->addStretch();
}

bool SignUpScreen::validateForm() {
    QString username = m_usernameInput->text().trimmed();
    QString email = m_emailInput->text().trimmed();
    QString password = m_passwordInput->text();
    QString confirmPassword = m_confirmPasswordInput->text();

    if (username.isEmpty()) {
        m_errorLabel->setText("Please enter a username.");
        return false;
    }

    if (email.isEmpty() || !isValidEmail(email)) {
        m_errorLabel->setText("Please enter a valid email address.");
        return false;
    }

    if (!isValidPassword(password)) {
        m_errorLabel->setText("Password must be at least 8 characters long.");
        return false;
    }

    if (password != confirmPassword) {
        m_errorLabel->setText("Passwords do not match.");
        return false;
    }

    m_errorLabel->clear();
    return true;
}

void SignUpScreen::onSignUpClicked() {
    if (!validateForm()) {
        return;
    }

    QString email = m_emailInput->text().trimmed();
    QString password = m_passwordInput->text();
    QString confirmPassword = m_confirmPasswordInput->text();

    emit signUpAttempted(email, password, confirmPassword);
}

// ─────────────────────────────────────────────────────────────────────────────
// ResetPasswordScreen implementation
// ─────────────────────────────────────────────────────────────────────────────

ResetPasswordScreen::ResetPasswordScreen(QWidget* parent)
    : QWidget(parent), m_emailInput(nullptr), m_resetBtn(nullptr),
      m_backBtn(nullptr), m_statusLabel(nullptr) {
    initUI();
}

void ResetPasswordScreen::initUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    layout->setContentsMargins(40, 40, 40, 40);

    // Title
    QLabel* title = new QLabel("Reset Password");
    QFont titleFont = title->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    // Subtitle
    QLabel* subtitle = new QLabel("Enter your email to receive password reset instructions");
    QFont subtitleFont = subtitle->font();
    subtitleFont.setPointSize(10);
    subtitle->setFont(subtitleFont);
    subtitle->setStyleSheet("color: #888;");
    layout->addWidget(subtitle);

    layout->addSpacing(20);

    // Email input
    layout->addWidget(new QLabel("Email:"));
    m_emailInput = new QLineEdit();
    m_emailInput->setPlaceholderText("your@email.com");
    m_emailInput->setMinimumHeight(40);
    layout->addWidget(m_emailInput);

    layout->addSpacing(10);

    // Reset button
    m_resetBtn = new QPushButton("Send Reset Link");
    m_resetBtn->setMinimumHeight(44);
    m_resetBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #007AFF;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #0051D5;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #003DA8;"
        "}"
    );
    connect(m_resetBtn, &QPushButton::clicked, this, &ResetPasswordScreen::onResetClicked);
    layout->addWidget(m_resetBtn);

    // Status label
    m_statusLabel = new QLabel();
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);

    layout->addSpacing(16);

    // Back to login
    m_backBtn = new QPushButton("Back to Login");
    m_backBtn->setFlat(true);
    m_backBtn->setStyleSheet("color: #007AFF;");
    connect(m_backBtn, &QPushButton::clicked, this, &ResetPasswordScreen::switchToLogin);
    layout->addWidget(m_backBtn);

    layout->addStretch();
}

void ResetPasswordScreen::onResetClicked() {
    QString email = m_emailInput->text().trimmed();

    if (email.isEmpty() || !isValidEmail(email)) {
        setStatus("Please enter a valid email address.");
        return;
    }

    emit resetAttempted(email);
}

void ResetPasswordScreen::setStatus(const QString& message, const QString& color) {
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(QString("color: %1;").arg(color));
}

// ─────────────────────────────────────────────────────────────────────────────
// AccountDashboard implementation
// ─────────────────────────────────────────────────────────────────────────────

AccountDashboard::AccountDashboard(const QString& email, QWidget* parent)
    : QWidget(parent), m_email(email), m_usernameLabel(nullptr),
      m_emailLabel(nullptr), m_joinDateLabel(nullptr),
      m_changePasswordBtn(nullptr), m_updateProfileBtn(nullptr),
      m_logoutBtn(nullptr) {
    initUI();
    loadUserInfo();
}

void AccountDashboard::initUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    layout->setContentsMargins(40, 40, 40, 40);

    // Title
    QLabel* title = new QLabel("Account Dashboard");
    QFont titleFont = title->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    layout->addSpacing(20);

    // User info section
    QLabel* infoTitle = new QLabel("Account Information");
    QFont infoFont = infoTitle->font();
    infoFont.setPointSize(12);
    infoFont.setBold(true);
    infoTitle->setFont(infoFont);
    layout->addWidget(infoTitle);

    // Username
    layout->addWidget(new QLabel("Username:"));
    m_usernameLabel = new QLabel();
    m_usernameLabel->setStyleSheet("color: #666;");
    layout->addWidget(m_usernameLabel);

    // Email
    layout->addWidget(new QLabel("Email:"));
    m_emailLabel = new QLabel(m_email);
    m_emailLabel->setStyleSheet("color: #666;");
    layout->addWidget(m_emailLabel);

    // Join date
    layout->addWidget(new QLabel("Member Since:"));
    m_joinDateLabel = new QLabel();
    m_joinDateLabel->setStyleSheet("color: #666;");
    layout->addWidget(m_joinDateLabel);

    layout->addSpacing(20);

    // Action buttons
    QLabel* actionsTitle = new QLabel("Settings");
    QFont actionsFont = actionsTitle->font();
    actionsFont.setPointSize(12);
    actionsFont.setBold(true);
    actionsTitle->setFont(actionsFont);
    layout->addWidget(actionsTitle);

    m_updateProfileBtn = new QPushButton("Update Profile");
    m_updateProfileBtn->setMinimumHeight(40);
    connect(m_updateProfileBtn, &QPushButton::clicked, this, &AccountDashboard::onUpdateProfileClicked);
    layout->addWidget(m_updateProfileBtn);

    m_changePasswordBtn = new QPushButton("Change Password");
    m_changePasswordBtn->setMinimumHeight(40);
    connect(m_changePasswordBtn, &QPushButton::clicked, this, &AccountDashboard::onChangePasswordClicked);
    layout->addWidget(m_changePasswordBtn);

    layout->addSpacing(20);

    // Logout button
    m_logoutBtn = new QPushButton("Logout");
    m_logoutBtn->setMinimumHeight(44);
    m_logoutBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #FF3B30;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #CC2B20;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #991F15;"
        "}"
    );
    connect(m_logoutBtn, &QPushButton::clicked, this, &AccountDashboard::onLogoutClicked);
    layout->addWidget(m_logoutBtn);

    layout->addStretch();
}

void AccountDashboard::loadUserInfo() {
    // Extract username from email (before @)
    int atIndex = m_email.indexOf('@');
    QString username = atIndex > 0 ? m_email.left(atIndex) : m_email;
    m_usernameLabel->setText(username);

    // Set join date to today for now (would come from server in real implementation)
    m_joinDateLabel->setText(QDate::currentDate().toString("MMMM d, yyyy"));
}

void AccountDashboard::onLogoutClicked() {
    emit logout();
}

void AccountDashboard::onChangePasswordClicked() {
    QMessageBox::information(this, "Change Password",
        "Password change functionality would open a dialog here.");
}

void AccountDashboard::onUpdateProfileClicked() {
    QMessageBox::information(this, "Update Profile",
        "Profile update functionality would open a dialog here.");
}

// ─────────────────────────────────────────────────────────────────────────────
// AccountsManager implementation
// ─────────────────────────────────────────────────────────────────────────────

AccountsManager::AccountsManager(QWidget* parent)
    : QWidget(parent), m_stackedWidget(nullptr) {
    initUI();
    showLoginScreen();
}

void AccountsManager::initUI() {
    setWindowTitle("Meteor Accounts");

    // Calculate ideal window size
    QScreen* screen = QApplication::primaryScreen();
    auto [winW, winH] = idealWindowSize(screen->geometry().width(),
                                         screen->geometry().height(), 0.5);
    resize(winW, winH);

    // Create stacked widget
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_stackedWidget = new QStackedWidget();

    // Create screens
    m_loginScreen = std::make_unique<LoginScreen>();
    m_signUpScreen = std::make_unique<SignUpScreen>();
    m_resetScreen = std::make_unique<ResetPasswordScreen>();
    m_dashboard = nullptr;  // Created on demand after login

    // Add screens to stacked widget
    m_stackedWidget->addWidget(m_loginScreen.get());      // 0
    m_stackedWidget->addWidget(m_signUpScreen.get());     // 1
    m_stackedWidget->addWidget(m_resetScreen.get());      // 2

    // Add a placeholder for dashboard (will be replaced)
    QWidget* placeholder = new QWidget();
    m_stackedWidget->addWidget(placeholder);              // 3

    mainLayout->addWidget(m_stackedWidget);

    // Connect signals
    connect(m_loginScreen.get(), &LoginScreen::switchToSignUp,
            this, &AccountsManager::showSignUpScreen);
    connect(m_loginScreen.get(), &LoginScreen::switchToResetPassword,
            this, &AccountsManager::showResetPasswordScreen);
    connect(m_loginScreen.get(), &LoginScreen::loginAttempted,
            this, &AccountsManager::handleLogin);

    connect(m_signUpScreen.get(), &SignUpScreen::switchToLogin,
            this, &AccountsManager::showLoginScreen);
    connect(m_signUpScreen.get(), &SignUpScreen::signUpAttempted,
            this, &AccountsManager::handleSignUp);

    connect(m_resetScreen.get(), &ResetPasswordScreen::switchToLogin,
            this, &AccountsManager::showLoginScreen);
    connect(m_resetScreen.get(), &ResetPasswordScreen::resetAttempted,
            this, &AccountsManager::handleReset);
}

void AccountsManager::showLoginScreen() {
    m_stackedWidget->setCurrentIndex(LOGIN_SCREEN);
}

void AccountsManager::showSignUpScreen() {
    m_stackedWidget->setCurrentIndex(SIGNUP_SCREEN);
}

void AccountsManager::showResetPasswordScreen() {
    m_stackedWidget->setCurrentIndex(RESET_SCREEN);
}

void AccountsManager::showDashboard(const QString& email) {
    // Remove old dashboard if exists
    if (m_dashboard) {
        disconnect(m_dashboard.get(), nullptr, this, nullptr);
        m_stackedWidget->removeWidget(m_dashboard.get());
    }

    // Create new dashboard
    m_dashboard = std::make_unique<AccountDashboard>(email);
    connect(m_dashboard.get(), &AccountDashboard::logout,
            this, &AccountsManager::handleLogout);

    m_stackedWidget->addWidget(m_dashboard.get());
    m_stackedWidget->setCurrentWidget(m_dashboard.get());
}

void AccountsManager::handleLogout() {
    // Reset screens
    m_loginScreen = std::make_unique<LoginScreen>();
    m_signUpScreen = std::make_unique<SignUpScreen>();
    m_resetScreen = std::make_unique<ResetPasswordScreen>();
    m_dashboard = nullptr;

    // Clear stacked widget and re-add screens
    while (m_stackedWidget->count() > 0) {
        QWidget* w = m_stackedWidget->widget(0);
        m_stackedWidget->removeWidget(w);
        if (w != m_loginScreen.get() && w != m_signUpScreen.get() && w != m_resetScreen.get()) {
            delete w;
        }
    }

    m_stackedWidget->addWidget(m_loginScreen.get());
    m_stackedWidget->addWidget(m_signUpScreen.get());
    m_stackedWidget->addWidget(m_resetScreen.get());

    // Reconnect signals
    connect(m_loginScreen.get(), &LoginScreen::switchToSignUp,
            this, &AccountsManager::showSignUpScreen);
    connect(m_loginScreen.get(), &LoginScreen::switchToResetPassword,
            this, &AccountsManager::showResetPasswordScreen);
    connect(m_loginScreen.get(), &LoginScreen::loginAttempted,
            this, &AccountsManager::handleLogin);

    connect(m_signUpScreen.get(), &SignUpScreen::switchToLogin,
            this, &AccountsManager::showLoginScreen);
    connect(m_signUpScreen.get(), &SignUpScreen::signUpAttempted,
            this, &AccountsManager::handleSignUp);

    connect(m_resetScreen.get(), &ResetPasswordScreen::switchToLogin,
            this, &AccountsManager::showLoginScreen);
    connect(m_resetScreen.get(), &ResetPasswordScreen::resetAttempted,
            this, &AccountsManager::handleReset);

    showLoginScreen();
}

void AccountsManager::handleLogin(const QString& email, const QString& password) {
    // In a real app, this would validate credentials with the server
    // For now, just show a success message and go to dashboard
    QMessageBox::information(this, "Login",
        QString("Would login as: %1\n(Server validation would happen here)").arg(email));
    showDashboard(email);
}

void AccountsManager::handleSignUp(const QString& email, const QString& password,
                                   const QString& confirmPassword) {
    // In a real app, this would register with the server
    QMessageBox::information(this, "Sign Up",
        QString("Would create account for: %1\n(Server registration would happen here)").arg(email));
    showDashboard(email);
}

void AccountsManager::handleReset(const QString& email) {
    // In a real app, this would send reset email
    m_resetScreen->setStatus(
        "If an account exists with this email, a password reset link has been sent.",
        "#34C759");
}

// ─────────────────────────────────────────────────────────────────────────────
// Entry point: standalone executable
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    AccountsManager manager;
    manager.show();
    return app.exec();
}
