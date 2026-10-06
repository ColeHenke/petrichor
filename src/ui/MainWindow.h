#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include <QMainWindow>

#include "include/internal/cef_types_wrappers.h"  // CefRect
#include "include/internal/cef_mac.h"             // CefWindowHandle
#include "include/cef_browser.h"
#include <QStackedWidget>
#include <QVBoxLayout>
#include "ui/TabBar.h"

class AddressBar;
class QPushButton;
class QResizeEvent;

// Qt-owned application window. Hosts the toolbar (back/forward/reload +
// address bar) and a plain container widget that CEF's Alloy-style browser
// view is parented into as a native child view.
//
// MainWindow does NOT create the CEF browser itself — CEF isn't initialized
// yet when this window is constructed. It only exposes the native handle
// and bounds that BrowserApp::OnContextInitialized() needs in order to call
// window_info.SetAsChild(handle, rect).
class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

  // Return the native handle of browser_container_ that CEF should
  // parent its view into. On mac this is the container's winId(), cast to
  // CefWindowHandle (an NSView*). Note: winId() forces native window
  // creation if it hasn't happened yet — call this only after show().
  CefWindowHandle GetCefParentHandle() const;

  // Return browser_container_'s current geometry as a CefRect
  // (x, y, width, height), used as the initial bounds for SetAsChild().
  CefRect GetCefParentRect() const;

  void SetBrowser(CefRefPtr<CefBrowser> browser);

  void SetUrl(const QString& url);

  void SetNavigationState(bool is_loading, bool canGoBack, bool canGoForward);
  void SetWindowTitle(const QString& url);

  void AddTab(int browser_id, QString title);
  void RemoveTab(int browser_id);


 protected:
  // Qt will call this whenever the window is resized. Eventually
  // CEF's child view needs to track this. Decide: do you handle it here,
  // or rely on Cocoa's autoresizing mask (set on the CEF view itself,
  // elsewhere) to do it for free? Leave a comment explaining your choice.
  void resizeEvent(QResizeEvent* event) override;

 private slots:
  // called when AddressBar emits urlSubmitted(). for now this has
  // no browser to talk to yet — just log it? Or stub for later wiring.
  void OnUrlEntered(const QString& url);

  void OnBackClicked();
  void OnForwardClicked();
  void OnReloadClicked();

  void OnNewTabRequested();

 private:
  // build the toolbar (QToolBar or a layout with QPushButtons) and
  // the address bar, add them to the window.
  void BuildToolbar();

  // create browser_container_ as the central widget (or inside a
  // layout) — this is the widget whose native view CEF renders into.
  void BuildBrowserContainer();

  QWidget* browser_container_ = nullptr;
  AddressBar* address_bar_ = nullptr;
  QPushButton* back_button_ = nullptr;
  QPushButton* forward_button_ = nullptr;
  QPushButton* reload_button_ = nullptr;
  CefRefPtr<CefBrowser> browser_;
  TabBar* tab_bar_ = nullptr;
  QStackedWidget* stack_ = nullptr;
  QWidget* central_widget_ = nullptr;
  QVBoxLayout* layout_ = nullptr;
};

#endif  // MAIN_WINDOW_H_
