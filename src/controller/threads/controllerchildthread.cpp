#include "controllerchildthread.hpp"

QMutex ControllerChildThread::controllerChildThreadMutex {};
QWaitCondition ControllerChildThread::waitForControllerNotify {};
