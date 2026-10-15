#include <QPushButton>
#include <QEvent>
#include <QSignalBlocker>
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
  central_widget_ = new QWidget(this);
  layout_ = new QVBoxLayout(central_widget_);

  tab_bar_ = new TabBar(central_widget_);
  stack_ = new QStackedWidget(central_widget_);

  layout_->addWidget(tab_bar_);
  layout_->addWidget(stack_, 1);

  setCentralWidget(central_widget_);

  connect(tab_bar_, &TabBar::currentChanged, this, &MainWindow::OnCurrentTabChanged);

  connect(tab_bar_, &TabBar::NewTabRequested, this, &MainWindow::OnNewTabRequested);
  connect(tab_bar_, &TabBar::tabCloseRequested, this, &MainWindow::OnCloseTabRequested);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
  if (event->type() == QEvent::Resize || event->type() == QEvent::Show) {
    if (auto* page = qobject_cast<QWidget*>(watched)) {
      ResizeBrowserForPage(page);
    }
  }
  return QMainWindow::eventFilter(watched, event);
}

void MainWindow::ResizeBrowserForPage(QWidget* page) {
  auto* handler = BrowserHandler::GetInstance();
  if (!handler) {
    return;
  }
  const int browser_id = host_pages_by_browser_id_.key(page, -1);
  const auto browser = tab_manager_.FindBrowser(browser_id);
  if (browser) {
    const auto rect = page->rect();
    handler->ResizeBrowser(browser, {0, 0, rect.width(), rect.height()});
  }
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
  CreateTab();
}

void MainWindow::OnCurrentTabChanged(int index) {
  const int browser_id = tab_bar_->GetBrowserIdAt(index);

  if (!tab_manager_.SetActiveBrowser(browser_id)) {
    return;
  }

  QWidget* page = host_pages_by_browser_id_.value(browser_id, nullptr);
  if (!page) {
    return;
  }

  stack_->setCurrentWidget(page);
  browser_ = tab_manager_.GetActiveBrowser();
  ResizeBrowserForPage(page);
  if (const auto frame = browser_->GetMainFrame()) {
    SetUrl(QString::fromStdString(frame->GetURL().ToString()));
  }
  SetNavigationState(browser_->IsLoading(), browser_->CanGoBack(), browser_->CanGoForward());
}

void MainWindow::OnCloseTabRequested(int index) {
  RemoveTab(index);
}

void MainWindow::CreateTab(const QString& url) {
  BrowserHandler* handler = BrowserHandler::GetInstance();
  if (!handler) {
    return;
  }

  auto* host_page = new QWidget(stack_);
  stack_->addWidget(host_page);
  host_page->installEventFilter(this);
  host_page->resize(stack_->contentsRect().size());

  QRect qBounds = host_page->rect();
  CefRect cefBounds(qBounds.x(), qBounds.y(), qBounds.width(), qBounds.height());

  CefWindowInfo window_info;
  window_info.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
  window_info.SetAsChild(reinterpret_cast<CefWindowHandle>(host_page->winId()), cefBounds);

  CefBrowserSettings browser_settings;

  if (!handler->CreateBrowserForHost(window_info, handler, url.toStdString(), browser_settings, host_page)) {
    stack_->removeWidget(host_page);
    delete host_page;
  }
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
  auto browser = tab_manager_.FindBrowser(browser_id);
  browser->GetHost()->CloseBrowser(false);
  tab_bar_->RemoveTab(browser_id);
}

TabManager& MainWindow::GetTabManager() {
  return tab_manager_;
}

void MainWindow::RegisterBrowser(CefRefPtr<CefBrowser> browser, QWidget* host_page) {
  if (!browser || !host_page || !tab_manager_.AddBrowser(browser)) {
    return;
  }
  host_pages_by_browser_id_.insert(browser->GetIdentifier(), host_page);
  {
    // addTab() can emit currentChanged before setTabData() stores the ID.
    QSignalBlocker blocker(tab_bar_);
    AddTab(browser->GetIdentifier(), QStringLiteral("new tab"));
    tab_bar_->SetActiveTab(browser->GetIdentifier());
  }
  OnCurrentTabChanged(tab_bar_->currentIndex());
}
