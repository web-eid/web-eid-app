// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#include "controllerchildthread.hpp"

QMutex ControllerChildThread::controllerChildThreadMutex {};
QWaitCondition ControllerChildThread::waitForControllerNotify {};
