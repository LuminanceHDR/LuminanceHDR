#ifndef EXTWIZARDPAGE_H
#define EXTWIZARDPAGE_H

#include <QWizardPage>

namespace Ui {
class ExtWizardPage;
}

class ExtWizardPage : public QWizardPage {
   public:
    enum CompleteStatus { Undefined, AlwaysTrue, AlwaysFalse };

    Q_OBJECT

   public:
    ExtWizardPage(QWidget *parent = 0);

    void registerExtField(const QString &name, QWidget *widget,
                          const char *property = 0,
                          const char *changedSignal = 0);
    void setCompleteStatus(CompleteStatus newStatus);

    virtual bool isComplete() const;

   private:
    CompleteStatus m_completeStatus;
};

#endif  // EXTWIZARDPAGE_H
