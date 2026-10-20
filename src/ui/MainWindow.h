#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include <QMainWindow>

#include "include/internal/cef_types_wrappers.h"  // CefRect
#include "include/cef_browser.h"
#include <QStackedWidget>
#include <QVBoxLayout>

#include "browser/TabController.h"
#include "ui/TabBar.h"

class AddressBar;
class QPushButton;

// Qt-owned application window. Hosts the toolbar (back/forward/reload +
// address bar) and a plain container widget that CEF's Alloy-style browser
// view is parented into as a native child view.
//
// Construction builds the Qt shell. PetrichorApp calls CreateTab() only
// after CEF is initialized; subsequent tabs use that same creation path.
class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

  void CreateTab(const QString& url = QStringLiteral("https://www.google.com"));

  void SetUrl(const QString& url);

  void SetNavigationState(bool is_loading, bool canGoBack, bool canGoForward);
  void SetWindowTitle(const QString& url);

  void AddTab(int browser_id, QString title);
  void RemoveTab(int browser_id);
  TabController& GetTabController();


 protected:
  // Observe the actual host pages so CEF receives their post-layout size.
  bool eventFilter(QObject* watched, QEvent* event) override;

 private slots:
  // called when AddressBar emits urlSubmitted(). for now this has
  // no browser to talk to yet — just log it? Or stub for later wiring.
  void OnUrlEntered(const QString& url);

  void OnBackClicked();
  void OnForwardClicked();
  void OnReloadClicked();

  void OnNewTabRequested();
  void OnCurrentTabChanged(int index);
  void OnCloseTabRequested(int index);

 private:
  // build the toolbar (QToolBar or a layout with QPushButtons) and
  // the address bar, add them to the window.
  void BuildToolbar();

  // Build the tab bar and stack; CreateTab() adds each browser's host page.
  void BuildBrowserContainer();
  void ResizeBrowserForPage(QWidget* page);
  void ApplyActiveBrowser(int browser_id);

  AddressBar* address_bar_ = nullptr;
  QPushButton* back_button_ = nullptr;
  QPushButton* forward_button_ = nullptr;
  QPushButton* reload_button_ = nullptr;
  TabBar* tab_bar_ = nullptr;
  QStackedWidget* stack_ = nullptr;
  QWidget* central_widget_ = nullptr;
  QVBoxLayout* layout_ = nullptr;
  TabController tab_controller_;
};

#endif  // MAIN_WINDOW_H_
