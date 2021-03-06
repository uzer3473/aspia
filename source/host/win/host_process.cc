//
// PROJECT:         Aspia
// FILE:            host/win/host_process.cc
// LICENSE:         GNU General Public License 3
// PROGRAMMERS:     Dmitry Chapyshev (dmitry@aspia.ru)
//

#include "host/win/host_process.h"

#include <QDebug>

#include "host/win/host_process_impl.h"

namespace aspia {

HostProcess::HostProcess(QObject* parent)
    : QObject(parent)
{
    impl_.reset(new HostProcessImpl(this));
}

HostProcess::~HostProcess()
{
    impl_.reset();
}

void HostProcess::start(quint32 session_id,
                        Account account,
                        const QString& program,
                        const QStringList& arguments)
{
    if (impl_->state_ != NotRunning)
    {
        qWarning("HostProcess::start: Process is already running");
        return;
    }

    impl_->session_id_ = session_id;
    impl_->account_ = account;
    impl_->program_ = program;
    impl_->arguments_ = arguments;

    start();
}

quint32 HostProcess::sessionId() const
{
    return impl_->session_id_;
}

void HostProcess::setSessionId(quint32 session_id)
{
    if (impl_->state_ != NotRunning)
    {
        qWarning("HostProcess::setSessionId: Process is already running");
        return;
    }

    impl_->session_id_ = session_id;
}

HostProcess::Account HostProcess::account() const
{
    return impl_->account_;
}

void HostProcess::setAccount(Account account)
{
    if (impl_->state_ != NotRunning)
    {
        qWarning("HostProcess::setAccount: Process is already running");
        return;
    }

    impl_->account_ = account;
}

QString HostProcess::program() const
{
    return impl_->program_;
}

void HostProcess::setProgram(const QString& program)
{
    if (impl_->state_ != NotRunning)
    {
        qWarning("HostProcess::setProgram: Process is already running");
        return;
    }

    impl_->program_ = program;
}

QStringList HostProcess::arguments() const
{
    return impl_->arguments_;
}

void HostProcess::setArguments(const QStringList& arguments)
{
    if (impl_->state_ != NotRunning)
    {
        qWarning("HostProcess::setArguments: Process is already running");
        return;
    }

    impl_->arguments_ = arguments;
}

HostProcess::ProcessState HostProcess::state() const
{
    return impl_->state_;
}

void HostProcess::start()
{
    impl_->startProcess();
}

void HostProcess::kill()
{
    impl_->killProcess();
}

} // namespace aspia
