#include <QPushButton>
#include <QEvent>
#include <QSignalBlocker>
#include <QToolBar>
#include <QWidget>

#include "ui/MainWindow.h"
#include "ui/AddressBar.h"
#include "browser/BrowserHandler.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), tab_controller_(this) {
  // call BuildToolbar() and BuildBrowserContainer()
  BuildToolbar();
  BuildBrowserContainer();
  connect(&tab_controller_, &TabController::TabAdded, this, [this](int id) {
    QSignalBlocker blocker(tab_bar_);
    AddTab(id, QStringLiteral("new tab"));
  });
  connect(&tab_controller_, &TabController::TabRemoved, this, &MainWindow::RemoveTab);
  connect(&tab_controller_, &TabController::ActiveBrowserChanged,
          this, &MainWindow::ApplyActiveBrowser);
  connect(&tab_controller_, &TabController::HostCloseRequested, this, [this](QWidget* page) {
    page->removeEventFilter(this);
    stack_->removeWidget(page);
    page->deleteLater();
  });
  connect(&tab_controller_, &TabController::ActiveUrlChanged, this, &MainWindow::SetUrl);
  connect(&tab_controller_, &TabController::ActiveTitleChanged, this, &MainWindow::SetWindowTitle);
  connect(&tab_controller_, &TabController::ActiveNavigationChanged,
          this, &MainWindow::SetNavigationState);
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
  const auto browser = tab_controller_.FindBrowserForHostPage(page);
  if (browser) {
    const auto rect = page->rect();
    handler->ResizeBrowser(browser, {0, 0, rect.width(), rect.height()});
  }
}

void MainWindow::OnUrlEntered(const QString& url) {
  const auto browser = tab_controller_.GetActiveBrowser();
  if (!browser) {
    return;
  }
  CefRefPtr<CefFrame> frame = browser->GetMainFrame();

  if (!frame) {
    return;
  }
  frame->LoadURL(url.toStdString());
}

void MainWindow::OnBackClicked() {
  const auto browser = tab_controller_.GetActiveBrowser();
  if (!browser) {
    return;
  }

  if (browser->CanGoBack()) {
    browser->GoBack();
  }
}

void MainWindow::OnForwardClicked() {
  const auto browser = tab_controller_.GetActiveBrowser();
  if (!browser) {
    return;
  }

  if (browser->CanGoForward()) {
    browser->GoForward();
  }
}

void MainWindow::OnReloadClicked() {
  const auto browser = tab_controller_.GetActiveBrowser();
  if (!browser) {
    return;
  }
  browser->Reload();
}

void MainWindow::OnNewTabRequested() {
  CreateTab();
}

void MainWindow::OnCurrentTabChanged(int index) {
  tab_controller_.SetActiveBrowser(tab_bar_->GetBrowserIdAt(index));
}

void MainWindow::ApplyActiveBrowser(int browser_id) {
  const auto browser = tab_controller_.GetActiveBrowser();
  QWidget* page = tab_controller_.FindHostPage(browser_id);
  if (!browser || !page) {
    SetUrl(QString());
    SetNavigationState(true, false, false);
    return;
  }
  {
    QSignalBlocker blocker(tab_bar_);
    tab_bar_->SetActiveTab(browser_id);
  }
  stack_->setCurrentWidget(page);
  ResizeBrowserForPage(page);
  if (const auto frame = browser->GetMainFrame())
    SetUrl(QString::fromStdString(frame->GetURL().ToString()));
  SetNavigationState(browser->IsLoading(), browser->CanGoBack(), browser->CanGoForward());
}

void MainWindow::OnCloseTabRequested(int index) {
  tab_controller_.RequestCloseTab(tab_bar_->GetBrowserIdAt(index));
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

  if (!tab_controller_.CreateTab(url, host_page)) {
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
  QSignalBlocker blocker(tab_bar_);
  tab_bar_->RemoveTab(browser_id);
}

TabController& MainWindow::GetTabController() {
  return tab_controller_;
}
