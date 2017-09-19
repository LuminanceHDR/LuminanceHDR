#include "ExtWizardPage.h"

ExtWizardPage::ExtWizardPage(QWidget *parent)
    : QWizardPage(parent), m_completeStatus(CompleteStatus::Undefined) {}

void ExtWizardPage::registerExtField(const QString &name, QWidget *widget,
                                     const char *property,
                                     const char *changedSignal) {
    QWizardPage::registerField(name, widget, property, changedSignal);
}

void ExtWizardPage::setCompleteStatus(CompleteStatus newStatus) {
    m_completeStatus = newStatus;
    emit completeChanged();
}

bool ExtWizardPage::isComplete() const {
    return m_completeStatus == AlwaysTrue
               ? true
               : (m_completeStatus == AlwaysFalse ? false
                                                  : QWizardPage::isComplete());
}
