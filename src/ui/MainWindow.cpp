#include <QPushButton>
#include <QResizeEvent>
#include <QToolBar>
#include <QWidget>

#include "ui/MainWindow.h"
#include "ui/AddressBar.h"
#include "browser/BrowserHandler.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  // call BuildToolbar() and BuildBrowserContainer()
  BuildToolbar();
  BuildBrowserContainer();
  // set a reasonable initial window size
  resize(1200, 900);
  // set a window title, e.g. setWindowTitle
  setWindowTitle("petrichor");
}

MainWindow::~MainWindow() = default;

void MainWindow::BuildToolbar() {
  // create a QToolBar, add back_button_/forward_button_/reload_button_
  // and address_bar_ to it, addToolBar(toolbar).
  auto* toolbar = new QToolBar(this);
  back_button_ = new QPushButton("<-", this);
  forward_button_ = new QPushButton("->", this);
  reload_button_ = new QPushButton("r", this);
  address_bar_ = new AddressBar(this);
  toolbar->addWidget(back_button_);
  toolbar->addWidget(forward_button_);
  toolbar->addWidget(reload_button_);
  toolbar->addWidget(address_bar_);

  addToolBar(toolbar);

  // connect each button's clicked() signal to the matching slot
  // (OnBackClicked, OnForwardClicked, OnReloadClicked).
  connect(back_button_, &QPushButton::clicked, this, &MainWindow::OnBackClicked);
  connect(forward_button_, &QPushButton::clicked, this, &MainWindow::OnForwardClicked);
  connect(reload_button_, &QPushButton::clicked, this, &MainWindow::OnReloadClicked);

  // connect address_bar_'s urlSubmitted(QString) signal to OnUrlEntered(QString).
  connect(address_bar_, &AddressBar::urlSubmitted, this, &MainWindow::OnUrlEntered);
}



void MainWindow::SetBrowser(CefRefPtr<CefBrowser> browser) {
  browser_ = browser;
}

void MainWindow::BuildBrowserContainer() {
  // construct browser_container_ as a plain QWidget and call

  central_widget_ = new QWidget(this);
  layout_ = new QVBoxLayout(central_widget_);

  tab_bar_ = new TabBar(central_widget_);
  stack_ = new QStackedWidget(central_widget_);
  browser_container_ = new QWidget(central_widget_);
  stack_->addWidget(browser_container_);

  layout_->addWidget(tab_bar_);
  layout_->addWidget(stack_, 1);

  setCentralWidget(central_widget_);

  connect(tab_bar_, &TabBar::currentChanged, stack_, &QStackedWidget::setCurrentIndex);

  connect(tab_bar_, &TabBar::NewTabRequested, this, &MainWindow::OnNewTabRequested);
}

CefWindowHandle MainWindow::GetCefParentHandle() const {
  return reinterpret_cast<cef_window_handle_t>(browser_container_->winId());
}

CefRect MainWindow::GetCefParentRect() const {
  auto rect = browser_container_->rect();
  return {rect.x(), rect.y(), rect.width(), rect.height()}; // CefRect via braced initializer
}

void MainWindow::resizeEvent(QResizeEvent* event) {
  QMainWindow::resizeEvent(event);
  if (!browser_) {
    return;
  }
  BrowserHandler* handler = BrowserHandler::GetInstance();

  if (handler == nullptr) {
    return;
  }
  handler->ResizeBrowser(browser_, GetCefParentRect());
}

void MainWindow::OnUrlEntered(const QString& url) {
  if (!browser_) {
    return;
  }
  CefRefPtr<CefFrame> frame = browser_->GetMainFrame();

  if (!frame) {
    return;
  }
  frame->LoadURL(url.toStdString());
}

void MainWindow::OnBackClicked() {
  if (!browser_) {
    return;
  }

  if (browser_->CanGoBack()) {
    browser_->GoBack();
  }
}

void MainWindow::OnForwardClicked() {
  if (!browser_) {
    return;
  }

  if (browser_->CanGoForward()) {
    browser_->GoForward();
  }
}

void MainWindow::OnReloadClicked() {
  if (!browser_) {
    return;
  }
  browser_->Reload();
}

void MainWindow::OnNewTabRequested() {
  if (!browser_) {
    return;
  }

  tab_bar_->AddTab(browser_->GetIdentifier(), "new tab");
}

void MainWindow::SetUrl(const QString& url) {
  address_bar_->SetUrl(url);
}

void MainWindow::SetNavigationState(bool is_loading, bool canGoBack, bool canGoForward) {
  back_button_->setEnabled(canGoBack);
  forward_button_->setEnabled(canGoForward);
  reload_button_->setEnabled(!is_loading);
}

void MainWindow::SetWindowTitle(const QString& url) {
  this->setWindowTitle(url);
}

void MainWindow::AddTab(int browser_id, QString title) {
  tab_bar_->AddTab(browser_id, title);
}

void MainWindow::RemoveTab(int browser_id) {
  tab_bar_->RemoveTab(browser_id);
}
