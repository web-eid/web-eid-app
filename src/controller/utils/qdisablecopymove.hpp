#pragma once

// Q_DISABLE_COPY_MOVE is available since Qt 5.13, define it for earlier versions.
// Assumes that Qt headers are already included.
#ifndef Q_DISABLE_COPY_MOVE
#define Q_DISABLE_COPY_MOVE(Class)                                                                 \
    Q_DISABLE_COPY(Class)                                                                          \
    Class(Class&&) = delete;                                                                       \
    Class& operator=(Class&&) = delete;
#endif
