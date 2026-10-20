#ifndef TAB_CONTROLLER_H_
#define TAB_CONTROLLER_H_

#include <QObject>
#include <QPointer>
#include <QWidget>
#include <vector>
#include "include/cef_browser.h"

// Coordinates tab creation, selection and closing on the Qt/CEF UI thread.
// MainWindow owns the widgets and responds to presentation signals.
class TabController : public QObject {
  Q_OBJECT
 public:
  struct Tab {
    int browser_id;
    CefRefPtr<CefBrowser> browser;
    QPointer<QWidget> host_page;
    bool closing = false;
  };
  using TabList = std::vector<Tab>;

  explicit TabController(QObject* parent = nullptr);
  bool CreateTab(const QString& url, QWidget* host_page);
  void RequestCloseTab(int browser_id, bool force_close = false);
  void RequestCloseAll(bool force_close = false);

  void BrowserCreated(CefRefPtr<CefBrowser> browser);
  bool BrowserReadyToClose(CefRefPtr<CefBrowser> browser);
  void BrowserClosed(CefRefPtr<CefBrowser> browser);

  bool SetActiveBrowser(int browser_id);
  CefRefPtr<CefBrowser> GetActiveBrowser() const;
  CefRefPtr<CefBrowser> FindBrowser(int browser_id) const;
  CefRefPtr<CefBrowser> FindBrowserForHostPage(QWidget* host_page) const;
  QWidget* FindHostPage(int browser_id) const;
  int GetActiveBrowserId() const { return active_browser_id_; }
  std::size_t Count() const { return tabs_.size() + (pending_host_page_ ? 1 : 0); }
  bool IsEmpty() const { return Count() == 0; }
  const TabList& GetTabs() const { return tabs_; }

  void AddressChanged(int browser_id, const QString& url);
  void TitleChanged(int browser_id, const QString& title);
  void LoadingStateChanged(int browser_id, bool loading, bool back, bool forward);

 signals:
  void TabAdded(int browser_id);
  void TabRemoved(int browser_id);
  void ActiveBrowserChanged(int browser_id);
  void HostCloseRequested(QWidget* host_page);
  void ActiveUrlChanged(const QString& url);
  void ActiveTitleChanged(const QString& title);
  void ActiveNavigationChanged(bool loading, bool back, bool forward);

 private:
  TabList tabs_;
  int active_browser_id_ = -1;
  QPointer<QWidget> pending_host_page_;
  bool close_pending_browser_ = false;
  bool force_close_pending_browser_ = false;
};
#endif  // TAB_CONTROLLER_H_
