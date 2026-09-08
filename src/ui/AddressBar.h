#ifndef ADDRESS_BAR_H_
#define ADDRESS_BAR_H_

#include <QLineEdit>

// URL entry box. Emits urlSubmitted() with a normalized URL when the user
// presses Enter.
class AddressBar : public QLineEdit {
  Q_OBJECT

 public:
  explicit AddressBar(QWidget* parent = nullptr);

  // TODO: a setter to update the displayed text (e.g. when the browser
  // navigates elsewhere and you want the bar to reflect the real URL
  // without re-triggering urlSubmitted(). Think about why that matters.
  void SetUrl(const QString& url);

 signals:
  void urlSubmitted(const QString& url);

 private slots:
  // TODO: connect this to QLineEdit's own returnPressed() signal in the
  // constructor, and have it normalize + emit urlSubmitted().
  void OnReturnPressed();
};

#endif
